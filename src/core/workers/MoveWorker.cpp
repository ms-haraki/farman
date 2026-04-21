#include "MoveWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

namespace Farman {

MoveWorker::MoveWorker(
  const QStringList& srcPaths,
  const QString&     dstDir,
  QObject*           parent)
  : WorkerBase(parent)
  , m_srcPaths(srcPaths)
  , m_dstDir(dstDir) {
}

void MoveWorker::run() {
  bool success = true;
  WorkerProgress progress;
  progress.processed = 0;
  progress.total = -1;
  progress.filesDone = 0;
  progress.filesTotal = m_srcPaths.size();

  // Ensure destination directory exists
  QDir dstDir(m_dstDir);
  if (!dstDir.exists()) {
    emit errorOccurred(m_dstDir, "Destination directory does not exist");
    emit finished(false);
    return;
  }

  // Move each source path
  for (const QString& srcPath : m_srcPaths) {
    if (isCancelled()) {
      emit finished(false);
      return;
    }

    QFileInfo srcInfo(srcPath);
    if (!srcInfo.exists()) {
      emit errorOccurred(srcPath, "Source does not exist");
      success = false;
      continue;
    }

    QString dstPath = m_dstDir + "/" + srcInfo.fileName();

    if (QFileInfo::exists(dstPath)) {
      OverwriteResolution resolution = resolveOverwrite(srcPath, dstPath);
      if (resolution.action == OverwriteResolution::Action::Cancel) {
        requestCancel();
        emit finished(false);
        return;
      }
      if (resolution.action == OverwriteResolution::Action::Rename) {
        dstPath = resolution.targetPath;
      } else {
        // Overwrite: 既存を削除
        QFileInfo dstInfo(dstPath);
        if (dstInfo.isDir()) {
          if (!removeDirectory(dstPath)) {
            emit errorOccurred(dstPath, "Failed to remove existing directory");
            success = false;
            continue;
          }
        } else {
          if (!QFile::remove(dstPath)) {
            emit errorOccurred(dstPath, "Failed to remove existing file");
            success = false;
            continue;
          }
        }
      }
    }

    progress.currentFile = srcPath;
    emit progressUpdated(progress);

    if (!moveEntry(srcPath, dstPath)) {
      success = false;
      continue;
    }

    progress.filesDone++;
    emit progressUpdated(progress);
  }

  emit finished(success);
}

bool MoveWorker::moveEntry(const QString& src, const QString& dst) {
  if (isCancelled()) {
    return false;
  }

  QFileInfo srcInfo(src);
  // run() 側でトップレベル競合は解決済み。ここでは dst は存在しない前提。

  // Try to rename first (fast for same filesystem)
  if (QFile::rename(src, dst)) {
    return true;
  }

  // If rename fails, copy and then delete
  if (srcInfo.isDir()) {
    if (!copyDirectory(src, dst)) {
      return false;
    }
    if (!removeDirectory(src)) {
      emit errorOccurred(src, "Failed to remove source directory after copy");
      return false;
    }
  } else {
    if (!copyFile(src, dst)) {
      return false;
    }
    if (!QFile::remove(src)) {
      emit errorOccurred(src, "Failed to remove source file after copy");
      return false;
    }
  }

  return true;
}

bool MoveWorker::copyFile(const QString& src, const QString& dst) {
  if (isCancelled()) {
    return false;
  }

  // Open source file
  QFile srcFile(src);
  if (!srcFile.open(QIODevice::ReadOnly)) {
    emit errorOccurred(src, "Failed to open source file");
    return false;
  }

  // Open destination file
  QFile dstFile(dst);
  if (!dstFile.open(QIODevice::WriteOnly)) {
    emit errorOccurred(dst, "Failed to create destination file");
    srcFile.close();
    return false;
  }

  // Copy in chunks for cancelability and progress reporting
  const qint64 CHUNK_SIZE = 1024 * 1024; // 1MB chunks
  qint64 totalSize = srcFile.size();
  qint64 bytesCopied = 0;

  while (!srcFile.atEnd()) {
    if (isCancelled()) {
      srcFile.close();
      dstFile.close();
      QFile::remove(dst); // Remove incomplete file
      return false;
    }

    QByteArray chunk = srcFile.read(CHUNK_SIZE);
    if (chunk.isEmpty()) {
      break;
    }

    qint64 bytesWritten = dstFile.write(chunk);
    if (bytesWritten != chunk.size()) {
      emit errorOccurred(dst, "Failed to write to destination file");
      srcFile.close();
      dstFile.close();
      QFile::remove(dst);
      return false;
    }

    bytesCopied += bytesWritten;

    // Report byte-level progress
    WorkerProgress progress;
    progress.currentFile = src;
    progress.processed = bytesCopied;
    progress.total = totalSize;
    progress.filesDone = 0;
    progress.filesTotal = 0;
    emit progressUpdated(progress);
  }

  srcFile.close();
  dstFile.close();

  // Preserve file permissions
  QFile::setPermissions(dst, QFile::permissions(src));

  return true;
}

bool MoveWorker::copyDirectory(const QString& src, const QString& dst) {
  if (isCancelled()) {
    return false;
  }

  QDir srcDir(src);
  QDir dstDir(dst);

  // Create destination directory
  if (!dstDir.exists()) {
    if (!QDir().mkpath(dst)) {
      emit errorOccurred(dst, "Failed to create directory");
      return false;
    }
  }

  // Iterate through all entries in the directory
  QDirIterator it(src, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
  bool success = true;

  while (it.hasNext()) {
    if (isCancelled()) {
      return false;
    }

    QString srcPath = it.next();
    QFileInfo srcInfo(srcPath);
    QString relativePath = srcDir.relativeFilePath(srcPath);
    QString dstPath = dst + "/" + relativePath;

    WorkerProgress progress;
    progress.currentFile = srcPath;
    progress.processed = 0;
    progress.total = -1;
    progress.filesDone = 0;
    progress.filesTotal = 0;
    emit progressUpdated(progress);

    if (srcInfo.isDir()) {
      if (!copyDirectory(srcPath, dstPath)) {
        success = false;
      }
    } else {
      if (!copyFile(srcPath, dstPath)) {
        success = false;
      }
    }
  }

  return success;
}

bool MoveWorker::removeDirectory(const QString& path) {
  if (isCancelled()) {
    return false;
  }

  QDir dir(path);
  if (!dir.exists()) {
    return true;
  }

  // Remove all entries in the directory
  QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
  bool success = true;

  while (it.hasNext()) {
    if (isCancelled()) {
      return false;
    }

    QString entryPath = it.next();
    QFileInfo info(entryPath);

    if (info.isDir()) {
      if (!removeDirectory(entryPath)) {
        success = false;
      }
    } else {
      if (!QFile::remove(entryPath)) {
        emit errorOccurred(entryPath, "Failed to remove file");
        success = false;
      }
    }
  }

  // Remove the directory itself
  if (success && !dir.rmdir(path)) {
    emit errorOccurred(path, "Failed to remove directory");
    return false;
  }

  return success;
}

} // namespace Farman

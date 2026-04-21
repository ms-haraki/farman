#include "CopyWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

namespace Farman {

CopyWorker::CopyWorker(
  const QStringList& srcPaths,
  const QString&     dstDir,
  QObject*           parent)
  : WorkerBase(parent)
  , m_srcPaths(srcPaths)
  , m_dstDir(dstDir) {
}

void CopyWorker::run() {
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

  // Copy each source path
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

    // トップレベルの競合解決（ディレクトリ or ファイル）
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
        // Overwrite: 既存を削除（dir は再帰的に）
        QFileInfo dstInfo(dstPath);
        if (dstInfo.isDir()) {
          QDir(dstPath).removeRecursively();
        } else {
          QFile::remove(dstPath);
        }
      }
    }

    progress.currentFile = srcPath;
    emit progressUpdated(progress);

    if (!copyEntry(srcPath, dstPath)) {
      success = false;
      continue;
    }

    progress.filesDone++;
    emit progressUpdated(progress);
  }

  emit finished(success);
}

bool CopyWorker::copyEntry(const QString& src, const QString& dst) {
  QFileInfo srcInfo(src);

  if (srcInfo.isDir()) {
    return copyDirectory(src, dst);
  } else {
    return copyFile(src, dst);
  }
}

bool CopyWorker::copyFile(const QString& src, const QString& dstIn) {
  if (isCancelled()) {
    return false;
  }

  QString dst = dstIn;
  if (QFileInfo::exists(dst)) {
    OverwriteResolution resolution = resolveOverwrite(src, dst);
    if (resolution.action == OverwriteResolution::Action::Cancel) {
      requestCancel();
      return false;
    }
    if (resolution.action == OverwriteResolution::Action::Rename) {
      dst = resolution.targetPath;
    } else {
      if (!QFile::remove(dst)) {
        emit errorOccurred(dst, "Failed to remove existing file");
        return false;
      }
    }
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

bool CopyWorker::copyDirectory(const QString& src, const QString& dst) {
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

} // namespace Farman

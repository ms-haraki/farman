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

  // 事前スキャンで全ファイル数を把握 (rename で一気に動く分も含む)
  m_progress = WorkerProgress{};
  m_progress.processed  = 0;
  m_progress.total      = -1;
  m_progress.filesDone  = 0;
  m_progress.filesTotal = countAllFiles(m_srcPaths);
  emit progressUpdated(m_progress);

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

    m_progress.currentFile = srcPath;
    emit progressUpdated(m_progress);

    // 事前にこの src 配下のファイル数を覚えておく。
    // rename で一気に移動できた場合 copyFile() が走らないので、
    // moveEntry 後にここまでの累積期待値まで filesDone を引き上げる。
    const int beforeDone   = m_progress.filesDone;
    const int expectedDone = beforeDone + countAllFiles({srcPath});

    if (!moveEntry(srcPath, dstPath)) {
      success = false;
      continue;
    }

    if (m_progress.filesDone < expectedDone) {
      m_progress.filesDone = expectedDone;
      emit progressUpdated(m_progress);
    }
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

    // バイト単位は補助情報として共有 m_progress に乗せる
    m_progress.currentFile = src;
    m_progress.processed   = bytesCopied;
    m_progress.total       = totalSize;
    emit progressUpdated(m_progress);
  }

  srcFile.close();
  dstFile.close();

  // Preserve file permissions
  QFile::setPermissions(dst, QFile::permissions(src));

  // 1 ファイル完了
  ++m_progress.filesDone;
  m_progress.processed = 0;
  m_progress.total     = -1;
  emit progressUpdated(m_progress);

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

    m_progress.currentFile = srcPath;
    emit progressUpdated(m_progress);

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

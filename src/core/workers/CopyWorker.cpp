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

bool CopyWorker::copyFile(const QString& src, const QString& dst) {
  if (isCancelled()) {
    return false;
  }

  QFileInfo dstInfo(dst);
  if (dstInfo.exists()) {
    // Ask for overwrite confirmation
    OverwriteResult result = askOverwrite(src, dst);

    switch (result) {
      case OverwriteResult::Yes:
      case OverwriteResult::YesAll:
        if (!QFile::remove(dst)) {
          emit errorOccurred(dst, "Failed to remove existing file");
          return false;
        }
        break;
      case OverwriteResult::No:
      case OverwriteResult::NoAll:
        return true; // Skip this file
      case OverwriteResult::Cancel:
        requestCancel();
        return false;
    }
  }

  if (!QFile::copy(src, dst)) {
    emit errorOccurred(src, "Failed to copy file");
    return false;
  }

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

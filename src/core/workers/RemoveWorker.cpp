#include "RemoveWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

namespace Farman {

RemoveWorker::RemoveWorker(
  const QStringList& paths,
  bool               toTrash,
  QObject*           parent)
  : WorkerBase(parent)
  , m_paths(paths)
  , m_toTrash(toTrash) {
}

void RemoveWorker::run() {
  bool success = true;
  WorkerProgress progress;
  progress.processed = 0;
  progress.total = -1;
  progress.filesDone = 0;
  progress.filesTotal = m_paths.size();

  // Remove each path
  for (const QString& path : m_paths) {
    if (isCancelled()) {
      emit finished(false);
      return;
    }

    QFileInfo info(path);
    if (!info.exists()) {
      emit errorOccurred(path, "Path does not exist");
      success = false;
      continue;
    }

    progress.currentFile = path;
    emit progressUpdated(progress);

    bool removeSuccess = false;
    if (m_toTrash) {
      // TODO: Implement trash functionality using platform-specific APIs
      // For now, fall back to permanent deletion
      removeSuccess = removeEntry(path);
    } else {
      removeSuccess = removeEntry(path);
    }

    if (!removeSuccess) {
      success = false;
      continue;
    }

    progress.filesDone++;
    emit progressUpdated(progress);
  }

  emit finished(success);
}

bool RemoveWorker::removeEntry(const QString& path) {
  if (isCancelled()) {
    return false;
  }

  QFileInfo info(path);

  if (info.isDir()) {
    return removeDirectory(path);
  } else {
    if (!QFile::remove(path)) {
      emit errorOccurred(path, "Failed to remove file");
      return false;
    }
    return true;
  }
}

bool RemoveWorker::removeDirectory(const QString& path) {
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

    WorkerProgress progress;
    progress.currentFile = entryPath;
    progress.processed = 0;
    progress.total = -1;
    progress.filesDone = 0;
    progress.filesTotal = 0;
    emit progressUpdated(progress);

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

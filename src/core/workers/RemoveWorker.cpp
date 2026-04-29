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

  m_progress = WorkerProgress{};
  m_progress.processed  = 0;
  m_progress.total      = -1;
  m_progress.filesDone  = 0;
  // Trash 移動時はディレクトリ全体を 1 操作で動かすので、トップレベル数で十分。
  // 完全削除時は再帰でファイル単位に削除するので、再帰スキャンで全件数える。
  m_progress.filesTotal = m_toTrash ? m_paths.size()
                                    : countAllFiles(m_paths);
  emit progressUpdated(m_progress);

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

    m_progress.currentFile = path;
    emit progressUpdated(m_progress);

    bool removeSuccess = false;
    if (m_toTrash) {
      if (!QFile::moveToTrash(path)) {
        emit errorOccurred(path, "Failed to move to trash");
        removeSuccess = false;
      } else {
        removeSuccess = true;
      }
    } else {
      removeSuccess = removeEntry(path);
    }

    if (!removeSuccess) {
      success = false;
      continue;
    }

    if (m_toTrash) {
      // ゴミ箱移動はトップレベル単位で 1 件ずつ進める
      ++m_progress.filesDone;
      emit progressUpdated(m_progress);
    }
    // 完全削除時は removeDirectory / 単一 QFile::remove 内で
    // filesDone をインクリメント済み。
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
    ++m_progress.filesDone;
    m_progress.currentFile = path;
    emit progressUpdated(m_progress);
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

    m_progress.currentFile = entryPath;
    emit progressUpdated(m_progress);

    if (info.isDir()) {
      if (!removeDirectory(entryPath)) {
        success = false;
      }
    } else {
      if (!QFile::remove(entryPath)) {
        emit errorOccurred(entryPath, "Failed to remove file");
        success = false;
      } else {
        ++m_progress.filesDone;
        emit progressUpdated(m_progress);
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

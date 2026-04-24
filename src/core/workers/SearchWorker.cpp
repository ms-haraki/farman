#include "SearchWorker.h"
#include <QDirIterator>

namespace Farman {

SearchWorker::SearchWorker(const QString&     rootPath,
                           const QStringList& namePatterns,
                           bool               includeSubdirs,
                           QObject*           parent)
  : WorkerBase(parent)
  , m_rootPath(rootPath)
  , m_namePatterns(namePatterns)
  , m_includeSubdirs(includeSubdirs) {
}

void SearchWorker::run() {
  // ファイルのみ対象（ディレクトリは再帰進入のために使うだけ）。
  // Hidden も含めて走査する。
  const QDir::Filters filters =
    QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot;
  const QDirIterator::IteratorFlags flags = m_includeSubdirs
    ? QDirIterator::Subdirectories
    : QDirIterator::NoIteratorFlags;

  // 空パターンはフィルタなし（全ファイル対象）
  QStringList patterns = m_namePatterns;
  patterns.removeAll(QString());

  QDirIterator it(m_rootPath, patterns, filters, flags);
  while (it.hasNext()) {
    if (isCancelled()) {
      emit finished(false);
      return;
    }
    const QString path = it.next();
    emit resultFound(path);
  }
  emit finished(true);
}

} // namespace Farman

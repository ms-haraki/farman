#include "SearchWorker.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace Farman {

SearchWorker::SearchWorker(const QString&     rootPath,
                           const QStringList& namePatterns,
                           const QStringList& excludeDirPatterns,
                           bool               includeSubdirs,
                           QObject*           parent)
  : WorkerBase(parent)
  , m_rootPath(rootPath)
  , m_namePatterns(namePatterns)
  , m_excludeDirPatterns(excludeDirPatterns)
  , m_includeSubdirs(includeSubdirs) {
  m_namePatterns.removeAll(QString());
  m_excludeDirPatterns.removeAll(QString());
}

void SearchWorker::run() {
  searchIn(m_rootPath);
  emit finished(!isCancelled());
}

void SearchWorker::searchIn(const QString& dirPath) {
  if (isCancelled()) return;

  QDir dir(dirPath);
  if (!dir.exists()) return;

  // ファイル（パターン適用）
  const QStringList filters = m_namePatterns.isEmpty()
    ? QStringList{QStringLiteral("*")}
    : m_namePatterns;
  const QFileInfoList files = dir.entryInfoList(
    filters,
    QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
  for (const QFileInfo& fi : files) {
    if (isCancelled()) return;
    emit resultFound(fi.absoluteFilePath());
  }

  if (!m_includeSubdirs) return;

  // サブディレクトリを手動で再帰。除外パターンに該当するものは入らない。
  const QFileInfoList subs = dir.entryInfoList(
    QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
  for (const QFileInfo& fi : subs) {
    if (isCancelled()) return;
    if (fi.isSymLink()) continue;  // ループ回避
    if (isExcludedDir(fi.fileName())) continue;
    searchIn(fi.absoluteFilePath());
  }
}

bool SearchWorker::isExcludedDir(const QString& dirName) const {
  for (const QString& pattern : m_excludeDirPatterns) {
    const QRegularExpression re = QRegularExpression::fromWildcard(
      pattern.trimmed(),
      Qt::CaseInsensitive);
    if (re.match(dirName).hasMatch()) return true;
  }
  return false;
}

} // namespace Farman

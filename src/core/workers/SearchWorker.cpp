#include "SearchWorker.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace Farman {

SearchWorker::SearchWorker(const QString&      rootPath,
                           const QStringList&  namePatterns,
                           const QStringList&  excludeDirPatterns,
                           const QStringList&  excludeFilePatterns,
                           bool                includeSubdirs,
                           const SearchFilter& filter,
                           QObject*            parent)
  : WorkerBase(parent)
  , m_rootPath(rootPath)
  , m_namePatterns(namePatterns)
  , m_excludeDirPatterns(excludeDirPatterns)
  , m_excludeFilePatterns(excludeFilePatterns)
  , m_includeSubdirs(includeSubdirs)
  , m_filter(filter) {
  m_namePatterns.removeAll(QString());
  m_excludeDirPatterns.removeAll(QString());
  m_excludeFilePatterns.removeAll(QString());
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
    if (isExcludedFile(fi.fileName())) continue;
    if (!matchesFilter(fi)) continue;
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

bool SearchWorker::isExcludedFile(const QString& fileName) const {
  for (const QString& pattern : m_excludeFilePatterns) {
    const QRegularExpression re = QRegularExpression::fromWildcard(
      pattern.trimmed(),
      Qt::CaseInsensitive);
    if (re.match(fileName).hasMatch()) return true;
  }
  return false;
}

bool SearchWorker::matchesFilter(const QFileInfo& fi) const {
  // サイズ
  if (m_filter.sizeEnabled) {
    const qint64 sz = fi.size();
    if (m_filter.minSize > 0 && sz < m_filter.minSize) return false;
    if (m_filter.maxSize > 0 && sz > m_filter.maxSize) return false;
  }

  // 更新日時
  if (m_filter.modifiedEnabled) {
    const QDateTime mt = fi.lastModified();
    if (m_filter.modifiedFrom.isValid() && mt < m_filter.modifiedFrom) return false;
    if (m_filter.modifiedTo.isValid()   && mt > m_filter.modifiedTo)   return false;
  }

  // 内容テキスト (大きいファイルはスキップ)
  if (m_filter.contentEnabled && !m_filter.contentBytes.isEmpty()) {
    if (fi.size() > m_filter.contentMaxScanBytes) return false;
    if (!fileContainsContent(fi.absoluteFilePath())) return false;
  }

  return true;
}

bool SearchWorker::fileContainsContent(const QString& filePath) const {
  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) return false;

  // チャンクで読みつつ検索。前のチャンクの末尾と次のチャンクの先頭を
  // 跨ぐマッチも拾えるよう、検索文字列長 - 1 バイトの残しを次に渡す。
  // 大文字小文字を無視するときはバイト列を予め lower にして比較。
  const bool cs = m_filter.contentCaseSensitive;
  const QByteArray needle = cs ? m_filter.contentBytes
                                : m_filter.contentBytes.toLower();
  const qint64 chunkSize = 64 * 1024;
  const int    overlap   = qMax(0, needle.size() - 1);
  QByteArray buf;
  buf.reserve(static_cast<int>(chunkSize) + overlap);

  while (!f.atEnd()) {
    if (isCancelled()) return false;
    QByteArray chunk = f.read(chunkSize);
    if (!cs) chunk = chunk.toLower();
    buf.append(chunk);
    if (buf.indexOf(needle) >= 0) return true;
    // 次回の overlap 用に末尾だけ残す
    if (buf.size() > overlap) {
      buf.remove(0, buf.size() - overlap);
    }
  }
  return false;
}

} // namespace Farman

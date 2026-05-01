#include "PropertiesWorker.h"
#include <QDir>
#include <QFileInfo>

namespace Farman {

PropertiesWorker::PropertiesWorker(const QString& rootPath, QObject* parent)
  : WorkerBase(parent)
  , m_rootPath(rootPath) {
}

void PropertiesWorker::run() {
  scan(m_rootPath);
  // 最終結果を必ず一回流す
  if (!isCancelled()) {
    emit statsUpdated(m_totalBytes, m_fileCount, m_dirCount);
  }
  emit finished(!isCancelled());
}

void PropertiesWorker::scan(const QString& dirPath) {
  if (isCancelled()) return;

  QDir dir(dirPath);
  if (!dir.exists()) return;

  // ファイル
  const QFileInfoList files = dir.entryInfoList(
    QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
  for (const QFileInfo& fi : files) {
    if (isCancelled()) return;
    if (fi.isSymLink()) continue;  // リンクはサイズ加算しない
    m_totalBytes += fi.size();
    ++m_fileCount;
    if ((++m_emitCounter & 0xFF) == 0) {
      // 256 件ごとに途中経過を emit (頻度を抑える)
      emit statsUpdated(m_totalBytes, m_fileCount, m_dirCount);
    }
  }

  // サブディレクトリを再帰
  const QFileInfoList subs = dir.entryInfoList(
    QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
  for (const QFileInfo& fi : subs) {
    if (isCancelled()) return;
    if (fi.isSymLink()) continue;  // ループ回避
    ++m_dirCount;
    scan(fi.absoluteFilePath());
  }
}

} // namespace Farman

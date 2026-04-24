#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

// ファイル名パターンに一致するファイルを rootPath 以下から探すワーカー。
// 逐次 resultFound(path) を emit し、完了時に finished(true) を発行する。
// キャンセルされた場合は finished(false) で終了する。
class SearchWorker : public WorkerBase {
  Q_OBJECT

public:
  SearchWorker(const QString&     rootPath,
               const QStringList& namePatterns,
               bool               includeSubdirs,
               QObject*           parent = nullptr);

signals:
  void resultFound(const QString& path);

protected:
  void run() override;

private:
  QString     m_rootPath;
  QStringList m_namePatterns;
  bool        m_includeSubdirs;
};

} // namespace Farman

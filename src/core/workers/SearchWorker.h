#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

// ファイル名パターンに一致するファイルを rootPath 以下から探すワーカー。
// 逐次 resultFound(path) を emit し、完了時に finished(true) を発行する。
// キャンセルされた場合は finished(false) で終了する。
//
// サブディレクトリは excludeDirPatterns（ディレクトリ名の glob パターン）
// にマッチするとそのサブツリーごとスキップする。シンボリックリンクは
// 無限再帰回避のため追跡しない。
class SearchWorker : public WorkerBase {
  Q_OBJECT

public:
  SearchWorker(const QString&     rootPath,
               const QStringList& namePatterns,
               const QStringList& excludeDirPatterns,
               bool               includeSubdirs,
               QObject*           parent = nullptr);

signals:
  void resultFound(const QString& path);

protected:
  void run() override;

private:
  void searchIn(const QString& dirPath);
  bool isExcludedDir(const QString& dirName) const;

  QString     m_rootPath;
  QStringList m_namePatterns;
  QStringList m_excludeDirPatterns;
  bool        m_includeSubdirs;
};

} // namespace Farman

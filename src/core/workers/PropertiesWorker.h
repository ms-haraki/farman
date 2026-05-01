#pragma once

#include "WorkerBase.h"
#include <QString>

namespace Farman {

// 与えられたディレクトリ配下を再帰的に走査して、合計サイズ / 合計
// ファイル数 / 合計ディレクトリ数を計算するワーカー。プロパティダイアログ
// から呼び出される。
//
// 走査中、一定間隔で statsUpdated を emit して途中経過を UI に流す。
// シンボリックリンクは追跡しない (無限ループ回避)。
//
// finished(bool) は WorkerBase の既定シグナル。キャンセルされたら false。
class PropertiesWorker : public WorkerBase {
  Q_OBJECT

public:
  PropertiesWorker(const QString& rootPath, QObject* parent = nullptr);

signals:
  // 途中経過。totalBytes はそれまで集計したバイト数、fileCount は通常
  // ファイル数、dirCount はディレクトリ数 (root 自身を含まない)。
  void statsUpdated(qint64 totalBytes, int fileCount, int dirCount);

protected:
  void run() override;

private:
  void scan(const QString& dirPath);

  QString m_rootPath;
  qint64  m_totalBytes = 0;
  int     m_fileCount  = 0;
  int     m_dirCount   = 0;
  // emit を間引くためのカウンタ
  int     m_emitCounter = 0;
};

} // namespace Farman

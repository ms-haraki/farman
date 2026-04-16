#pragma once

#include "types.h"
#include <QThread>
#include <QString>
#include <atomic>

namespace Farman {

// Worker → UI への進捗通知
struct WorkerProgress {
  QString currentFile;  // 現在処理中のファイル名
  qint64  processed;    // 処理済みバイト数
  qint64  total;        // 合計バイト数（不明な場合は -1）
  int     filesDone;    // 完了ファイル数
  int     filesTotal;   // 合計ファイル数
};

class WorkerBase : public QThread {
  Q_OBJECT

public:
  explicit WorkerBase(QObject* parent = nullptr);
  ~WorkerBase() override;

  // キャンセル要求（スレッドセーフ）
  void requestCancel();
  bool isCancelled() const;

signals:
  void progressUpdated(const WorkerProgress& progress);
  void overwriteRequired(
    const QString& srcPath,
    const QString& dstPath,
    OverwriteResult* result  // Worker側でブロックして待つ
  );
  void errorOccurred(const QString& path, const QString& message);
  void finished(bool success);

protected:
  void run() override = 0;

  // 上書き確認（UIスレッドと同期）
  OverwriteResult askOverwrite(
    const QString& srcPath,
    const QString& dstPath
  );

  std::atomic<bool> m_cancelRequested{false};
};

} // namespace Farman

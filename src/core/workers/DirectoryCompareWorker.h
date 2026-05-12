#pragma once

#include "WorkerBase.h"
#include "core/DirectoryCompare.h"
#include <QString>

namespace Farman {

// 左右 2 つのディレクトリを突き合わせて、各エントリに DiffStatus を割り当てる
// ワーカー。Phase A 版: NameOnly / SizeMtime 粒度、非再帰のみ対応。
// Hash 粒度と再帰比較は Phase B 以降。
//
// 結果は worker 完了後に `result()` で取り出す。WorkerBase の
// `progressUpdated` シグナルは Phase A では一度も emit しない (両ディレクトリ
// 走査は実質瞬時)。Hash モードを足す Phase B で進捗通知を追加する。
class DirectoryCompareWorker : public WorkerBase {
  Q_OBJECT

public:
  DirectoryCompareWorker(const QString&        leftDir,
                         const QString&        rightDir,
                         const CompareOptions& opts,
                         QObject*              parent = nullptr);

  // 完了後にメインスレッドから読み出す比較結果。
  const CompareResult& result() const { return m_result; }

protected:
  void run() override;

private:
  QString        m_leftDir;
  QString        m_rightDir;
  CompareOptions m_opts;
  CompareResult  m_result;
};

} // namespace Farman

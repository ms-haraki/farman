#include "DirectoryCompareWorker.h"
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHash>
#include <QSet>

namespace Farman {

namespace {

struct Snapshot {
  qint64    size  = -1;
  QDateTime mtime;
  bool      isDir = false;
};

// 1 ディレクトリ直下のエントリを列挙し、name → Snapshot のマップを作る。
// 隠しファイル含む。"." / ".." は除外。
QHash<QString, Snapshot> snapshotDir(const QString& dir) {
  QHash<QString, Snapshot> out;
  QDir d(dir);
  if (!d.exists() || !d.isReadable()) return out;
  const QFileInfoList list = d.entryInfoList(
    QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
    QDir::NoSort);
  for (const QFileInfo& info : list) {
    Snapshot s;
    s.size  = info.isDir() ? -1 : info.size();
    s.mtime = info.lastModified();
    s.isDir = info.isDir();
    out.insert(info.fileName(), s);
  }
  return out;
}

} // namespace

DirectoryCompareWorker::DirectoryCompareWorker(
  const QString&        leftDir,
  const QString&        rightDir,
  const CompareOptions& opts,
  QObject*              parent)
  : WorkerBase(parent)
  , m_leftDir(leftDir)
  , m_rightDir(rightDir)
  , m_opts(opts) {
}

void DirectoryCompareWorker::run() {
  // 左右のディレクトリを直下だけ列挙してスナップショット化。
  // Phase A は非再帰固定 (m_opts.recursive は読まない)。
  const QHash<QString, Snapshot> leftSnap  = snapshotDir(m_leftDir);
  const QHash<QString, Snapshot> rightSnap = snapshotDir(m_rightDir);

  // 同じ name が両側にあるかをまず分類する。
  for (auto it = leftSnap.cbegin(); it != leftSnap.cend(); ++it) {
    const QString& name = it.key();
    auto r = rightSnap.constFind(name);
    if (r == rightSnap.cend()) {
      m_result.left.insert(name, DiffStatus::OnlyHere);
      continue;
    }
    // 両側にある: 粒度に応じて Same / Differ を決定
    DiffStatus s = DiffStatus::Same;
    switch (m_opts.granularity) {
      case CompareGranularity::NameOnly:
        s = DiffStatus::Same;
        break;
      case CompareGranularity::SizeMtime: {
        // ディレクトリ同士の場合は名前一致を Same とする
        // (中身比較は再帰オプション、Phase B で対応)。
        if (it.value().isDir && r.value().isDir) {
          s = DiffStatus::Same;
        } else if (it.value().isDir != r.value().isDir) {
          // 片方がファイルでもう片方がディレクトリ → Differ
          s = DiffStatus::Differ;
        } else if (it.value().size != r.value().size) {
          s = DiffStatus::Differ;
        } else {
          // mtime は秒精度で比較 (FS によってナノ秒以下が落ちる)
          const qint64 lt = it.value().mtime.toSecsSinceEpoch();
          const qint64 rt = r.value().mtime.toSecsSinceEpoch();
          s = (lt == rt) ? DiffStatus::Same : DiffStatus::Differ;
        }
        break;
      }
      case CompareGranularity::Hash:
        // Phase B で実装。当面は SizeMtime と同じ動作 (フォールバック)。
        if (it.value().isDir && r.value().isDir) {
          s = DiffStatus::Same;
        } else if (it.value().size != r.value().size) {
          s = DiffStatus::Differ;
        } else {
          s = DiffStatus::Same;
        }
        break;
    }
    m_result.left.insert(name, s);
    m_result.right.insert(name, s);
  }

  // 右にしかない: 右側 overlay に OnlyHere、左側 overlay には現れない (= 左に
  // そもそも表示されないので)
  for (auto it = rightSnap.cbegin(); it != rightSnap.cend(); ++it) {
    if (!leftSnap.contains(it.key())) {
      m_result.right.insert(it.key(), DiffStatus::OnlyHere);
    }
  }

  emit finished(!isCancelled());
}

} // namespace Farman

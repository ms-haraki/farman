#include "MoveWorker.h"

namespace Farman {

MoveWorker::MoveWorker(
  const QStringList& srcPaths,
  const QString&     dstDir,
  QObject*           parent)
  : WorkerBase(parent)
  , m_srcPaths(srcPaths)
  , m_dstDir(dstDir) {
}

void MoveWorker::run() {
  // TODO: 実装
}

} // namespace Farman

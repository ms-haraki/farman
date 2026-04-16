#include "CopyWorker.h"

namespace Farman {

CopyWorker::CopyWorker(
  const QStringList& srcPaths,
  const QString&     dstDir,
  QObject*           parent)
  : WorkerBase(parent)
  , m_srcPaths(srcPaths)
  , m_dstDir(dstDir) {
}

void CopyWorker::run() {
  // TODO: 実装
}

bool CopyWorker::copyEntry(const QString& src, const QString& dst) {
  // TODO: 実装
  return false;
}

} // namespace Farman

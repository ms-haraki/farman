#include "RemoveWorker.h"

namespace Farman {

RemoveWorker::RemoveWorker(
  const QStringList& paths,
  bool               toTrash,
  QObject*           parent)
  : WorkerBase(parent)
  , m_paths(paths)
  , m_toTrash(toTrash) {
}

void RemoveWorker::run() {
  // TODO: 実装
}

} // namespace Farman

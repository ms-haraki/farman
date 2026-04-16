#include "WorkerBase.h"

namespace Farman {

WorkerBase::WorkerBase(QObject* parent) : QThread(parent) {
}

WorkerBase::~WorkerBase() = default;

void WorkerBase::requestCancel() {
  m_cancelRequested.store(true);
}

bool WorkerBase::isCancelled() const {
  return m_cancelRequested.load();
}

OverwriteResult WorkerBase::askOverwrite(
  const QString& srcPath,
  const QString& dstPath) {
  // TODO: 実装
  return OverwriteResult::Cancel;
}

} // namespace Farman

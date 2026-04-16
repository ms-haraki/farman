#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

class MoveWorker : public WorkerBase {
  Q_OBJECT
public:
  MoveWorker(
    const QStringList& srcPaths,
    const QString&     dstDir,
    QObject*           parent = nullptr
  );

protected:
  void run() override;

private:
  QStringList m_srcPaths;
  QString     m_dstDir;
};

} // namespace Farman

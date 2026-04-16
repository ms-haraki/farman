#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

class CopyWorker : public WorkerBase {
  Q_OBJECT
public:
  CopyWorker(
    const QStringList& srcPaths,
    const QString&     dstDir,
    QObject*           parent = nullptr
  );

protected:
  void run() override;

private:
  bool copyEntry(const QString& src, const QString& dst);

  QStringList m_srcPaths;
  QString     m_dstDir;
};

} // namespace Farman

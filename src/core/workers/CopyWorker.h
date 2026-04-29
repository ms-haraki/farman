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
  bool copyFile(const QString& src, const QString& dstIn);
  bool copyDirectory(const QString& src, const QString& dst);

  QStringList    m_srcPaths;
  QString        m_dstDir;
  // 再帰呼び出しで共有する全体進捗。filesTotal は run() で
  // 事前スキャン (countAllFiles) して埋める。
  WorkerProgress m_progress;
};

} // namespace Farman

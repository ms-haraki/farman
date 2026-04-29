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
  bool moveEntry(const QString& src, const QString& dst);
  bool copyFile(const QString& src, const QString& dst);
  bool copyDirectory(const QString& src, const QString& dst);
  bool removeDirectory(const QString& path);

  QStringList    m_srcPaths;
  QString        m_dstDir;
  // 再帰呼び出しで共有する全体進捗 (run() で countAllFiles 後の値を入れる)
  WorkerProgress m_progress;
};

} // namespace Farman

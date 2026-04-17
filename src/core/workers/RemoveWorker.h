#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

class RemoveWorker : public WorkerBase {
  Q_OBJECT
public:
  explicit RemoveWorker(
    const QStringList& paths,
    bool               toTrash = true,  // false = 完全削除
    QObject*           parent  = nullptr
  );

protected:
  void run() override;

private:
  bool removeEntry(const QString& path);
  bool removeDirectory(const QString& path);

  QStringList m_paths;
  bool        m_toTrash;
};

} // namespace Farman

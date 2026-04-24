#pragma once

#include "WorkerBase.h"

namespace Farman {

// 指定されたアーカイブを展開するワーカー。libarchive の read API を利用し、
// フォーマット自動判定で zip / tar / tar.gz / tar.bz2 / tar.xz などを扱う。
class ArchiveExtractWorker : public WorkerBase {
  Q_OBJECT

public:
  ArchiveExtractWorker(const QString& archivePath,
                       const QString& outputDir,
                       QObject*       parent = nullptr);

protected:
  void run() override;

private:
  QString m_archivePath;
  QString m_outputDir;
};

} // namespace Farman

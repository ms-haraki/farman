#pragma once

#include "WorkerBase.h"
#include <QStringList>

// libarchive の不透明構造体をグローバル名前空間で前方宣言しておく。
// namespace Farman 内に置くと Farman::archive として解釈されてしまう。
struct archive;

namespace Farman {

// 選択されたファイル／ディレクトリをアーカイブにまとめるワーカー。
// libarchive の write API を使用し、ディレクトリは再帰的に収める。
class ArchiveCreateWorker : public WorkerBase {
  Q_OBJECT

public:
  enum class Format {
    Zip,        // .zip
    Tar,        // .tar
    TarGz,      // .tar.gz
    TarBz2,     // .tar.bz2
    TarXz,      // .tar.xz
  };

  ArchiveCreateWorker(const QString&     outputPath,
                      Format             format,
                      const QStringList& inputPaths,
                      QObject*           parent = nullptr);

protected:
  void run() override;

private:
  // 1 ファイル分をアーカイブに追加する。成功なら true。
  bool addEntry(::archive*     a,
                const QString& absPath,
                const QString& entryName);
  // ディレクトリを再帰走査して addEntry する
  bool addDirectoryRecursive(::archive*     a,
                             const QString& absPath,
                             const QString& entryName);

  QString     m_outputPath;
  Format      m_format;
  QStringList m_inputPaths;
};

} // namespace Farman

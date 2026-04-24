#pragma once

#include "core/workers/ArchiveCreateWorker.h"
#include <QDialog>
#include <QString>
#include <QStringList>

class QLineEdit;
class QComboBox;
class QPushButton;

namespace Farman {

// 選択ファイルをアーカイブ化する前の設定ダイアログ。
// - Format（zip / tar / tar.gz / tar.bz2 / tar.xz）
// - Output directory（既定は反対ペイン）
// - Output filename
class CreateArchiveDialog : public QDialog {
  Q_OBJECT

public:
  CreateArchiveDialog(const QStringList& inputPaths,
                      const QString&     defaultOutputDir,
                      QWidget*           parent = nullptr);
  ~CreateArchiveDialog() override = default;

  QString                      outputPath() const;
  ArchiveCreateWorker::Format  format()     const;

protected:
  void keyPressEvent(QKeyEvent* event) override;

private slots:
  void onBrowseDir();
  void onFormatChanged();

private:
  void setupUi(const QString& defaultOutputDir);
  QString baseName() const;
  QString extensionForFormat(ArchiveCreateWorker::Format fmt) const;

  QStringList  m_inputPaths;
  QComboBox*   m_formatCombo;
  QLineEdit*   m_dirEdit;
  QPushButton* m_browseButton;
  QLineEdit*   m_nameEdit;
};

} // namespace Farman

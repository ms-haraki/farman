#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;

namespace Farman {

// アーカイブ展開ダイアログ。
// - 対象アーカイブパス（表示のみ）
// - 出力ディレクトリ（既定は反対ペイン）+ Browse
class ExtractArchiveDialog : public QDialog {
  Q_OBJECT

public:
  ExtractArchiveDialog(const QString& archivePath,
                       const QString& defaultOutputDir,
                       QWidget*       parent = nullptr);
  ~ExtractArchiveDialog() override = default;

  QString outputDirectory() const;

protected:
  void keyPressEvent(QKeyEvent* event) override;

private slots:
  void onBrowseDir();

private:
  void setupUi(const QString& archivePath, const QString& defaultOutputDir);

  QLineEdit*   m_dirEdit;
  QPushButton* m_browseButton;
};

} // namespace Farman

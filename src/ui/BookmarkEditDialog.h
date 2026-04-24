#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;

namespace Farman {

// ブックマーク（名前 + パス）を編集するダイアログ。
// - Name 欄（Alt+N）、Path 欄（Alt+P）、Browse... ボタン（Alt+B）
// - Browse... は QFileDialog::getExistingDirectory で選択
// - Cancel (Alt+X) / OK (Alt+O)。Enter で OK、Escape で Cancel
class BookmarkEditDialog : public QDialog {
  Q_OBJECT

public:
  BookmarkEditDialog(const QString& initialName,
                     const QString& initialPath,
                     QWidget* parent = nullptr);
  ~BookmarkEditDialog() override = default;

  QString name() const;
  QString path() const;

protected:
  void keyPressEvent(QKeyEvent* event) override;

private slots:
  void onBrowse();
  void updateOkButtonState();

private:
  void setupUi(const QString& initialName, const QString& initialPath);

  QLineEdit*   m_nameEdit;
  QLineEdit*   m_pathEdit;
  QPushButton* m_browseButton;
  QPushButton* m_okButton;
};

} // namespace Farman

#pragma once

#include "types.h"
#include <QDialog>

class QLineEdit;
class QPushButton;

namespace Farman {

// コピー／移動中に上書き競合が起きたとき（Ask モード）に都度表示する確認ダイアログ。
// 3 つのアクション:
//   - Overwrite: そのまま dst を上書き
//   - Rename:    入力されたファイル名でリネーム
//   - Cancel:    操作をキャンセル
class OverwriteDialog : public QDialog {
  Q_OBJECT

public:
  OverwriteDialog(const QString& srcPath,
                  const QString& dstPath,
                  QWidget* parent = nullptr);
  ~OverwriteDialog() override = default;

  OverwriteDecision decision() const { return m_decision; }

private slots:
  void onOverwrite();
  void onRename();
  void onCancel();
  void onRenameTextChanged(const QString& text);

private:
  void setupUi(const QString& srcPath, const QString& dstPath);

  QLineEdit*   m_renameEdit;
  QPushButton* m_overwriteButton;
  QPushButton* m_renameButton;
  QPushButton* m_cancelButton;

  QString m_originalName;  // 衝突している dst のファイル名（比較用）
  OverwriteDecision m_decision;
};

} // namespace Farman

#pragma once

#include <QMessageBox>

class QShowEvent;

namespace Farman {

// QMessageBox 派生クラス。macOS の「キーボードナビゲーション」OS 設定が OFF
// だと Tab フォーカス移動や Alt+key ショートカットが標準 QMessageBox では
// 動かない問題を、派生クラス側で「全 QPushButton に Qt::StrongFocus を
// 後付けし、Tab order を明示設定する」ことで回避する。
//
// 利用パターン:
//   FarmanMessageBox box(this);
//   box.setIcon(QMessageBox::Warning);
//   box.setText(...);
//   auto* okBtn = box.addButton(tr("OK"), QMessageBox::AcceptRole);
//   applyAltShortcut(okBtn, Qt::Key_O);
//   box.exec();
//
// addButton() で追加したボタンに対しても、showEvent のタイミングで
// enforceTabFocus() が自動実行されるので、フォーカス設定の呼び忘れがない。
class FarmanMessageBox : public QMessageBox {
  Q_OBJECT
public:
  FarmanMessageBox(QWidget* parent = nullptr);

  // 全ボタンに Qt::StrongFocus を設定 + 左→右 の Tab order を構築。
  // showEvent から自動で呼ばれるが、addButton 後に手動で呼んでも安全。
  void enforceTabFocus();

protected:
  void showEvent(QShowEvent* event) override;
};

} // namespace Farman

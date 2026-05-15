#include "FarmanMessageBox.h"
#include <QPushButton>
#include <QShowEvent>

namespace Farman {

FarmanMessageBox::FarmanMessageBox(QWidget* parent)
    : QMessageBox(parent) {
}

void FarmanMessageBox::enforceTabFocus() {
  // QMessageBox は QDialogButtonBox 内に QPushButton を生成する。
  // findChildren で再帰的に拾い、Qt::StrongFocus を明示設定する。
  // (macOS のキーボードナビゲーション設定が OFF でも Tab で巡回できる)。
  const auto buttons = findChildren<QPushButton*>();
  QPushButton* prev = nullptr;
  for (QPushButton* btn : buttons) {
    btn->setFocusPolicy(Qt::StrongFocus);
    if (prev) setTabOrder(prev, btn);
    prev = btn;
  }
}

void FarmanMessageBox::showEvent(QShowEvent* event) {
  QMessageBox::showEvent(event);
  enforceTabFocus();
}

} // namespace Farman

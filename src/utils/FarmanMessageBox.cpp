#include "FarmanMessageBox.h"
#include <QAbstractButton>
#include <QKeyEvent>
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

void FarmanMessageBox::setBareKeyShortcut(Qt::Key key, QAbstractButton* button) {
  if (!button) return;
  m_bareShortcuts.insert(static_cast<int>(key), button);
  // 子ボタンにフォーカスがある状態でも単押しを拾えるよう、各ボタンに
  // 自身を eventFilter として仕掛ける。ダイアログ自身の keyPressEvent と
  // 合わせて二段構えにする (ConfirmDialog 旧実装と同じパターン)。
  button->installEventFilter(this);
}

bool FarmanMessageBox::handleBareShortcut(QKeyEvent* event) {
  // 修飾キーなしの単押しだけ受ける。Shift / Ctrl / Alt / Meta は除外。
  // (KeypadModifier は OS が自動付加するので無視。)
  const auto mods = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier
                                          | Qt::AltModifier | Qt::MetaModifier);
  if (mods != Qt::NoModifier) return false;
  auto it = m_bareShortcuts.constFind(event->key());
  if (it == m_bareShortcuts.constEnd() || !it.value()) return false;
  it.value()->click();
  return true;
}

void FarmanMessageBox::keyPressEvent(QKeyEvent* event) {
  if (handleBareShortcut(event)) return;
  QMessageBox::keyPressEvent(event);
}

bool FarmanMessageBox::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    if (handleBareShortcut(static_cast<QKeyEvent*>(event))) return true;
  }
  return QMessageBox::eventFilter(watched, event);
}

void FarmanMessageBox::showEvent(QShowEvent* event) {
  QMessageBox::showEvent(event);
  enforceTabFocus();
}

} // namespace Farman

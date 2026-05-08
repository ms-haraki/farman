#include "EnterClickFilter.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QEvent>
#include <QKeyEvent>
#include <QWidget>

namespace Farman {

EnterClickFilter::EnterClickFilter(QObject* parent) : QObject(parent) {}

void EnterClickFilter::installOnButtonsIn(QWidget* root) {
  if (!root) return;
  // root 自身が対象ウィジェットの場合も含める。
  if (auto* selfBtn = qobject_cast<QAbstractButton*>(root)) {
    selfBtn->installEventFilter(this);
  } else if (auto* selfCombo = qobject_cast<QComboBox*>(root);
             selfCombo && !selfCombo->isEditable()) {
    selfCombo->installEventFilter(this);
  }
  // QAbstractButton 子孫すべてに installEventFilter。findChildren は
  // デフォルトで再帰的に探す (Qt::FindChildrenRecursively)。
  const auto buttons = root->findChildren<QAbstractButton*>();
  for (QAbstractButton* btn : buttons) {
    btn->installEventFilter(this);
  }
  // 編集不可な QComboBox 子孫にも install。編集可能な combo は内部
  // QLineEdit が Enter を確定として処理するので触らない。
  const auto combos = root->findChildren<QComboBox*>();
  for (QComboBox* combo : combos) {
    if (!combo->isEditable()) {
      combo->installEventFilter(this);
    }
  }
}

bool EnterClickFilter::eventFilter(QObject* obj, QEvent* ev) {
  if (ev->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(ev);
    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
      // QAction のショートカット処理と被らないよう、修飾子無し時のみ反応する。
      const auto mods = ke->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier
                                           | Qt::AltModifier  | Qt::MetaModifier);
      if (mods == Qt::NoModifier) {
        if (auto* btn = qobject_cast<QAbstractButton*>(obj)) {
          // checkable は click() でトグル + clicked シグナルが飛ぶ。
          // 通常ボタンも同様に click()。
          btn->click();
          ev->accept();
          return true;
        }
        if (auto* combo = qobject_cast<QComboBox*>(obj);
            combo && !combo->isEditable()) {
          // 編集不可コンボ: ドロップダウンを開く (= "Enter で発動" として
          // ユーザーが期待する素直な挙動)。
          combo->showPopup();
          ev->accept();
          return true;
        }
      }
    }
  }
  return QObject::eventFilter(obj, ev);
}

} // namespace Farman

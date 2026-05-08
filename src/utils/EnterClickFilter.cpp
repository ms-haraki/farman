#include "EnterClickFilter.h"

#include <QAbstractButton>
#include <QEvent>
#include <QKeyEvent>
#include <QWidget>

namespace Farman {

EnterClickFilter::EnterClickFilter(QObject* parent) : QObject(parent) {}

void EnterClickFilter::installOnButtonsIn(QWidget* root) {
  if (!root) return;
  // root 自身がボタンの場合も対象。
  if (auto* selfBtn = qobject_cast<QAbstractButton*>(root)) {
    selfBtn->installEventFilter(this);
  }
  // 全 QAbstractButton 子孫に installEventFilter。findChildren はデフォルトで
  // 再帰的に探す (Qt::FindChildrenRecursively)。
  const auto buttons = root->findChildren<QAbstractButton*>();
  for (QAbstractButton* btn : buttons) {
    btn->installEventFilter(this);
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
      }
    }
  }
  return QObject::eventFilter(obj, ev);
}

} // namespace Farman

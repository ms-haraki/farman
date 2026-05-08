#pragma once

#include <QObject>

class QWidget;

namespace Farman {

// QToolButton / QPushButton などのボタンが Tab フォーカスを取っているとき、
// Enter / Return キーをそのボタンの「クリック」として扱うようにする
// QObject 製のイベントフィルタ。
//
// ツールバー (メイン / ビュアー) では Tab で各ボタンを巡回したうえで Enter
// で押せると素直なキーボード操作ができるが、QToolButton の既定では
// Enter は反応しない (= 親に伝播してファイラの "ファイル開く" 等が誤発火)。
// そこをこのフィルタで吸収する。
//
// 使い方:
//   auto* f = new EnterClickFilter(parent);
//   f->installOnButtonsIn(toolbarWidget);  // QAbstractButton 子孫に一括 install
//
// install したフィルタは parent のライフタイムに従う。後から動的に追加された
// ボタンには別途 installOnButtonsIn を呼ぶこと。
class EnterClickFilter : public QObject {
  Q_OBJECT

public:
  explicit EnterClickFilter(QObject* parent = nullptr);

  // root 配下にある QAbstractButton 子孫すべてに、本フィルタを
  // installEventFilter する。root 自身がボタンの場合も対象に含める。
  void installOnButtonsIn(QWidget* root);

protected:
  bool eventFilter(QObject* obj, QEvent* ev) override;
};

} // namespace Farman

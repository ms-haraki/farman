#include "FileListDelegate.h"
#include <QPainter>
#include <QApplication>

namespace Farman {

FileListDelegate::FileListDelegate(QObject* parent)
  : QStyledItemDelegate(parent) {
}

void FileListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  QStyleOptionViewItem opt = option;

  // デフォルトのフォーカス矩形を描画しない
  opt.state &= ~QStyle::State_HasFocus;

  // デフォルトの選択ハイライトも描画しない（カーソル位置の背景色変更を防ぐ）
  opt.state &= ~QStyle::State_Selected;

  // デフォルトの描画（選択状態の背景色などはModelのQt::BackgroundRoleで制御）
  QStyledItemDelegate::paint(painter, opt, index);

  // カレントアイテム（カーソル）の場合は下線を描画
  // 最初のカラムでのみ描画して、行全体の幅で線を引く
  if ((option.state & QStyle::State_HasFocus) && index.column() == 0) {
    painter->save();

    // 下線の色（黒）
    QPen pen(Qt::black);
    pen.setWidth(2);
    painter->setPen(pen);

    // 行の下部に、ビューポート全体の幅で線を描画
    int y = option.rect.bottom();

    // widget() で親ウィジェットを取得して幅を取得
    const QWidget* widget = option.widget;
    if (widget) {
      int left = 0;
      int right = widget->width();
      painter->drawLine(left, y, right, y);
    } else {
      // フォールバック: セルの範囲で描画
      painter->drawLine(option.rect.left(), y, option.rect.right(), y);
    }

    painter->restore();
  }
}

} // namespace Farman

#include "FileListDelegate.h"
#include <QPainter>
#include <QApplication>
#include <QAbstractItemView>

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
  const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
  if (view && index.column() == 0) {
    QModelIndex currentIndex = view->currentIndex();
    if (currentIndex.isValid() && currentIndex.row() == index.row()) {
      painter->save();

      // 下線の色（黒）
      QPen pen(Qt::black);
      pen.setWidth(2);
      painter->setPen(pen);

      // 行の下部に、ビューポート全体の幅で線を描画
      int y = option.rect.bottom();
      int left = 0;
      int right = view->width();
      painter->drawLine(left, y, right, y);

      painter->restore();
    }
  }
}

} // namespace Farman

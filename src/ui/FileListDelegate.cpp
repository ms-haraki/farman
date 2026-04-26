#include "FileListDelegate.h"
#include "settings/Settings.h"
#include <QPainter>
#include <QApplication>
#include <QAbstractItemView>

namespace Farman {

FileListDelegate::FileListDelegate(QObject* parent)
  : QStyledItemDelegate(parent)
  , m_active(true) {
}

void FileListDelegate::setActive(bool active) {
  m_active = active;
}

void FileListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  QStyleOptionViewItem opt = option;

  // デフォルトのフォーカス矩形を描画しない
  opt.state &= ~QStyle::State_HasFocus;

  // デフォルトの選択ハイライトも描画しない（カーソル位置の背景色変更を防ぐ）
  opt.state &= ~QStyle::State_Selected;

  // カーソル行判定
  const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
  bool isCurrentRow = false;
  if (view) {
    const QModelIndex currentIndex = view->currentIndex();
    isCurrentRow = currentIndex.isValid() && currentIndex.row() == index.row();
  }

  const Settings& s = Settings::instance();
  QColor cursorColor = s.cursorColor(m_active);
  if (!cursorColor.isValid()) {
    cursorColor = m_active ? Qt::black : Qt::lightGray;
  }

  // RowBackground 形状の場合、デフォルト描画前に背景を塗っておく
  if (isCurrentRow && s.cursorShape() == CursorShape::RowBackground) {
    painter->fillRect(option.rect, cursorColor);
  }

  QStyledItemDelegate::paint(painter, opt, index);

  // Underline 形状の場合、行下端に線を描画
  if (isCurrentRow && s.cursorShape() == CursorShape::Underline) {
    painter->save();
    const int thickness = qMax(1, s.cursorThickness());
    QPen pen(cursorColor);
    pen.setWidth(thickness);
    painter->setPen(pen);
    // 太線が下端からはみ出さないよう中心を内側にオフセット
    const int y = option.rect.bottom() - thickness / 2;
    painter->drawLine(option.rect.left(), y, option.rect.right(), y);
    painter->restore();
  }
}

} // namespace Farman

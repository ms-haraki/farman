#pragma once

#include <QStyledItemDelegate>

namespace Farman {

class FileListDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit FileListDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
};

} // namespace Farman

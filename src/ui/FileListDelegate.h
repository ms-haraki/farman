#pragma once

#include <QStyledItemDelegate>

namespace Farman {

class FileListDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit FileListDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

  void setActive(bool active);
  bool isActive() const { return m_active; }

private:
  bool m_active;
};

} // namespace Farman

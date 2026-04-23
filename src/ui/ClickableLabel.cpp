#include "ClickableLabel.h"
#include <QMouseEvent>

namespace Farman {

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    emit clicked();
    event->accept();
    return;
  }
  QLabel::mousePressEvent(event);
}

} // namespace Farman

#include "FileListView.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>

namespace Farman {

FileListView::FileListView(QWidget* parent) : QTableView(parent) {
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::DragDrop);
}

void FileListView::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_dragStartPos = event->pos();
  }
  QTableView::mousePressEvent(event);
}

void FileListView::mouseMoveEvent(QMouseEvent* event) {
  if ((event->buttons() & Qt::LeftButton) && m_urlsProvider) {
    if ((event->pos() - m_dragStartPos).manhattanLength()
        >= QApplication::startDragDistance()) {
      startExternalDrag();
      return;
    }
  }
  QTableView::mouseMoveEvent(event);
}

void FileListView::startExternalDrag() {
  const QList<QUrl> urls = m_urlsProvider();
  if (urls.isEmpty()) return;

  auto* mime = new QMimeData;
  mime->setUrls(urls);
  auto* drag = new QDrag(this);
  drag->setMimeData(mime);
  // CopyAction を既定アクションにする (受け側アプリの判断は受け側に任せる)
  drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
}

void FileListView::dragEnterEvent(QDragEnterEvent* event) {
  // 自分自身の drag は受け取らない (同じディレクトリへの copy/move を防ぐ)
  if (event->mimeData()->hasUrls() && event->source() != this) {
    event->acceptProposedAction();
  } else {
    QTableView::dragEnterEvent(event);
  }
}

void FileListView::dragMoveEvent(QDragMoveEvent* event) {
  if (event->mimeData()->hasUrls() && event->source() != this) {
    event->acceptProposedAction();
  } else {
    QTableView::dragMoveEvent(event);
  }
}

void FileListView::dropEvent(QDropEvent* event) {
  if (!event->mimeData()->hasUrls() || event->source() == this) {
    QTableView::dropEvent(event);
    return;
  }
  emit externalUrlsDropped(event->mimeData()->urls());
  event->acceptProposedAction();
}

} // namespace Farman

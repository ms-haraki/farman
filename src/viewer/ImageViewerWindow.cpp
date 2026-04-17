#include "ImageViewerWindow.h"
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QPixmap>
#include <QMessageBox>
#include <QFileInfo>
#include <QResizeEvent>

namespace Farman {

ImageViewerWindow::ImageViewerWindow(const QString& filePath, QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
  , m_imageLabel(nullptr)
  , m_scrollArea(nullptr)
{
  setupUi();
  loadImage();
}

void ImageViewerWindow::setupUi() {
  // Set window title with file name
  QFileInfo fileInfo(m_filePath);
  setWindowTitle(QString("Image Viewer - %1").arg(fileInfo.fileName()));

  // Create central widget with layout
  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  // Create scroll area
  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setWidgetResizable(false);
  m_scrollArea->setAlignment(Qt::AlignCenter);

  // Create image label
  m_imageLabel = new QLabel(this);
  m_imageLabel->setScaledContents(false);
  m_imageLabel->setAlignment(Qt::AlignCenter);

  m_scrollArea->setWidget(m_imageLabel);
  layout->addWidget(m_scrollArea);
  setCentralWidget(centralWidget);

  // Set window size
  resize(800, 600);
}

void ImageViewerWindow::loadImage() {
  m_originalPixmap = QPixmap(m_filePath);

  if (m_originalPixmap.isNull()) {
    QMessageBox::critical(this, "Error",
      QString("Failed to load image: %1").arg(m_filePath));
    return;
  }

  scaleImage();
}

void ImageViewerWindow::scaleImage() {
  if (m_originalPixmap.isNull()) {
    return;
  }

  // Get available size (scroll area viewport size)
  QSize availableSize = m_scrollArea->viewport()->size();

  // Calculate scaled size maintaining aspect ratio
  QSize imageSize = m_originalPixmap.size();
  QSize scaledSize = imageSize;

  // Only scale down if image is larger than available space
  if (imageSize.width() > availableSize.width() ||
      imageSize.height() > availableSize.height()) {
    scaledSize = imageSize.scaled(availableSize, Qt::KeepAspectRatio);
  }

  // Set scaled pixmap
  QPixmap scaledPixmap = m_originalPixmap.scaled(
    scaledSize,
    Qt::KeepAspectRatio,
    Qt::SmoothTransformation
  );

  m_imageLabel->setPixmap(scaledPixmap);
  m_imageLabel->resize(scaledPixmap.size());
}

void ImageViewerWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  scaleImage();
}

} // namespace Farman

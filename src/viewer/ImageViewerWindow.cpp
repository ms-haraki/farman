#include "ImageViewerWindow.h"
#include "ImageView.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

namespace Farman {

ImageViewerWindow::ImageViewerWindow(const QString& filePath, QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
{
  setupUi();
  loadImage();
}

void ImageViewerWindow::setupUi() {
  QFileInfo fileInfo(m_filePath);
  setWindowTitle(QString("Image Viewer - %1").arg(fileInfo.fileName()));

  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  m_imageView = new ImageView(this);
  layout->addWidget(m_imageView);
  setCentralWidget(centralWidget);

  resize(800, 600);
}

void ImageViewerWindow::loadImage() {
  if (!m_imageView->loadFile(m_filePath)) {
    QMessageBox::critical(this, QStringLiteral("Error"),
      QStringLiteral("Failed to load image: %1").arg(m_filePath));
    return;
  }
  m_imageView->setFocus();
}

} // namespace Farman

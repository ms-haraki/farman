#pragma once

#include <QMainWindow>

namespace Farman {

class ImageView;

class ImageViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit ImageViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~ImageViewerWindow() override = default;

private:
  void setupUi();
  void loadImage();

  QString    m_filePath;
  ImageView* m_imageView = nullptr;
};

} // namespace Farman

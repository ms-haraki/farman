#pragma once

#include <QMainWindow>

class QLabel;
class QScrollArea;

namespace Farman {

class ImageViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit ImageViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~ImageViewerWindow() override = default;

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void setupUi();
  void loadImage();
  void scaleImage();

  QString m_filePath;
  QLabel* m_imageLabel;
  QScrollArea* m_scrollArea;
  QPixmap m_originalPixmap;
};

} // namespace Farman

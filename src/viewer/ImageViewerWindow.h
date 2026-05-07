#pragma once

#include <QMainWindow>

namespace Farman {

class ImageView;

class ImageViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit ImageViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~ImageViewerWindow() override = default;

protected:
  // Esc でウィンドウを閉じる。
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUi();
  void loadImage();
  // 画像の自然サイズに合わせてウィンドウを resize する。ツールバー / 余白
  // ぶんは画像表示領域 (m_imageView->sizeHint() ベース) との差で補正する。
  void fitWindowToImage();

  QString    m_filePath;
  ImageView* m_imageView = nullptr;
};

} // namespace Farman

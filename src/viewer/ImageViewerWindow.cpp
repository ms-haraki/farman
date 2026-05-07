#include "ImageViewerWindow.h"
#include "ImageView.h"

#include <QFileInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
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

  // 「ウィンドウサイズを画像にあわせる」ボタンを ImageView の既存ツールバー
  // (Zoom / Fit / Anim / Transparency) と同じ列に挿入する。
  // - ショートカット: Ctrl+1 (Total Commander 風の "1:1 表示" 流儀)
  // - Tab フォーカス: QPushButton のデフォルトは StrongFocus で OK だが、
  //   macOS のキーボードナビゲーション設定に依存しないよう明示する。
  auto* fitWinBtn = new QPushButton(tr("Fit window to image"), this);
  fitWinBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
  fitWinBtn->setFocusPolicy(Qt::StrongFocus);
  fitWinBtn->setToolTip(tr(
    "Resize this window so that the image is shown at its natural size "
    "(Ctrl+1). If the image is larger than the screen, the window is "
    "clamped to the available screen area."));
  connect(fitWinBtn, &QPushButton::clicked, this,
          &ImageViewerWindow::fitWindowToImage);
  m_imageView->addToolbarWidget(fitWinBtn);

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

void ImageViewerWindow::fitWindowToImage() {
  if (!m_imageView) return;
  const QSize imgSize = m_imageView->naturalImageSize();
  if (!imgSize.isValid() || imgSize.isEmpty()) return;

  // 現在の実効ズーム率 (Fit-to-Window 時はビューポートに基づく自動倍率) を
  // 反映する。ユーザーが Zoom = 25% にしているなら、ウィンドウは画像の
  // 25% サイズに収まるよう調整される。
  const int zoom = qMax(1, m_imageView->effectiveZoomPercent());
  const QSize scaledImage(
    qMax(1, imgSize.width()  * zoom / 100),
    qMax(1, imgSize.height() * zoom / 100));

  // 「ウィンドウ全体 - 画像表示エリア (= スクロールビューポート)」が
  // chrome (ウィンドウ枠 + ImageView ツールバー + 余白) の合計。
  // この部分はズームの影響を受けないので、固定値として扱う。
  // 何らかの事情で imageAreaSize が取れなければ ImageView 全体を chrome と
  // 見なすフォールバックを使う (= 旧挙動相当)。
  const QSize imageArea = m_imageView->imageAreaSize();
  const QSize chrome = (imageArea.isValid() && !imageArea.isEmpty())
                         ? size() - imageArea
                         : size() - m_imageView->size();

  QSize target = scaledImage + chrome;

  // 画面に収まらない場合はクランプ。利用可能領域 (タスクバー / メニューバーを
  // 除いたサイズ) を上限にする。利用可能領域が取れない場合は無制限。
  if (auto* scr = screen()) {
    const QSize avail = scr->availableGeometry().size();
    target = target.boundedTo(avail);
  }

  // 画像が極端に小さい (1x1 等) の場合に窓が消えるのを避けて最低サイズを設定。
  target = target.expandedTo(QSize(200, 150));

  resize(target);
}

void ImageViewerWindow::keyPressEvent(QKeyEvent* event) {
  // Esc でウィンドウを閉じる。WA_DeleteOnClose 付きで生成されているので破棄される。
  if (event->key() == Qt::Key_Escape) {
    close();
    return;
  }
  QMainWindow::keyPressEvent(event);
}

} // namespace Farman

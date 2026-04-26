#include "ViewerPanel.h"
#include "viewer/BinaryView.h"
#include "viewer/ImageView.h"
#include "viewer/TextView.h"
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

namespace Farman {

ViewerPanel::ViewerPanel(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
}

ViewerPanel::~ViewerPanel() = default;

void ViewerPanel::setupUi() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_stack = new QStackedWidget(this);
  layout->addWidget(m_stack);

  // ===== Text Viewer =====
  m_textView = new TextView(this);
  m_stack->addWidget(m_textView);

  // ===== Image Viewer =====
  m_imageView = new ImageView(this);
  m_stack->addWidget(m_imageView);

  // ===== Binary Viewer (fallback) =====
  m_binaryView = new BinaryView(this);
  m_stack->addWidget(m_binaryView);
}

bool ViewerPanel::openFile(const QString& filePath) {
  if (filePath.isEmpty()) {
    return false;
  }

  QFileInfo fileInfo(filePath);
  if (!fileInfo.exists() || !fileInfo.isFile()) {
    return false;
  }

  const QString extension = fileInfo.suffix().toLower();

  // 画像ファイル判定 (拡張子 or MIME)
  static const QStringList imageExtensions = {
    "png", "jpg", "jpeg", "gif", "bmp", "svg", "webp", "ico", "tiff", "tif"};
  QMimeDatabase mimeDb;
  const QMimeType mime = mimeDb.mimeTypeForFile(filePath);
  const QString mimeName = mime.name();

  if (imageExtensions.contains(extension) || mimeName.startsWith(QLatin1String("image/"))) {
    return openImageFile(filePath);
  }

  // テキスト判定: MIME が text/* または text/plain 派生
  if (mimeName.startsWith(QLatin1String("text/")) || mime.inherits(QStringLiteral("text/plain"))) {
    return openTextFile(filePath);
  }

  // それ以外はバイナリビュアーへフォールバック
  return openBinaryFile(filePath);
}

bool ViewerPanel::openTextFile(const QString& filePath) {
  if (!m_textView->loadFile(filePath)) {
    return false;
  }
  m_stack->setCurrentWidget(m_textView);
  setFocusProxy(m_textView);
  m_currentFilePath = filePath;
  emit fileOpened(filePath);
  return true;
}

bool ViewerPanel::openImageFile(const QString& filePath) {
  if (!m_imageView->loadFile(filePath)) {
    return false;
  }
  m_stack->setCurrentWidget(m_imageView);
  setFocusProxy(m_imageView);
  m_currentFilePath = filePath;
  emit fileOpened(filePath);
  return true;
}

bool ViewerPanel::openBinaryFile(const QString& filePath) {
  if (!m_binaryView->loadFile(filePath)) {
    return false;
  }
  m_stack->setCurrentWidget(m_binaryView);
  setFocusProxy(m_binaryView);
  m_currentFilePath = filePath;
  emit fileOpened(filePath);
  return true;
}

void ViewerPanel::clear() {
  m_textView->clearContent();
  m_imageView->clearContent();
  m_binaryView->clearContent();
  m_currentFilePath.clear();
  emit fileClosed();
}

} // namespace Farman

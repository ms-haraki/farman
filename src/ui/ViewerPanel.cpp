#include "ViewerPanel.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QFileInfo>
#include <QResizeEvent>

namespace Farman {

ViewerPanel::ViewerPanel(QWidget* parent)
  : QWidget(parent)
  , m_stack(nullptr)
  , m_textEdit(nullptr)
  , m_imageScrollArea(nullptr)
  , m_imageLabel(nullptr) {

  setupUi();
}

ViewerPanel::~ViewerPanel() = default;

void ViewerPanel::setupUi() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_stack = new QStackedWidget(this);
  layout->addWidget(m_stack);

  // ===== Text Viewer =====
  m_textEdit = new QTextEdit(this);
  m_textEdit->setReadOnly(true);
  m_textEdit->setLineWrapMode(QTextEdit::NoWrap);
  m_stack->addWidget(m_textEdit);

  // ===== Image Viewer =====
  m_imageScrollArea = new QScrollArea(this);
  m_imageScrollArea->setWidgetResizable(false);
  m_imageScrollArea->setAlignment(Qt::AlignCenter);

  m_imageLabel = new QLabel(this);
  m_imageLabel->setScaledContents(false);
  m_imageLabel->setAlignment(Qt::AlignCenter);

  m_imageScrollArea->setWidget(m_imageLabel);
  m_stack->addWidget(m_imageScrollArea);
}

bool ViewerPanel::openFile(const QString& filePath) {
  if (filePath.isEmpty()) {
    return false;
  }

  QFileInfo fileInfo(filePath);
  if (!fileInfo.exists() || !fileInfo.isFile()) {
    return false;
  }

  QString extension = fileInfo.suffix().toLower();

  // 画像ファイル判定
  QStringList imageExtensions = {"png", "jpg", "jpeg", "gif", "bmp", "svg", "webp", "ico", "tiff", "tif"};
  if (imageExtensions.contains(extension)) {
    return openImageFile(filePath);
  } else {
    // デフォルトはテキストファイルとして開く
    return openTextFile(filePath);
  }
}

bool ViewerPanel::openTextFile(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  QString content = in.readAll();
  m_textEdit->setPlainText(content);
  file.close();

  // Move cursor to beginning
  QTextCursor cursor = m_textEdit->textCursor();
  cursor.movePosition(QTextCursor::Start);
  m_textEdit->setTextCursor(cursor);

  m_stack->setCurrentWidget(m_textEdit);
  m_currentFilePath = filePath;
  emit fileOpened(filePath);

  return true;
}

bool ViewerPanel::openImageFile(const QString& filePath) {
  m_originalPixmap = QPixmap(filePath);

  if (m_originalPixmap.isNull()) {
    return false;
  }

  updateImageScale();

  m_stack->setCurrentWidget(m_imageScrollArea);
  m_currentFilePath = filePath;
  emit fileOpened(filePath);

  return true;
}

void ViewerPanel::updateImageScale() {
  if (m_originalPixmap.isNull()) {
    return;
  }

  // Get available size
  QSize availableSize = m_imageScrollArea->viewport()->size();

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

void ViewerPanel::clear() {
  m_textEdit->clear();
  m_imageLabel->clear();
  m_originalPixmap = QPixmap();
  m_currentFilePath.clear();
  emit fileClosed();
}

void ViewerPanel::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  // 画像表示中の場合はリサイズに応じて再スケール
  if (m_stack->currentWidget() == m_imageScrollArea) {
    updateImageScale();
  }
}

} // namespace Farman

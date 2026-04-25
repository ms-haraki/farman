#include "ViewerPanel.h"
#include "viewer/BinaryView.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QResizeEvent>

namespace Farman {

ViewerPanel::ViewerPanel(QWidget* parent)
  : QWidget(parent)
  , m_stack(nullptr)
  , m_textEdit(nullptr)
  , m_imageScrollArea(nullptr)
  , m_imageLabel(nullptr)
  , m_binaryView(nullptr) {

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
  // (TextViewerPlugin と分散しないよう将来 Settings 化予定 — Phase 4)
  if (mimeName.startsWith(QLatin1String("text/")) || mime.inherits(QStringLiteral("text/plain"))) {
    return openTextFile(filePath);
  }

  // それ以外はバイナリビュアーへフォールバック
  return openBinaryFile(filePath);
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

bool ViewerPanel::openBinaryFile(const QString& filePath) {
  if (!m_binaryView->loadFile(filePath)) {
    return false;
  }

  m_stack->setCurrentWidget(m_binaryView);
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
  m_binaryView->clear();
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

#include "ViewerPanel.h"
#include "settings/Settings.h"
#include "viewer/BinaryView.h"
#include "viewer/ImageView.h"
#include "viewer/TextView.h"
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>

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

namespace {

// 拡張子パターン (大文字小文字無視) とマッチするか。
//   - `*` / `?` を含む場合はグロブとして解釈 (例: "c*" は "c", "cc", "cpp" などに一致)
//   - `!` プレフィックスは除外パターン (例: "c* !class" は class を除く c 系拡張子)
//   - 除外がマッチすると即座に不一致確定
//   - 通常パターンが 1 つもなければ何もマッチしない
bool extensionMatches(const QStringList& patterns, const QString& extension) {
  auto patternMatches = [&](const QString& p) {
    if (p.contains(QLatin1Char('*')) || p.contains(QLatin1Char('?'))) {
      QRegularExpression re(
        QRegularExpression::wildcardToRegularExpression(p),
        QRegularExpression::CaseInsensitiveOption);
      return re.match(extension).hasMatch();
    }
    return extension.compare(p, Qt::CaseInsensitive) == 0;
  };

  bool anyInclude = false;
  bool included   = false;
  for (const QString& raw : patterns) {
    QString p = raw.trimmed();
    if (p.isEmpty()) continue;
    const bool isExclude = p.startsWith(QLatin1Char('!'));
    if (isExclude) {
      p = p.mid(1).trimmed();
      if (p.isEmpty()) continue;
      if (patternMatches(p)) return false;  // 除外マッチで即不一致
    } else {
      anyInclude = true;
      if (patternMatches(p)) included = true;
    }
  }
  return anyInclude && included;
}

// MIME パターンとマッチするか。末尾 `*` で前方一致、それ以外は完全一致
// または inherits 判定。
bool mimeMatches(const QStringList& patterns, const QMimeType& mime) {
  const QString name = mime.name();
  for (const QString& p : patterns) {
    const QString trimmed = p.trimmed();
    if (trimmed.isEmpty()) continue;
    if (trimmed.endsWith(QLatin1Char('*'))) {
      const QString prefix = trimmed.left(trimmed.size() - 1);
      if (name.startsWith(prefix, Qt::CaseInsensitive)) return true;
    } else {
      if (name.compare(trimmed, Qt::CaseInsensitive) == 0) return true;
      if (mime.inherits(trimmed)) return true;
    }
  }
  return false;
}

} // anonymous namespace

bool ViewerPanel::openFile(const QString& filePath) {
  if (filePath.isEmpty()) {
    return false;
  }

  QFileInfo fileInfo(filePath);
  if (!fileInfo.exists() || !fileInfo.isFile()) {
    return false;
  }

  const QString extension = fileInfo.suffix().toLower();
  QMimeDatabase mimeDb;
  const QMimeType mime = mimeDb.mimeTypeForFile(filePath);

  const Settings& s = Settings::instance();

  // 画像ビュアー: 拡張子 (グロブ可) または MIME パターンマッチ
  if (extensionMatches(s.imageViewerExtensions(), extension)
      || mimeMatches(s.imageViewerMimePatterns(), mime)) {
    return openImageFile(filePath);
  }

  // テキストビュアー: 拡張子 (グロブ可) または MIME パターンマッチ
  if (extensionMatches(s.textViewerExtensions(), extension)
      || mimeMatches(s.textViewerMimePatterns(), mime)) {
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
  emit viewerStatusChanged(filePath, m_textView->statusInfo());
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
  emit viewerStatusChanged(filePath, m_imageView->statusInfo());
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
  emit viewerStatusChanged(filePath, m_binaryView->statusInfo());
  return true;
}

void ViewerPanel::clear() {
  m_textView->clearContent();
  m_imageView->clearContent();
  m_binaryView->clearContent();
  m_currentFilePath.clear();
  emit fileClosed();
  emit viewerStatusChanged(QString(), QString());
}

} // namespace Farman

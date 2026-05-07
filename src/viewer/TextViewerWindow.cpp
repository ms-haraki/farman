#include "TextViewerWindow.h"
#include "TextView.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QFileInfo>
#include <QKeyEvent>

namespace Farman {

TextViewerWindow::TextViewerWindow(const QString& filePath, QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
  , m_textView(nullptr)
{
  setupUi();
  loadFile();
}

void TextViewerWindow::setupUi() {
  QFileInfo fileInfo(m_filePath);
  setWindowTitle(QString("Text Viewer - %1").arg(fileInfo.fileName()));

  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  m_textView = new TextView(this);
  layout->addWidget(m_textView);
  setCentralWidget(centralWidget);

  resize(800, 600);
}

void TextViewerWindow::loadFile() {
  if (!m_textView->loadFile(m_filePath)) {
    QMessageBox::critical(this, QStringLiteral("Error"),
      QStringLiteral("Failed to open file: %1").arg(m_filePath));
    return;
  }
  m_textView->setFocus();
}

void TextViewerWindow::keyPressEvent(QKeyEvent* event) {
  // Esc でウィンドウを閉じる (Inline モード時の Enter / Esc 戻りと挙動を揃える)。
  // close() は WA_DeleteOnClose 付きで生成されているのでウィンドウは破棄される。
  if (event->key() == Qt::Key_Escape) {
    close();
    return;
  }
  QMainWindow::keyPressEvent(event);
}

} // namespace Farman

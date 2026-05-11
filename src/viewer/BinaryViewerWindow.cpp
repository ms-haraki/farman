#include "BinaryViewerWindow.h"
#include "BinaryView.h"

#include <QFileInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

namespace Farman {

BinaryViewerWindow::BinaryViewerWindow(const QString& filePath,
                                       const QString& displayPath,
                                       QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
  , m_displayPath(displayPath.isEmpty() ? filePath : displayPath)
  , m_view(nullptr)
{
  QFileInfo fileInfo(m_displayPath);
  setWindowTitle(QString("Binary Viewer - %1").arg(fileInfo.fileName()));

  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(0, 0, 0, 0);

  m_view = new BinaryView(this);
  layout->addWidget(m_view);
  setCentralWidget(central);

  resize(800, 600);

  if (!m_view->loadFile(filePath)) {
    QMessageBox::critical(this, QStringLiteral("Error"),
      QStringLiteral("Failed to open file: %1").arg(filePath));
  }
  // ウィンドウを表示しただけでは Qt の auto-first-responder 機構が
  // 「最初の focusable 子孫」(= ツールバー先頭の Unit コンボ) を選んでしまう。
  // BinaryView の setFocusProxy(m_textArea) を効かせるため、明示的に setFocus を呼ぶ
  // (TextViewerWindow / ImageViewerWindow と同じ流儀)。
  m_view->setFocus();
}

void BinaryViewerWindow::keyPressEvent(QKeyEvent* event) {
  // Esc / Enter / Return でウィンドウを閉じる (Inline モード時の戻り操作と
  // 挙動を揃える)。close() は WA_DeleteOnClose 付きで生成されているので
  // ウィンドウは破棄される。
  if (event->key() == Qt::Key_Escape ||
      event->key() == Qt::Key_Return ||
      event->key() == Qt::Key_Enter) {
    close();
    return;
  }
  QMainWindow::keyPressEvent(event);
}

} // namespace Farman

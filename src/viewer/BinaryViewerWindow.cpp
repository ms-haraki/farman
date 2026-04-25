#include "BinaryViewerWindow.h"
#include "BinaryView.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

namespace Farman {

BinaryViewerWindow::BinaryViewerWindow(const QString& filePath, QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
  , m_view(nullptr)
{
  QFileInfo fileInfo(filePath);
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
}

} // namespace Farman

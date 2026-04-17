#include "TextViewerPlugin.h"
#include "TextViewerWindow.h"

namespace Farman {

QWidget* TextViewerPlugin::createViewer(
  const QString& filePath,
  QWidget* parent)
{
  auto* window = new TextViewerWindow(filePath, parent);
  window->setAttribute(Qt::WA_DeleteOnClose);
  window->show();
  return window;
}

} // namespace Farman

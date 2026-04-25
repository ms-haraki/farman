#include "BinaryViewerPlugin.h"
#include "BinaryViewerWindow.h"

namespace Farman {

QWidget* BinaryViewerPlugin::createViewer(
  const QString& filePath,
  QWidget* parent)
{
  auto* window = new BinaryViewerWindow(filePath, parent);
  window->setAttribute(Qt::WA_DeleteOnClose);
  window->show();
  return window;
}

} // namespace Farman

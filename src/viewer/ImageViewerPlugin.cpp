#include "ImageViewerPlugin.h"
#include "ImageViewerWindow.h"

namespace Farman {

QWidget* ImageViewerPlugin::createViewer(
  const QString& filePath,
  QWidget* parent)
{
  auto* window = new ImageViewerWindow(filePath, parent);
  window->setAttribute(Qt::WA_DeleteOnClose);
  window->show();
  return window;
}

} // namespace Farman

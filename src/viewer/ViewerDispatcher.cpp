#include "ViewerDispatcher.h"

namespace Farman {

ViewerDispatcher& ViewerDispatcher::instance() {
  static ViewerDispatcher instance;
  return instance;
}

ViewerDispatcher::ViewerDispatcher(QObject* parent) : QObject(parent) {
}

void ViewerDispatcher::registerBuiltins() {
  // TODO: 実装
}

void ViewerDispatcher::loadPlugins(const QDir& pluginDir) {
  // TODO: 実装
}

IViewerPlugin* ViewerDispatcher::resolvePlugin(const QString& filePath) const {
  // TODO: 実装
  return nullptr;
}

QWidget* ViewerDispatcher::createViewer(
  const QString& filePath,
  QWidget*       parent) const {
  // TODO: 実装
  return nullptr;
}

QList<IViewerPlugin*> ViewerDispatcher::allPlugins() const {
  // TODO: 実装
  return {};
}

void ViewerDispatcher::registerPlugin(std::shared_ptr<IViewerPlugin> plugin) {
  // TODO: 実装
}

bool IViewerPlugin::canHandle(const QString& filePath) const {
  // TODO: 実装
  return false;
}

} // namespace Farman

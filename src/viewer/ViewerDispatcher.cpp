#include "ViewerDispatcher.h"
#include "TextViewerPlugin.h"
#include "ImageViewerPlugin.h"
#include <QFileInfo>
#include <QMimeDatabase>
#include <QPluginLoader>

namespace Farman {

ViewerDispatcher& ViewerDispatcher::instance() {
  static ViewerDispatcher instance;
  return instance;
}

ViewerDispatcher::ViewerDispatcher(QObject* parent) : QObject(parent) {
}

void ViewerDispatcher::registerBuiltins() {
  // Register built-in viewers
  registerPlugin(std::make_shared<TextViewerPlugin>());
  registerPlugin(std::make_shared<ImageViewerPlugin>());
}

void ViewerDispatcher::loadPlugins(const QDir& pluginDir) {
  // Load external plugins from directory
  if (!pluginDir.exists()) {
    return;
  }

  QStringList filters;
  filters << "*.so" << "*.dylib" << "*.dll";

  QFileInfoList plugins = pluginDir.entryInfoList(filters, QDir::Files);
  for (const QFileInfo& fileInfo : plugins) {
    QPluginLoader loader(fileInfo.absoluteFilePath());
    QObject* plugin = loader.instance();

    if (plugin) {
      IViewerPlugin* viewerPlugin = qobject_cast<IViewerPlugin*>(plugin);
      if (viewerPlugin) {
        // Wrap in shared_ptr with custom deleter that doesn't delete
        // (plugin loader manages the lifetime)
        registerPlugin(std::shared_ptr<IViewerPlugin>(viewerPlugin, [](IViewerPlugin*){}));
      }
    }
  }
}

IViewerPlugin* ViewerDispatcher::resolvePlugin(const QString& filePath) const {
  if (filePath.isEmpty()) {
    return nullptr;
  }

  QFileInfo fileInfo(filePath);
  if (!fileInfo.exists() || !fileInfo.isFile()) {
    return nullptr;
  }

  QString extension = fileInfo.suffix().toLower();

  // Find matching plugin with highest priority
  IViewerPlugin* bestMatch = nullptr;
  int highestPriority = -1;

  for (const auto& plugin : m_plugins) {
    // Check if plugin can handle this file
    if (plugin->canHandle(filePath)) {
      if (plugin->priority() > highestPriority) {
        bestMatch = plugin.get();
        highestPriority = plugin->priority();
      }
    } else if (!extension.isEmpty() &&
               plugin->supportedExtensions().contains(extension, Qt::CaseInsensitive)) {
      // Check by extension
      if (plugin->priority() > highestPriority) {
        bestMatch = plugin.get();
        highestPriority = plugin->priority();
      }
    }
  }

  return bestMatch;
}

QWidget* ViewerDispatcher::createViewer(
  const QString& filePath,
  QWidget*       parent) const {

  IViewerPlugin* plugin = resolvePlugin(filePath);
  if (plugin) {
    return plugin->createViewer(filePath, parent);
  }

  // No viewer found - could fallback to hex viewer in future
  return nullptr;
}

QList<IViewerPlugin*> ViewerDispatcher::allPlugins() const {
  QList<IViewerPlugin*> result;
  result.reserve(m_plugins.size());

  for (const auto& plugin : m_plugins) {
    result.append(plugin.get());
  }

  return result;
}

void ViewerDispatcher::registerPlugin(std::shared_ptr<IViewerPlugin> plugin) {
  if (!plugin) {
    return;
  }

  // Check if plugin with same ID already exists
  for (const auto& existingPlugin : m_plugins) {
    if (existingPlugin->pluginId() == plugin->pluginId()) {
      return; // Already registered
    }
  }

  m_plugins.append(plugin);
}

bool IViewerPlugin::canHandle(const QString& filePath) const {
  QFileInfo fileInfo(filePath);
  QString extension = fileInfo.suffix().toLower();

  // Check by extension
  if (!extension.isEmpty() &&
      supportedExtensions().contains(extension, Qt::CaseInsensitive)) {
    return true;
  }

  // Check by MIME type
  QMimeDatabase mimeDb;
  QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);

  for (const QString& supportedMime : supportedMimeTypes()) {
    if (mimeType.inherits(supportedMime)) {
      return true;
    }
  }

  return false;
}

} // namespace Farman

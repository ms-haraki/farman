#pragma once

#include "IViewerPlugin.h"
#include <QObject>
#include <QDir>
#include <memory>

namespace Farman {

class ViewerDispatcher : public QObject {
  Q_OBJECT

public:
  static ViewerDispatcher& instance();

  // 組み込みビュアーの登録（起動時に呼ぶ）
  void registerBuiltins();

  // プラグインディレクトリからロード
  void loadPlugins(const QDir& pluginDir);

  // ファイルに対応するビュアーを返す（nullptr = 対応なし）
  IViewerPlugin* resolvePlugin(const QString& filePath) const;

  // ビュアーウィジェットを生成して返す
  // 対応ビュアーがない場合は HexViewer にフォールバック
  QWidget* createViewer(
    const QString& filePath,
    QWidget*       parent = nullptr
  ) const;

  // 全登録プラグイン一覧
  QList<IViewerPlugin*> allPlugins() const;

private:
  explicit ViewerDispatcher(QObject* parent = nullptr);
  void registerPlugin(std::shared_ptr<IViewerPlugin> plugin);

  QList<std::shared_ptr<IViewerPlugin>> m_plugins;
};

} // namespace Farman

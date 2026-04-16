#pragma once

#include <QWidget>
#include <QStringList>
#include <QtPlugin>

namespace Farman {

class IViewerPlugin {
public:
  virtual ~IViewerPlugin() = default;

  // プラグイン識別情報
  virtual QString     pluginId()   const = 0;  // "text_viewer"
  virtual QString     pluginName() const = 0;  // "テキストビュアー"
  virtual int         priority()   const { return 0; }  // 高いほど優先

  // 対応ファイルの宣言
  virtual QStringList supportedExtensions() const = 0;  // {"txt","log","cpp"}
  virtual QStringList supportedMimeTypes()  const = 0;  // {"text/plain"}

  // このファイルを開けるか（より細かい判定が必要な場合にoverride）
  virtual bool canHandle(const QString& filePath) const;

  // ビュアーウィジェット生成（呼び出し元がownershipを持つ）
  virtual QWidget* createViewer(
    const QString& filePath,
    QWidget*       parent = nullptr
  ) = 0;
};

} // namespace Farman

Q_DECLARE_INTERFACE(Farman::IViewerPlugin, "com.farman.IViewerPlugin/1.0")

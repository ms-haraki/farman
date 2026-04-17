#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QList>

namespace Farman {

class KeyBindingManager : public QObject {
  Q_OBJECT

public:
  static KeyBindingManager& instance();

  // キー → コマンドID のマッピング
  void    bind(const QKeySequence& key, const QString& commandId);
  void    unbind(const QKeySequence& key);
  QString commandFor(const QKeySequence& key) const;
  bool    isBound(const QKeySequence& key) const;

  // コマンドに紐付けられたすべてのキーを取得
  QList<QKeySequence> keysForCommand(const QString& commandId) const;

  // デフォルトバインディングをロード
  void loadDefaults();

  // Settings から読み書き (Settings クラスを通じて JSON で保存)
  void loadFromSettings();
  void saveToSettings() const;

  // 全バインディング（設定画面に表示するため）
  QMap<QKeySequence, QString> allBindings() const;

  // すべてのバインディングをクリア
  void clearAllBindings();

signals:
  void bindingsChanged();

private:
  explicit KeyBindingManager(QObject* parent = nullptr);
  QMap<QKeySequence, QString> m_bindings;
};

} // namespace Farman

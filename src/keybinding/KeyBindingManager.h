#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>

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

  // デフォルトバインディングをロード
  void loadDefaults();

  // QSettings から読み書き
  void loadFromSettings();
  void saveToSettings() const;

  // 全バインディング（設定画面に表示するため）
  QMap<QKeySequence, QString> allBindings() const;

private:
  explicit KeyBindingManager(QObject* parent = nullptr);
  QMap<QKeySequence, QString> m_bindings;
};

} // namespace Farman

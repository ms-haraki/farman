#include "KeyBindingManager.h"

namespace Farman {

KeyBindingManager& KeyBindingManager::instance() {
  static KeyBindingManager instance;
  return instance;
}

KeyBindingManager::KeyBindingManager(QObject* parent) : QObject(parent) {
}

void KeyBindingManager::bind(const QKeySequence& key, const QString& commandId) {
  // TODO: 実装
}

void KeyBindingManager::unbind(const QKeySequence& key) {
  // TODO: 実装
}

QString KeyBindingManager::commandFor(const QKeySequence& key) const {
  // TODO: 実装
  return QString();
}

bool KeyBindingManager::isBound(const QKeySequence& key) const {
  // TODO: 実装
  return false;
}

void KeyBindingManager::loadDefaults() {
  // TODO: 実装
}

void KeyBindingManager::loadFromSettings() {
  // TODO: 実装
}

void KeyBindingManager::saveToSettings() const {
  // TODO: 実装
}

QMap<QKeySequence, QString> KeyBindingManager::allBindings() const {
  // TODO: 実装
  return {};
}

} // namespace Farman

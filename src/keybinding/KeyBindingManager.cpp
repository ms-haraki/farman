#include "KeyBindingManager.h"
#include "settings/Settings.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

namespace Farman {

KeyBindingManager& KeyBindingManager::instance() {
  static KeyBindingManager instance;
  return instance;
}

KeyBindingManager::KeyBindingManager(QObject* parent) : QObject(parent) {
}

void KeyBindingManager::bind(const QKeySequence& key, const QString& commandId) {
  if (key.isEmpty()) {
    qWarning() << "KeyBindingManager::bind: empty key sequence";
    return;
  }

  if (commandId.isEmpty()) {
    qWarning() << "KeyBindingManager::bind: empty command id";
    return;
  }

  // Check for conflicts
  if (m_bindings.contains(key) && m_bindings.value(key) != commandId) {
    qWarning() << "KeyBindingManager::bind: key already bound to"
               << m_bindings.value(key) << ", rebinding to" << commandId;
  }

  m_bindings.insert(key, commandId);
  emit bindingsChanged();
}

void KeyBindingManager::unbind(const QKeySequence& key) {
  if (m_bindings.remove(key) > 0) {
    emit bindingsChanged();
  }
}

QString KeyBindingManager::commandFor(const QKeySequence& key) const {
  return m_bindings.value(key, QString());
}

bool KeyBindingManager::isBound(const QKeySequence& key) const {
  return m_bindings.contains(key);
}

QList<QKeySequence> KeyBindingManager::keysForCommand(const QString& commandId) const {
  QList<QKeySequence> result;

  for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
    if (it.value() == commandId) {
      result.append(it.key());
    }
  }

  return result;
}

void KeyBindingManager::loadDefaults() {
  clearAllBindings();

  // Navigation
  bind(QKeySequence(Qt::Key_Up),        "navigate.up");
  bind(QKeySequence(Qt::Key_Down),      "navigate.down");
  bind(QKeySequence(Qt::Key_Left),      "navigate.left");
  bind(QKeySequence(Qt::Key_Right),     "navigate.right");
  bind(QKeySequence(Qt::Key_Home),      "navigate.home");
  bind(QKeySequence(Qt::Key_End),       "navigate.end");
  bind(QKeySequence(Qt::Key_PageUp),    "navigate.pageup");
  bind(QKeySequence(Qt::Key_PageDown),  "navigate.pagedown");
  bind(QKeySequence(Qt::Key_Return),    "navigate.enter");
  bind(QKeySequence(Qt::Key_Enter),     "navigate.enter");
  bind(QKeySequence(Qt::Key_Backspace), "navigate.parent");

  // Selection
  bind(QKeySequence(Qt::Key_Space),     "select.toggle");
  bind(QKeySequence(Qt::Key_Insert),    "select.toggle_and_down");
  bind(QKeySequence(Qt::Key_Asterisk),  "select.invert");
  bind(QKeySequence(Qt::CTRL | Qt::Key_A), "select.all");

  // Pane switching
  bind(QKeySequence(Qt::Key_Tab),       "pane.switch");
  bind(QKeySequence(Qt::CTRL | Qt::Key_O), "pane.toggle_single");

  // File operations
  bind(QKeySequence(Qt::Key_F5),        "file.copy");
  bind(QKeySequence(Qt::Key_F6),        "file.move");
  bind(QKeySequence(Qt::Key_F8),        "file.delete");
  bind(QKeySequence(Qt::Key_F7),        "file.mkdir");

  // View
  bind(QKeySequence(Qt::Key_F3),        "view.file");

  // Application
  bind(QKeySequence(Qt::Key_F10),       "app.quit");
  bind(QKeySequence(Qt::CTRL | Qt::Key_Q), "app.quit");

  qDebug() << "KeyBindingManager: loaded" << m_bindings.size() << "default bindings";
}

void KeyBindingManager::loadFromSettings() {
  QSettings settings("farman", "farman");

  // Try to load from JSON format in settings
  QString jsonData = settings.value("keybindings/json").toString();

  if (!jsonData.isEmpty()) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    if (!doc.isObject()) {
      qWarning() << "KeyBindingManager::loadFromSettings: invalid JSON format";
      loadDefaults();
      return;
    }

    clearAllBindings();
    QJsonObject root = doc.object();
    QJsonArray bindings = root.value("bindings").toArray();

    for (const QJsonValue& val : bindings) {
      if (!val.isObject()) continue;

      QJsonObject binding = val.toObject();
      QString keyStr = binding.value("key").toString();
      QString commandId = binding.value("command").toString();

      if (keyStr.isEmpty() || commandId.isEmpty()) continue;

      QKeySequence key(keyStr);
      if (!key.isEmpty()) {
        m_bindings.insert(key, commandId);
      }
    }

    qDebug() << "KeyBindingManager: loaded" << m_bindings.size() << "bindings from settings";
  } else {
    // No saved bindings, load defaults
    qDebug() << "KeyBindingManager: no saved bindings, loading defaults";
    loadDefaults();
  }

  emit bindingsChanged();
}

void KeyBindingManager::saveToSettings() const {
  QJsonArray bindings;

  for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
    QJsonObject binding;
    binding["key"] = it.key().toString();
    binding["command"] = it.value();
    bindings.append(binding);
  }

  QJsonObject root;
  root["bindings"] = bindings;
  root["version"] = 1;

  QJsonDocument doc(root);
  QString jsonData = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));

  QSettings settings("farman", "farman");
  settings.setValue("keybindings/json", jsonData);
  settings.sync();

  qDebug() << "KeyBindingManager: saved" << m_bindings.size() << "bindings to settings";
}

QMap<QKeySequence, QString> KeyBindingManager::allBindings() const {
  return m_bindings;
}

void KeyBindingManager::clearAllBindings() {
  if (!m_bindings.isEmpty()) {
    m_bindings.clear();
    emit bindingsChanged();
  }
}

} // namespace Farman

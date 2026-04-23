#include "KeyBindingManager.h"
#include "settings/Settings.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QSet>

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

namespace {

QList<QPair<QKeySequence, QString>> defaultBindingList() {
  return {
    // Navigation
    { QKeySequence(Qt::Key_Up),        "navigate.up"        },
    { QKeySequence(Qt::Key_Down),      "navigate.down"      },
    { QKeySequence(Qt::Key_Left),      "navigate.left"      },
    { QKeySequence(Qt::Key_Right),     "navigate.right"     },
    { QKeySequence(Qt::Key_Home),      "navigate.home"      },
    { QKeySequence(Qt::Key_End),       "navigate.end"       },
    { QKeySequence(Qt::Key_PageUp),    "navigate.pageup"    },
    { QKeySequence(Qt::Key_PageDown),  "navigate.pagedown"  },
    { QKeySequence(Qt::Key_Return),    "navigate.enter"     },
    { QKeySequence(Qt::Key_Enter),     "navigate.enter"     },
    { QKeySequence(Qt::Key_Backspace), "navigate.parent"    },

    // Selection
    { QKeySequence(Qt::Key_Space),           "select.toggle"          },
    { QKeySequence(Qt::Key_Insert),          "select.toggle_and_down" },
    { QKeySequence(Qt::Key_Asterisk),        "select.invert"          },
    { QKeySequence(Qt::CTRL | Qt::Key_A),    "select.all"             },

    // Pane
    { QKeySequence(Qt::Key_Tab),             "pane.switch"        },
    { QKeySequence(Qt::CTRL | Qt::Key_O),    "pane.toggle_single" },
    { QKeySequence(Qt::Key_F4),              "pane.sort_filter"   },

    // File operations
    { QKeySequence(Qt::Key_F5), "file.copy"   },
    { QKeySequence(Qt::Key_F6), "file.move"   },
    { QKeySequence(Qt::Key_F7), "file.mkdir"  },
    { QKeySequence(Qt::Key_F8), "file.delete" },

    // View
    { QKeySequence(Qt::Key_F3), "view.file" },

    // Bookmark
    { QKeySequence(Qt::Key_B),            "bookmark.toggle" },
    { QKeySequence(Qt::CTRL | Qt::Key_B), "bookmark.list"   },

    // Application
    { QKeySequence(Qt::Key_F9),           "app.settings" },
    { QKeySequence(Qt::Key_F10),          "app.quit"     },
    { QKeySequence(Qt::CTRL | Qt::Key_Q), "app.quit"     },
  };
}

} // anonymous namespace

void KeyBindingManager::loadDefaults() {
  clearAllBindings();
  for (const auto& [key, command] : defaultBindingList()) {
    m_bindings.insert(key, command);
  }
  emit bindingsChanged();
  qDebug() << "KeyBindingManager: loaded" << m_bindings.size() << "default bindings";
}

void KeyBindingManager::loadFromSettings() {
  QSettings settings("farman", "farman");

  QString jsonData = settings.value("keybindings/json").toString();

  if (jsonData.isEmpty()) {
    qDebug() << "KeyBindingManager: no saved bindings, loading defaults";
    loadDefaults();
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
  if (!doc.isObject()) {
    qWarning() << "KeyBindingManager::loadFromSettings: invalid JSON format";
    loadDefaults();
    return;
  }

  clearAllBindings();

  QJsonObject root = doc.object();
  QJsonArray bindings = root.value("bindings").toArray();

  QSet<QString> savedCommands;
  for (const QJsonValue& val : bindings) {
    if (!val.isObject()) continue;

    QJsonObject binding = val.toObject();
    QString keyStr = binding.value("key").toString();
    QString commandId = binding.value("command").toString();

    if (keyStr.isEmpty() || commandId.isEmpty()) continue;

    QKeySequence key(keyStr);
    if (!key.isEmpty()) {
      m_bindings.insert(key, commandId);
      savedCommands.insert(commandId);
    }
  }

  // 保存データに含まれない新規コマンドについてはデフォルトを補完する。
  // これにより、アプリアップデートで追加されたコマンドが既存ユーザー環境でも
  // 動作する。保存済みキーと衝突する場合は上書きせずスキップ。
  int merged = 0;
  for (const auto& [key, command] : defaultBindingList()) {
    if (savedCommands.contains(command)) continue;
    if (m_bindings.contains(key))        continue;
    m_bindings.insert(key, command);
    ++merged;
  }

  qDebug() << "KeyBindingManager: loaded" << m_bindings.size()
           << "bindings from settings ("
           << merged << "merged from defaults)";

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

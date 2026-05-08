#include "PresetIO.h"
#include "keybinding/KeyBindingManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

namespace Farman {
namespace PresetIO {

namespace {

constexpr int kSchemaVersion = 1;

// 共通: ファイルを読んで JSON オブジェクトを取り出す
Result readJsonFile(const QString& path, QJsonObject& out) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return Result::failure(QObject::tr("Cannot open file: %1").arg(file.errorString()));
  }
  const QByteArray bytes = file.readAll();
  file.close();
  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
  if (err.error != QJsonParseError::NoError) {
    return Result::failure(QObject::tr("Invalid JSON: %1").arg(err.errorString()));
  }
  if (!doc.isObject()) {
    return Result::failure(QObject::tr("JSON root must be an object"));
  }
  out = doc.object();
  return Result::success();
}

Result writeJsonFile(const QString& path, const QJsonObject& obj) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return Result::failure(QObject::tr("Cannot open file for writing: %1")
                             .arg(file.errorString()));
  }
  const QJsonDocument doc(obj);
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();
  return Result::success();
}

// 共通: 期待する type / version をチェック
Result checkHeader(const QJsonObject& obj, const QString& expectedType) {
  const QString type = obj.value(QStringLiteral("type")).toString();
  if (type != expectedType) {
    return Result::failure(QObject::tr("Wrong file type: expected '%1', got '%2'")
                             .arg(expectedType, type));
  }
  const int version = obj.value(QStringLiteral("version")).toInt(-1);
  if (version <= 0) {
    return Result::failure(QObject::tr("Missing or invalid version field"));
  }
  if (version > kSchemaVersion) {
    return Result::failure(QObject::tr("File version %1 is newer than supported version %2")
                             .arg(version).arg(kSchemaVersion));
  }
  return Result::success();
}

// id 衝突回避 (Append インポート時用)。`name` の末尾に `(2)` `(3)` を足す。
QString uniqueId(const QString& base, const QSet<QString>& used) {
  if (!used.contains(base)) return base;
  for (int n = 2; n < 10000; ++n) {
    const QString candidate = QStringLiteral("%1-%2").arg(base).arg(n);
    if (!used.contains(candidate)) return candidate;
  }
  return base;  // give up (extreme edge case)
}

// 同梱プリセット: 1 ディレクトリを QDir で列挙して BundledPreset を生成
QList<BundledPreset> listBundledIn(const QString& subdir) {
  QList<BundledPreset> result;
  const QString prefix = QStringLiteral(":/presets/%1").arg(subdir);
  QDir dir(prefix);
  if (!dir.exists()) return result;
  const auto entries = dir.entryInfoList(QStringList() << QStringLiteral("*.json"),
                                         QDir::Files, QDir::Name);
  for (const QFileInfo& fi : entries) {
    BundledPreset p;
    p.id           = fi.baseName();  // "default" / "total-commander"
    p.resourcePath = fi.absoluteFilePath();
    // `name` フィールドが JSON 内にあれば優先、無ければ id を整形
    QJsonObject obj;
    if (loadJsonFromResource(p.resourcePath, obj).ok) {
      p.displayName = obj.value(QStringLiteral("name")).toString();
    }
    if (p.displayName.isEmpty()) {
      // "total-commander" → "Total Commander"
      QString s = p.id;
      s.replace(QLatin1Char('-'), QLatin1Char(' '));
      s.replace(QLatin1Char('_'), QLatin1Char(' '));
      if (!s.isEmpty()) s[0] = s[0].toUpper();
      p.displayName = s;
    }
    result.append(p);
  }
  return result;
}

} // namespace

// ── キーバインド ─────────────────────────────────

QJsonObject keybindingsToJson(const QString& name) {
  QJsonObject root;
  root[QStringLiteral("version")] = kSchemaVersion;
  root[QStringLiteral("type")]    = QStringLiteral("farman.keybindings");
  if (!name.isEmpty()) root[QStringLiteral("name")] = name;

  // 内部の per-key 形式 → per-command 形式へ正規化
  // {C → file.copy, Ctrl+C → file.copy} を {file.copy: [C, Ctrl+C]} にする
  QMap<QString, QStringList> byCommand;
  const auto allBindings = KeyBindingManager::instance().allBindings();
  for (auto it = allBindings.begin(); it != allBindings.end(); ++it) {
    byCommand[it.value()].append(it.key().toString());
  }

  QJsonArray bindings;
  for (auto it = byCommand.begin(); it != byCommand.end(); ++it) {
    QJsonObject entry;
    entry[QStringLiteral("command")] = it.key();
    QJsonArray keys;
    for (const QString& k : it.value()) keys.append(k);
    entry[QStringLiteral("keys")] = keys;
    bindings.append(entry);
  }
  root[QStringLiteral("bindings")] = bindings;
  return root;
}

Result applyKeybindingsFromJson(const QJsonObject& obj) {
  if (Result r = checkHeader(obj, QStringLiteral("farman.keybindings")); !r.ok) return r;
  if (!obj.value(QStringLiteral("bindings")).isArray()) {
    return Result::failure(QObject::tr("Missing 'bindings' array"));
  }

  // 適用は全置換: clearAllBindings してから KeyBindingManager::bind() で追加
  auto& mgr = KeyBindingManager::instance();
  mgr.clearAllBindings();
  const QJsonArray bindings = obj.value(QStringLiteral("bindings")).toArray();
  int totalKeys = 0;
  for (const QJsonValue& v : bindings) {
    if (!v.isObject()) continue;
    const QJsonObject e = v.toObject();
    const QString cmd = e.value(QStringLiteral("command")).toString();
    if (cmd.isEmpty()) continue;
    const QJsonArray keys = e.value(QStringLiteral("keys")).toArray();
    for (const QJsonValue& kv : keys) {
      const QString s = kv.toString();
      if (s.isEmpty()) continue;
      const QKeySequence seq(s);
      if (seq.isEmpty()) continue;
      mgr.bind(seq, cmd);
      ++totalKeys;
    }
  }
  mgr.saveToSettings();
  return totalKeys > 0
           ? Result::success()
           : Result::failure(QObject::tr("No valid bindings were applied"));
}

Result exportKeybindingsToFile(const QString& path, const QString& name) {
  return writeJsonFile(path, keybindingsToJson(name));
}

Result importKeybindingsFromFile(const QString& path) {
  QJsonObject obj;
  if (Result r = readJsonFile(path, obj); !r.ok) return r;
  return applyKeybindingsFromJson(obj);
}

// ── テーマ ───────────────────────────────────────

QJsonObject themeToJson(const ThemeData& d) {
  QJsonObject root;
  root[QStringLiteral("version")] = kSchemaVersion;
  root[QStringLiteral("type")]    = QStringLiteral("farman.theme");
  if (!d.name.isEmpty()) root[QStringLiteral("name")] = d.name;
  if (d.light) root[QStringLiteral("light")] = colorSchemeToJson(*d.light);
  if (d.dark)  root[QStringLiteral("dark")]  = colorSchemeToJson(*d.dark);
  return root;
}

Result themeFromJson(const QJsonObject& obj, ThemeData& out) {
  if (Result r = checkHeader(obj, QStringLiteral("farman.theme")); !r.ok) return r;
  out.name = obj.value(QStringLiteral("name")).toString();

  // 受け入れる形式は { light: {...} } / { dark: {...} } / 両方を含むもの。
  // 両方欠ければエラー。
  if (obj.contains(QStringLiteral("light")) && obj.value(QStringLiteral("light")).isObject()) {
    ColorScheme s = defaultLightScheme();
    colorSchemeFromJson(obj.value(QStringLiteral("light")).toObject(), s);
    out.light = s;
  }
  if (obj.contains(QStringLiteral("dark")) && obj.value(QStringLiteral("dark")).isObject()) {
    ColorScheme s = defaultDarkScheme();
    colorSchemeFromJson(obj.value(QStringLiteral("dark")).toObject(), s);
    out.dark = s;
  }
  if (!out.light && !out.dark) {
    return Result::failure(QObject::tr("Theme file must contain at least one of 'light' or 'dark'"));
  }
  return Result::success();
}

Result exportThemeToFile(const QString& path, const ThemeData& d) {
  if (!d.light && !d.dark) {
    return Result::failure(QObject::tr("Nothing to export (both schemes are empty)"));
  }
  return writeJsonFile(path, themeToJson(d));
}

Result importThemeFromFile(const QString& path, ThemeData& out) {
  QJsonObject obj;
  if (Result r = readJsonFile(path, obj); !r.ok) return r;
  return themeFromJson(obj, out);
}

// ── ユーザコマンド ───────────────────────────────

QJsonObject userCommandsToJson(const QList<UserCommand>& cmds) {
  QJsonObject root;
  root[QStringLiteral("version")] = kSchemaVersion;
  root[QStringLiteral("type")]    = QStringLiteral("farman.commands");
  QJsonArray arr;
  for (const UserCommand& c : cmds) arr.append(userCommandToJson(c));
  root[QStringLiteral("commands")] = arr;
  return root;
}

Result userCommandsFromJson(const QJsonObject& obj, QList<UserCommand>& out) {
  if (Result r = checkHeader(obj, QStringLiteral("farman.commands")); !r.ok) return r;
  if (!obj.value(QStringLiteral("commands")).isArray()) {
    return Result::failure(QObject::tr("Missing 'commands' array"));
  }
  out.clear();
  const QJsonArray arr = obj.value(QStringLiteral("commands")).toArray();
  for (const QJsonValue& v : arr) {
    if (!v.isObject()) continue;
    UserCommand c = userCommandFromJson(v.toObject());
    if (c.id.isEmpty()) continue;
    out.append(c);
  }
  if (out.isEmpty()) {
    return Result::failure(QObject::tr("No valid commands were imported"));
  }
  return Result::success();
}

Result exportUserCommandsToFile(const QString& path, const QList<UserCommand>& cmds) {
  return writeJsonFile(path, userCommandsToJson(cmds));
}

Result importUserCommandsFromFile(const QString& path, QList<UserCommand>& out) {
  QJsonObject obj;
  if (Result r = readJsonFile(path, obj); !r.ok) return r;
  return userCommandsFromJson(obj, out);
}

// ── 同梱プリセット ───────────────────────────────

QList<BundledPreset> listBundledKeybindingPresets() {
  return listBundledIn(QStringLiteral("keybindings"));
}

QList<BundledPreset> listBundledThemePresets() {
  return listBundledIn(QStringLiteral("themes"));
}

Result loadJsonFromResource(const QString& resourcePath, QJsonObject& out) {
  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return Result::failure(QObject::tr("Cannot open bundled preset: %1").arg(resourcePath));
  }
  const QByteArray bytes = file.readAll();
  file.close();
  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
  if (err.error != QJsonParseError::NoError) {
    return Result::failure(QObject::tr("Invalid JSON in %1: %2")
                             .arg(resourcePath, err.errorString()));
  }
  if (!doc.isObject()) {
    return Result::failure(QObject::tr("JSON root must be an object: %1").arg(resourcePath));
  }
  out = doc.object();
  return Result::success();
}

// uniqueId は将来 Append インポート実装時に CmdMgr 側で利用予定 (Phase 6)。
// 現状参照なしの警告を避けるためここで dummy 参照する。
[[maybe_unused]] static auto _silenceUnusedUniqueId = [] {
  (void)&uniqueId;
  return 0;
}();

} // namespace PresetIO
} // namespace Farman

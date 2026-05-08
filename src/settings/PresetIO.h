#pragma once

#include "ColorScheme.h"
#include "core/UserCommand.h"
#include <QJsonObject>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QString>
#include <optional>

namespace Farman {

// プリセット / インポート・エクスポート用の JSON シリアライズ層。
//
// 取扱う 3 ドメイン:
//  1. キーバインド  (`type: "farman.keybindings"`)
//  2. テーマ        (`type: "farman.theme"` — Light / Dark どちらか or 両方)
//  3. ユーザコマンド (`type: "farman.commands"`)
//
// 各 JSON ファイルは `version: 1` と `type` フィールドを必ず持つ。
// バージョン非互換 / 型不一致は import 時のエラーとして扱う。
namespace PresetIO {

// 共通結果型。ok=true なら error は空。失敗時は人間可読な英語メッセージ。
struct Result {
  bool    ok = false;
  QString error;
  static Result success() { return { true, {} }; }
  static Result failure(const QString& msg) { return { false, msg }; }
};

// ── キーバインド ─────────────────────────────────
//
// 保存形式 (SPEC.md L1546 と一致):
//   {
//     "version": 1,
//     "type":    "farman.keybindings",
//     "name":    "Total Commander",     // 任意
//     "bindings": [
//       { "command": "file.copy", "keys": ["F5", "C", "Ctrl+C"] },
//       ...
//     ]
//   }
//
// 注意: 内部の `keybindings/json` (KeyBindingManager 永続化) は
//       「キー 1 個 → コマンド」の per-key 形式だが、エクスポートは
//       「コマンド 1 個 → キー集合」の per-command 形式に正規化する。
//       人間に読みやすく、プリセット同梱もしやすいため。

// 現在 KeyBindingManager に登録されているバインドを per-command 形式へ。
QJsonObject keybindingsToJson(const QString& name = {});

// JSON → KeyBindingManager に適用 (clearAllBindings → 全置換)。
// 入力: per-command 形式 (上記)。Replace 動作のみ。
Result      applyKeybindingsFromJson(const QJsonObject& obj);

Result      exportKeybindingsToFile(const QString& path, const QString& name = {});
Result      importKeybindingsFromFile(const QString& path);

// ── テーマ ───────────────────────────────────────
//
// 保存形式:
//   {
//     "version": 1,
//     "type":    "farman.theme",
//     "name":    "Solarized",            // 任意
//     "light":   { /* ColorScheme */ },  // どちらか or 両方
//     "dark":    { /* ColorScheme */ }
//   }
struct ThemeData {
  QString name;
  std::optional<ColorScheme> light;
  std::optional<ColorScheme> dark;
};

QJsonObject themeToJson(const ThemeData& d);
Result      themeFromJson(const QJsonObject& obj, ThemeData& out);

Result      exportThemeToFile(const QString& path, const ThemeData& d);
Result      importThemeFromFile(const QString& path, ThemeData& out);

// ── ユーザコマンド ───────────────────────────────
//
// 保存形式:
//   {
//     "version": 1,
//     "type":    "farman.commands",
//     "commands": [ /* userCommandToJson(...) の配列 */ ]
//   }
enum class CommandImportMode {
  Replace,   // 既存をすべて捨てて入れ替え
  Append     // 既存末尾に追記 (ID 衝突は数字接尾辞でリネーム)
};

QJsonObject userCommandsToJson(const QList<UserCommand>& cmds);
Result      userCommandsFromJson(const QJsonObject& obj, QList<UserCommand>& out);

Result      exportUserCommandsToFile(const QString& path, const QList<UserCommand>& cmds);
Result      importUserCommandsFromFile(const QString& path, QList<UserCommand>& out);

// ── 同梱プリセット列挙 ───────────────────────────
//
// `:/presets/<kind>/*.json` を Qt リソースから列挙する。
// Phase 4/5 で実 JSON を qrc に追加するまでは空リストを返す。
struct BundledPreset {
  QString id;            // ファイル名から拡張子を除いたもの (`total-commander`)
  QString displayName;   // JSON 内 `name` フィールド or id を整形したもの
  QString resourcePath;  // `:/presets/keybindings/total-commander.json`
};
QList<BundledPreset> listBundledKeybindingPresets();
QList<BundledPreset> listBundledThemePresets();

// 内部利用 (バンドルプリセット共通): リソースから JSON を読み込む。
Result loadJsonFromResource(const QString& resourcePath, QJsonObject& out);

} // namespace PresetIO

} // namespace Farman

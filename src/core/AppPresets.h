#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace Farman {

// 「インストール済の既知アプリ」を表す 1 エントリ。
// AppPresets はこの構造体のリストを返し、ExternalAppsTab はそれを
// プリセットコンボボックスに並べる。各プリセットを選択すると Program /
// Arguments / Working Dir が自動で埋まる。
struct AppPreset {
  QString id;            // "iterm2", "vscode-cli" 等の安定 ID
  QString displayName;   // "iTerm2", "Visual Studio Code (code CLI)"
  QString kind;          // "terminal" / "editor"

  QString     program;
  QStringList argsTemplate;
  QString     workingDirTemplate;  // 通常空 (= active pane の cwd)
};

// kind = "terminal" / "editor" を渡し、その種別の **インストール済**
// プリセット一覧を返す。順序は detection list の上から (= ユーザーが
// 慣れていそうな順 / OS 標準を上位に置く)。
//
// 検出方法はプラットフォームによって異なる:
//   - macOS:  /Applications, ~/Applications, /System/Applications/Utilities の
//             .app バンドル + CLI ラッパ (code / subl 等) を PATH 上で確認
//   - Linux:  QStandardPaths::findExecutable() で PATH 上のバイナリを確認
//   - Windows: 同上 + 既知の Program Files パスを補助的にチェック
QList<AppPreset> detectInstalledPresets(const QString& kind);

// 現在の Program / Arguments が、検出済みプリセットのいずれかと **完全一致**
// すればその id を返す。一致しなければ空文字。ロード時に「コンボの初期選択」を
// 決めるのに使う。workingDirTemplate は UI から削除したため比較対象に含めない。
QString matchPresetId(const QList<AppPreset>& presets,
                      const QString& program,
                      const QStringList& argsTemplate);

} // namespace Farman

#include "AppPresets.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace Farman {

namespace {

// ── macOS: .app バンドル検索 ──
#ifdef Q_OS_MAC
// 与えられたバンドル名 ("iTerm.app") が macOS の標準的な配置パスのいずれかに
// 存在するかを調べる。`/Applications` `~/Applications` `/System/Applications/Utilities`
// (Terminal.app の置き場) を順に確認する。
bool appBundleExists(const QString& bundleName) {
  const QStringList prefixes = {
    QStringLiteral("/Applications"),
    QDir::homePath() + QStringLiteral("/Applications"),
    QStringLiteral("/System/Applications/Utilities"),
    QStringLiteral("/System/Applications"),
  };
  for (const QString& prefix : prefixes) {
    if (QFileInfo::exists(prefix + QLatin1Char('/') + bundleName)) {
      return true;
    }
  }
  return false;
}
#endif

// PATH 上にバイナリがあるかどうか。Linux / Windows の検出と、macOS の
// CLI ラッパ (code / subl / cursor) 検出に使う。
bool inPath(const QString& binary) {
  return !QStandardPaths::findExecutable(binary).isEmpty();
}

// macOS で `open -a <App>` 経由でターミナルアプリを起動するためのプリセット。
#ifdef Q_OS_MAC
AppPreset openA(const QString& id, const QString& display, const QString& kind,
                const QString& appName, const QString& targetPlaceholder) {
  AppPreset p;
  p.id           = id;
  p.displayName  = display;
  p.kind         = kind;
  p.program      = QStringLiteral("/usr/bin/open");
  p.argsTemplate = { QStringLiteral("-a"), appName, targetPlaceholder };
  return p;
}
#endif

// ターミナル / エディタ共通: CLI バイナリ + 引数のシンプルなプリセット。
AppPreset cliPreset(const QString& id, const QString& display, const QString& kind,
                    const QString& program, const QStringList& args) {
  AppPreset p;
  p.id           = id;
  p.displayName  = display;
  p.kind         = kind;
  p.program      = program;
  p.argsTemplate = args;
  return p;
}

// ── ターミナル一覧 (検出付き) ──
QList<AppPreset> detectTerminals() {
  QList<AppPreset> out;

#if defined(Q_OS_MAC)
  // OS 標準を最上位に、その下によく使われる人気アプリを並べる。
  if (appBundleExists(QStringLiteral("Terminal.app"))) {
    out.append(openA(QStringLiteral("terminal-app"), QStringLiteral("Terminal"),
                     QStringLiteral("terminal"),
                     QStringLiteral("Terminal"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("iTerm.app"))) {
    out.append(openA(QStringLiteral("iterm2"), QStringLiteral("iTerm2"),
                     QStringLiteral("terminal"),
                     QStringLiteral("iTerm"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("Warp.app"))) {
    out.append(openA(QStringLiteral("warp"), QStringLiteral("Warp"),
                     QStringLiteral("terminal"),
                     QStringLiteral("Warp"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("Ghostty.app"))) {
    out.append(openA(QStringLiteral("ghostty"), QStringLiteral("Ghostty"),
                     QStringLiteral("terminal"),
                     QStringLiteral("Ghostty"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("WezTerm.app"))) {
    out.append(openA(QStringLiteral("wezterm-app"), QStringLiteral("WezTerm"),
                     QStringLiteral("terminal"),
                     QStringLiteral("WezTerm"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("Alacritty.app"))) {
    out.append(openA(QStringLiteral("alacritty-app"), QStringLiteral("Alacritty"),
                     QStringLiteral("terminal"),
                     QStringLiteral("Alacritty"), QStringLiteral("{dir}")));
  }
  if (appBundleExists(QStringLiteral("kitty.app"))) {
    out.append(openA(QStringLiteral("kitty-app"), QStringLiteral("kitty"),
                     QStringLiteral("terminal"),
                     QStringLiteral("kitty"), QStringLiteral("{dir}")));
  }
#elif defined(Q_OS_WIN)
  if (inPath(QStringLiteral("wt"))) {
    out.append(cliPreset(QStringLiteral("windows-terminal"),
                         QStringLiteral("Windows Terminal"),
                         QStringLiteral("terminal"),
                         QStringLiteral("wt"),
                         { QStringLiteral("-d"), QStringLiteral("{dir}") }));
  }
  // cmd.exe は Windows なら必ずある (基本的に "always present")。検出は不要。
  out.append(cliPreset(QStringLiteral("cmd"), QStringLiteral("Command Prompt"),
                       QStringLiteral("terminal"),
                       QStringLiteral("cmd.exe"),
                       { QStringLiteral("/K"), QStringLiteral("cd"),
                         QStringLiteral("/D"), QStringLiteral("{dir}") }));
  if (inPath(QStringLiteral("pwsh"))) {
    out.append(cliPreset(QStringLiteral("pwsh"), QStringLiteral("PowerShell 7+"),
                         QStringLiteral("terminal"),
                         QStringLiteral("pwsh"),
                         { QStringLiteral("-NoExit"), QStringLiteral("-Command"),
                           QStringLiteral("Set-Location -LiteralPath \"{dir}\"") }));
  }
  if (inPath(QStringLiteral("powershell"))) {
    out.append(cliPreset(QStringLiteral("powershell"),
                         QStringLiteral("Windows PowerShell"),
                         QStringLiteral("terminal"),
                         QStringLiteral("powershell"),
                         { QStringLiteral("-NoExit"), QStringLiteral("-Command"),
                           QStringLiteral("Set-Location -LiteralPath \"{dir}\"") }));
  }
#else
  // Linux: PATH ベース。Ubuntu 24.04 LTS 標準を最上位に置く。
  if (inPath(QStringLiteral("gnome-terminal"))) {
    out.append(cliPreset(QStringLiteral("gnome-terminal"),
                         QStringLiteral("GNOME Terminal"),
                         QStringLiteral("terminal"),
                         QStringLiteral("gnome-terminal"),
                         { QStringLiteral("--working-directory={dir}") }));
  }
  if (inPath(QStringLiteral("konsole"))) {
    out.append(cliPreset(QStringLiteral("konsole"), QStringLiteral("Konsole"),
                         QStringLiteral("terminal"),
                         QStringLiteral("konsole"),
                         { QStringLiteral("--workdir"), QStringLiteral("{dir}") }));
  }
  if (inPath(QStringLiteral("xfce4-terminal"))) {
    out.append(cliPreset(QStringLiteral("xfce4-terminal"),
                         QStringLiteral("Xfce Terminal"),
                         QStringLiteral("terminal"),
                         QStringLiteral("xfce4-terminal"),
                         { QStringLiteral("--working-directory={dir}") }));
  }
  if (inPath(QStringLiteral("tilix"))) {
    out.append(cliPreset(QStringLiteral("tilix"), QStringLiteral("Tilix"),
                         QStringLiteral("terminal"),
                         QStringLiteral("tilix"),
                         { QStringLiteral("--working-directory={dir}") }));
  }
  if (inPath(QStringLiteral("alacritty"))) {
    out.append(cliPreset(QStringLiteral("alacritty"), QStringLiteral("Alacritty"),
                         QStringLiteral("terminal"),
                         QStringLiteral("alacritty"),
                         { QStringLiteral("--working-directory"), QStringLiteral("{dir}") }));
  }
  if (inPath(QStringLiteral("kitty"))) {
    out.append(cliPreset(QStringLiteral("kitty"), QStringLiteral("kitty"),
                         QStringLiteral("terminal"),
                         QStringLiteral("kitty"),
                         { QStringLiteral("--directory={dir}") }));
  }
  if (inPath(QStringLiteral("wezterm"))) {
    out.append(cliPreset(QStringLiteral("wezterm"), QStringLiteral("WezTerm"),
                         QStringLiteral("terminal"),
                         QStringLiteral("wezterm"),
                         { QStringLiteral("start"), QStringLiteral("--cwd"),
                           QStringLiteral("{dir}") }));
  }
  if (inPath(QStringLiteral("xterm"))) {
    // xterm は引数で working-directory を指定できないので workingDirTemplate
    // 側にセットする。シェルを起動して開きっぱなしにするのが普通の使い方。
    AppPreset p = cliPreset(QStringLiteral("xterm"), QStringLiteral("xterm"),
                            QStringLiteral("terminal"),
                            QStringLiteral("xterm"), {});
    p.workingDirTemplate = QStringLiteral("{dir}");
    out.append(p);
  }
#endif

  return out;
}

// ── テキストエディタ一覧 (検出付き) ──
QList<AppPreset> detectEditors() {
  QList<AppPreset> out;

#if defined(Q_OS_MAC)
  // CLI ラッパが使えるエディタは「(<App> CLI)」サフィックスを付けて、より
  // 軽量なバージョンを優先表示する。`code` / `subl` / `cursor` は「Shell
  // Command: Install '<X>' command in PATH」を実行したユーザーのみ存在する。
  // VSCode / Cursor / Sublime Text は「フォルダを workspace として開く」
  // 用途で使われがち。`open -a "<App>" <folder>` は既に起動済みのときに
  // フォルダ引数が無視される (or 別ウィンドウで開かれない) ことがあるので、
  // バンドルに同梱されている CLI スクリプトを直接使う方が確実。
  //
  //   VSCode:  /Applications/Visual Studio Code.app/Contents/Resources/app/bin/code
  //   Cursor:  /Applications/Cursor.app/Contents/Resources/app/bin/cursor
  //   Sublime: /Applications/Sublime Text.app/Contents/SharedSupport/bin/subl
  //
  // PATH 上に `code` / `cursor` / `subl` が入っているユーザー向けには CLI 版を
  // 別エントリ ((<App> CLI)) として上位に出す。
  auto bundleCliPath = [](const QString& bundleName, const QString& relPath) -> QString {
    const QStringList prefixes = {
      QStringLiteral("/Applications"),
      QDir::homePath() + QStringLiteral("/Applications"),
    };
    for (const QString& prefix : prefixes) {
      const QString full = prefix + QLatin1Char('/') + bundleName + QLatin1Char('/') + relPath;
      if (QFileInfo::exists(full)) return full;
    }
    return QString();
  };

  if (inPath(QStringLiteral("code"))) {
    out.append(cliPreset(QStringLiteral("vscode-cli"),
                         QStringLiteral("Visual Studio Code (code CLI)"),
                         QStringLiteral("editor"),
                         QStringLiteral("code"),
                         { QStringLiteral("{file}") }));
  }
  if (const QString p = bundleCliPath(QStringLiteral("Visual Studio Code.app"),
                                       QStringLiteral("Contents/Resources/app/bin/code"));
      !p.isEmpty()) {
    out.append(cliPreset(QStringLiteral("vscode"),
                         QStringLiteral("Visual Studio Code"),
                         QStringLiteral("editor"),
                         p, { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("cursor"))) {
    out.append(cliPreset(QStringLiteral("cursor-cli"),
                         QStringLiteral("Cursor (cursor CLI)"),
                         QStringLiteral("editor"),
                         QStringLiteral("cursor"),
                         { QStringLiteral("{file}") }));
  }
  if (const QString p = bundleCliPath(QStringLiteral("Cursor.app"),
                                       QStringLiteral("Contents/Resources/app/bin/cursor"));
      !p.isEmpty()) {
    out.append(cliPreset(QStringLiteral("cursor"),
                         QStringLiteral("Cursor"),
                         QStringLiteral("editor"),
                         p, { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("subl"))) {
    out.append(cliPreset(QStringLiteral("sublime-cli"),
                         QStringLiteral("Sublime Text (subl CLI)"),
                         QStringLiteral("editor"),
                         QStringLiteral("subl"),
                         { QStringLiteral("{file}") }));
  }
  if (const QString p = bundleCliPath(QStringLiteral("Sublime Text.app"),
                                       QStringLiteral("Contents/SharedSupport/bin/subl"));
      !p.isEmpty()) {
    out.append(cliPreset(QStringLiteral("sublime"),
                         QStringLiteral("Sublime Text"),
                         QStringLiteral("editor"),
                         p, { QStringLiteral("{file}") }));
  }
  if (appBundleExists(QStringLiteral("Zed.app"))) {
    out.append(openA(QStringLiteral("zed"), QStringLiteral("Zed"),
                     QStringLiteral("editor"),
                     QStringLiteral("Zed"), QStringLiteral("{file}")));
  }
  if (appBundleExists(QStringLiteral("BBEdit.app"))) {
    out.append(openA(QStringLiteral("bbedit"), QStringLiteral("BBEdit"),
                     QStringLiteral("editor"),
                     QStringLiteral("BBEdit"), QStringLiteral("{file}")));
  }
  if (appBundleExists(QStringLiteral("TextEdit.app"))) {
    out.append(openA(QStringLiteral("textedit"), QStringLiteral("TextEdit"),
                     QStringLiteral("editor"),
                     QStringLiteral("TextEdit"), QStringLiteral("{file}")));
  }
#elif defined(Q_OS_WIN)
  if (inPath(QStringLiteral("code"))) {
    out.append(cliPreset(QStringLiteral("vscode"),
                         QStringLiteral("Visual Studio Code"),
                         QStringLiteral("editor"),
                         QStringLiteral("code"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("cursor"))) {
    out.append(cliPreset(QStringLiteral("cursor"), QStringLiteral("Cursor"),
                         QStringLiteral("editor"),
                         QStringLiteral("cursor"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("subl"))) {
    out.append(cliPreset(QStringLiteral("sublime"),
                         QStringLiteral("Sublime Text"),
                         QStringLiteral("editor"),
                         QStringLiteral("subl"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("notepad++"))) {
    out.append(cliPreset(QStringLiteral("notepadpp"),
                         QStringLiteral("Notepad++"),
                         QStringLiteral("editor"),
                         QStringLiteral("notepad++"),
                         { QStringLiteral("{file}") }));
  }
  // notepad は Windows 標準で必ず存在する想定。
  out.append(cliPreset(QStringLiteral("notepad"), QStringLiteral("Notepad"),
                       QStringLiteral("editor"),
                       QStringLiteral("notepad"),
                       { QStringLiteral("{file}") }));
#else
  // Linux: PATH 経由の単純な CLI が多い。Ubuntu 24.04 LTS 既定の
  // gnome-text-editor を最上位に。
  if (inPath(QStringLiteral("gnome-text-editor"))) {
    out.append(cliPreset(QStringLiteral("gnome-text-editor"),
                         QStringLiteral("GNOME Text Editor"),
                         QStringLiteral("editor"),
                         QStringLiteral("gnome-text-editor"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("gedit"))) {
    out.append(cliPreset(QStringLiteral("gedit"), QStringLiteral("gedit"),
                         QStringLiteral("editor"),
                         QStringLiteral("gedit"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("kate"))) {
    out.append(cliPreset(QStringLiteral("kate"), QStringLiteral("Kate"),
                         QStringLiteral("editor"),
                         QStringLiteral("kate"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("mousepad"))) {
    out.append(cliPreset(QStringLiteral("mousepad"), QStringLiteral("Mousepad"),
                         QStringLiteral("editor"),
                         QStringLiteral("mousepad"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("code"))) {
    out.append(cliPreset(QStringLiteral("vscode"),
                         QStringLiteral("Visual Studio Code"),
                         QStringLiteral("editor"),
                         QStringLiteral("code"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("cursor"))) {
    out.append(cliPreset(QStringLiteral("cursor"), QStringLiteral("Cursor"),
                         QStringLiteral("editor"),
                         QStringLiteral("cursor"),
                         { QStringLiteral("{file}") }));
  }
  if (inPath(QStringLiteral("subl"))) {
    out.append(cliPreset(QStringLiteral("sublime"),
                         QStringLiteral("Sublime Text"),
                         QStringLiteral("editor"),
                         QStringLiteral("subl"),
                         { QStringLiteral("{file}") }));
  }
#endif

  return out;
}

} // anonymous namespace

QList<AppPreset> detectInstalledPresets(const QString& kind) {
  if (kind == QLatin1String("terminal")) return detectTerminals();
  if (kind == QLatin1String("editor"))   return detectEditors();
  return {};
}

QString matchPresetId(const QList<AppPreset>& presets,
                      const QString& program,
                      const QStringList& argsTemplate,
                      const QString& workingDirTemplate) {
  // 完全一致のみ。1 引数の差 (空白の有無 etc.) で誤マッチしないよう、
  // QString / QStringList の == 比較に依存する。
  for (const AppPreset& p : presets) {
    if (p.program == program
        && p.argsTemplate == argsTemplate
        && p.workingDirTemplate == workingDirTemplate) {
      return p.id;
    }
  }
  return QString();
}

} // namespace Farman

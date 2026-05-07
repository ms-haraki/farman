#pragma once

#include <QString>
#include <QStringList>

namespace Farman {

// 外部アプリ起動時に渡すコンテキスト。
// プレースホルダ展開はこの構造体だけを参照する (FileManagerPanel 等の UI 型に
// 依存しない)。FileManagerPanel 側で値を集めてから渡す。
struct PaneContext {
  QString     activeDir;       // {dir}      アクティブペインの現在ディレクトリ
  QString     otherDir;        // {otherDir} 反対ペインの現在ディレクトリ
  QString     cursorPath;      // {file}     カーソル位置のフルパス (空可)
  QString     cursorName;      // {name}     カーソル位置のファイル名 (拡張子含む)
  QString     cursorExt;       // {ext}      カーソル位置の拡張子 (先頭 `.` 無し)
  QStringList selectedPaths;   // {files}    選択ファイル群 (空なら cursorPath にフォールバック)
};

// 引数テンプレートを展開して、QProcess::startDetached(program, args) に
// 渡せる形に整える。
//
// 展開規則:
// - 引数全体がプレースホルダ単体 ("{files}") のときだけ複数引数に分解する。
//   例: ["edit", "{files}"] + 選択 3 件 → ["edit", "/a", "/b", "/c"]
// - 引数の一部にプレースホルダが混ざる場合 ("out-{name}.log") は文字列結合で
//   展開する (この場合 {files} は最初の 1 件のパスを使う)。
// - 未定義のプレースホルダは空文字に展開する (例: 選択なし & カーソル無しの
//   {file})。空に展開された結果として引数自体が空になっても引数リストには
//   残す (位置を変えない方針)。
QStringList expandArgs(const QStringList& tmpl, const PaneContext& ctx);

// 作業ディレクトリのテンプレート展開。テンプレートが空なら ctx.activeDir を返す。
QString expandWorkingDir(const QString& tmpl, const PaneContext& ctx);

} // namespace Farman

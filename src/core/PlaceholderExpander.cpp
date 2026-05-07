#include "PlaceholderExpander.h"

namespace Farman {

namespace {

// プレースホルダを ctx の値で展開した結果を返す。{files} はここでは
// 最初の 1 件のみを返す (引数内に部分混在するケース用)。
QString expandSingleValue(const QString& key, const PaneContext& ctx) {
  if (key == QLatin1String("dir"))      return ctx.activeDir;
  if (key == QLatin1String("otherDir")) return ctx.otherDir;
  if (key == QLatin1String("file"))     return ctx.cursorPath;
  if (key == QLatin1String("name"))     return ctx.cursorName;
  if (key == QLatin1String("ext"))      return ctx.cursorExt;
  if (key == QLatin1String("files")) {
    if (!ctx.selectedPaths.isEmpty()) return ctx.selectedPaths.first();
    return ctx.cursorPath;
  }
  // 未知のプレースホルダはそのまま残す (ユーザーが誤って `{foo}` と書いたときに
  // 痕跡が残ったほうが気付きやすい)。
  return QStringLiteral("{") + key + QLatin1Char('}');
}

// 単一引数中のプレースホルダを文字列結合で展開する。
// {files} はこの経路では最初の 1 件として展開される (= 単独 {files} 引数とは
// 挙動が違うことに注意)。
QString expandWithinArg(const QString& tmpl, const PaneContext& ctx) {
  QString out;
  out.reserve(tmpl.size());

  int i = 0;
  while (i < tmpl.size()) {
    const QChar c = tmpl.at(i);
    if (c == QLatin1Char('{')) {
      const int close = tmpl.indexOf(QLatin1Char('}'), i + 1);
      if (close > i) {
        const QString key = tmpl.mid(i + 1, close - i - 1);
        out += expandSingleValue(key, ctx);
        i = close + 1;
        continue;
      }
      // 閉じ括弧が無い: そのまま `{` を出して走査続行
    }
    out += c;
    ++i;
  }

  return out;
}

} // anonymous namespace

QStringList expandArgs(const QStringList& tmpl, const PaneContext& ctx) {
  QStringList out;
  out.reserve(tmpl.size());

  for (const QString& a : tmpl) {
    // 引数全体がプレースホルダ単体のときだけ複数引数に分解できる扱い。
    // 例: "{files}" → 選択ファイル群を 1 引数 = 1 要素で展開。
    // "{files}.bak" のように混ざる場合は文字列結合経路に流す。
    if (a == QLatin1String("{files}")) {
      if (!ctx.selectedPaths.isEmpty()) {
        for (const QString& p : ctx.selectedPaths) out.append(p);
      } else if (!ctx.cursorPath.isEmpty()) {
        out.append(ctx.cursorPath);
      }
      // 選択もカーソルも無い場合は何も追加しない (= その引数位置がスキップ)。
      continue;
    }

    out.append(expandWithinArg(a, ctx));
  }

  return out;
}

QString expandWorkingDir(const QString& tmpl, const PaneContext& ctx) {
  if (tmpl.isEmpty()) return ctx.activeDir;
  return expandWithinArg(tmpl, ctx);
}

} // namespace Farman

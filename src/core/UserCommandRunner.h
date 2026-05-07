#pragma once

#include "UserCommand.h"
#include "PlaceholderExpander.h"

namespace Farman {

// 1 件の UserCommand を起動する。
//
// - PlaceholderExpander で引数 / 作業ディレクトリを展開
// - QProcess::startDetached(program, args, workingDir) で外部プロセスを起動
// - 成功 / 失敗を Logger に流す
// - errorOut が非 null の場合、失敗時にユーザー向けメッセージを格納
//
// program が空 / 引数展開後にすべて空になった等の事前エラーも startDetached の
// 戻り値とは別に検知してログ + errorOut に積む。
//
// 戻り値: プロセス起動に成功 (= startDetached が true) なら true。
bool runUserCommand(const UserCommand& cmd,
                    const PaneContext& ctx,
                    QString*           errorOut = nullptr);

} // namespace Farman

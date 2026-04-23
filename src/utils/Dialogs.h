#pragma once

#include <QString>

class QWidget;

namespace Farman {

// Yes / No 2 択の確認ダイアログをキーボードのみで完結できるヘルパ。
// - `Y` / `N` キー: 即応答（macOS のキーボードナビゲーション設定に依存しない）
// - `Enter`: デフォルトボタン（defaultYes=true なら Yes、false なら No）
// - `Escape`: No
// 戻り値: Yes を選んだなら true、No / キャンセルなら false。
bool confirm(QWidget* parent,
             const QString& title,
             const QString& text,
             bool defaultYes = false);

} // namespace Farman

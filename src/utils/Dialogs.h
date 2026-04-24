#pragma once

#include <QString>
#include <Qt>

class QWidget;
class QPushButton;

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

// ボタンにショートカットキー（Alt+key）を設定し、テキスト末尾に
// 「(⌥X)」/「(Alt+X)」のネイティブ表記を添える。Tab でフォーカスを
// 当てられるよう focusPolicy も StrongFocus に明示する。
// 既に同じ表記が末尾にあれば重複付加はしない（再呼び出し安全）。
void applyAltShortcut(QPushButton* btn, Qt::Key key);

} // namespace Farman

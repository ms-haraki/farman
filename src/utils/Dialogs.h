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

// テキスト入力ダイアログ（QInputDialog の置き換え）。
// OK/Cancel にショートカット（Alt+O/Alt+X）を設定し、Tab 順は
// 入力欄 → Cancel → OK（実行系が最後）。
// OK で accepted のとき ok=true、戻り値に入力文字列。
//
// CursorPolicy: ダイアログを開いた直後の入力欄カーソル位置／選択状態。
//  - SelectAll: 既定。defaultValue 全体を選択 (新規入力で上書きしやすい)
//  - BeforeExtension: 拡張子の手前 = 最初に出てくる '.' の直前にカーソル、
//      選択無し ("foo.tar.gz" なら "foo" の直後、"foo.txt" なら "foo" の直後)。
//      リネーム向け。先頭が '.' のドットファイル ('.gitignore' 等) は
//      拡張子として扱わず、その後の `.` を見る。コピー衝突時の
//      OverwriteDialog のリネーム入力欄と同じ挙動。
enum class TextInputCursor {
  SelectAll,
  BeforeExtension,
};

QString inputText(QWidget* parent,
                  const QString& title,
                  const QString& label,
                  const QString& defaultValue = QString(),
                  bool* ok = nullptr,
                  TextInputCursor cursor = TextInputCursor::SelectAll);

} // namespace Farman

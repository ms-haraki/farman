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

// ラベル文字列に Alt+key ショートカットの視覚的ヒントを埋め込む。
// 戻り値はそのまま QLabel / QRadioButton / QPushButton の text に使える。
//   Windows / Linux: 該当文字の前に '&' を挿入 (例: "&Foo")。
//                    Qt が該当文字をアンダーラインで描画し、QPushButton や
//                    QRadioButton では Alt+key で自動アクティベートされる。
//                    該当字が無ければ末尾 "(&F)" 形式にフォールバック
//                    (例: 日本語訳 "コピー" + Qt::Key_C → "コピー (&C)")。
//   macOS:           HIG により '&' mnemonic は表示されないため、末尾に
//                    "(⌥X)" 形式で添える (例: "Foo (⌥F)")。
// 入力中の '&' は一旦除去するため、繰り返し呼び出しても二重付加しない。
QString withAltMnemonic(const QString& text, Qt::Key key);

// ボタンにショートカットキー (Alt+key) を設定し、表記を withAltMnemonic で
// 整える。Tab でフォーカスを当てられるよう focusPolicy も StrongFocus に
// 明示する。再呼び出し安全。
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

#pragma once

#include <QtGlobal>
#include <QString>
#include <QColor>

namespace Farman {

// ペインの種別
enum class PaneType {
  Left = 0,
  Right,
  Count
};

// ソートキー
enum class SortKey {
  None,          // ソートなし（第2キー用）
  Name,
  Size,
  Type,
  LastModified
};

// ディレクトリのソート位置
enum class SortDirsType {
  First,   // ディレクトリを先頭に
  Last,    // ディレクトリを末尾に
  Mixed    // ファイルと混在
};

// ファイルサイズの表示形式
enum class FileSizeFormat {
  Bytes,   // バイト表示
  SI,      // KB / MB / GB (1000進)
  IEC,     // KiB / MiB / GiB (1024進)
  Auto     // 自動選択
};

// 属性フィルタのフラグ
enum class AttrFilter : quint32 {
  None        = 0x00,
  ShowHidden  = 0x01,  // 隠しファイルを表示
  ShowSystem  = 0x02,  // システムファイルを表示 (Windows)
  DirsOnly    = 0x04,  // ディレクトリのみ
  FilesOnly   = 0x08,  // ファイルのみ
};
Q_DECLARE_FLAGS(AttrFilterFlags, AttrFilter)
Q_DECLARE_OPERATORS_FOR_FLAGS(AttrFilterFlags)

// ワーカーの操作種別
enum class WorkerOperation {
  Copy,
  Move,
  Remove
};

// 上書き時の動作モード（操作開始前に選択）
enum class OverwriteMode {
  Ask,            // 都度確認
  AutoOverwrite,  // 自動で上書き
  AutoRename      // 自動でリネーム（"foo (1).txt" など）
};

// 上書き競合発生時のユーザー選択（Ask モードの OverwriteDialog からの応答）
struct OverwriteDecision {
  enum class Action {
    Overwrite,  // そのまま上書き
    Rename,     // 任意のファイル名でリネーム
    Cancel      // 操作全体をキャンセル
  };
  Action  action  = Action::Cancel;
  QString newName;  // Rename 選択時のみ有効（ファイル名のみ、パスを含まない）
};

// ウィンドウサイズの復元モード
enum class WindowSizeMode {
  Default,      // デフォルトサイズ (1200x600)
  LastSession,  // 前回終了時のサイズ
  Custom        // ユーザー指定のサイズ
};

// ウィンドウ位置の復元モード
enum class WindowPositionMode {
  Default,      // デフォルト位置 (画面中央)
  LastSession,  // 前回終了時の位置
  Custom        // ユーザー指定の位置
};

// 初期表示ディレクトリのモード（ペイン別）
enum class InitialPathMode {
  Default,      // ホームディレクトリ
  LastSession,  // 前回終了時のパス
  Custom        // ユーザー指定の固定パス
};

// バイナリビュアーの 16 進ダンプ単位
enum class BinaryViewerUnit {
  Byte1,
  Byte2,
  Byte4,
  Byte8
};

// バイナリビュアーの多バイト単位表示時のエンディアン
enum class BinaryViewerEndian {
  Little,
  Big
};

// ファイル一覧でのカラーリング対象カテゴリ
enum class FileCategory {
  Normal    = 0,  // 通常ファイル
  Hidden    = 1,  // 隠しファイル／ディレクトリ（優先度: 最高）
  Directory = 2,  // 非隠しのディレクトリ
  Count     = 3
};

// ファイルカテゴリごとの表示スタイル
struct CategoryColor {
  QColor foreground;   // 文字色（invalid なら既定を使用）
  QColor background;   // 背景色（invalid なら背景を上書きしない）
  bool   bold = false; // 太字で表示するか
};

} // namespace Farman

#pragma once

#include <QtGlobal>

namespace Farman {

// ペインの種別
enum class PaneType {
  Left = 0,
  Right,
  Count
};

// ソートキー
enum class SortKey {
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

// 上書き確認の結果
enum class OverwriteResult {
  Yes,
  YesAll,
  No,
  NoAll,
  Cancel
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

} // namespace Farman

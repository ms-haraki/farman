#pragma once

#include "types.h"
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QList>
#include <array>

namespace Farman {

// テーマ単位で切り替わる「色とフォントのスナップショット」。
//
// Settings はこれを Light / Dark の 2 セット保持し、`themeMode()` で
// アクティブな側 (= 現在の見た目に反映されている側) を選ぶ。
// Settings 内の m_ フィールド (m_font / m_addressForeground / ...) は
// 「アクティブスキームのワーキングコピー」として常に同期されており、
// 個別の getter/setter は m_ を介して動く。
//
// SPEC.md L1554-1559 の `Theme JSON` の `light` / `dark` ブロック内の
// フィールドはこの構造体と 1:1 に対応する。
//
// 含まれない (= テーマ非依存) 項目:
//  - cursorShape / cursorThickness — UI 要素の構造的選択
//  - useInactivePaneColors          — 機能 ON/OFF のトグル
//  - 各種 extensions / mime / encoding / unit など viewer の挙動設定
struct ColorScheme {
  // ── ファイルリスト ─────────────────────────
  QFont   listFont;       // ファイル一覧
  QFont   addressFont;    // アドレスバー (各ペイン上部)
  int     fileListRowHeight = 0;  // 0 = Auto

  QList<ColorRule>     colorRules;
  // (cat × selected × inactive × inactive-selected) の 4 状態 × 3 カテゴリ
  std::array<CategoryColor, static_cast<size_t>(FileCategory::Count)>
      categoryColors,
      selectedCategoryColors,
      inactiveCategoryColors,
      inactiveSelectedCategoryColors;

  // アドレスバー / カーソル
  QColor  addressForeground;
  QColor  addressBackground;
  QColor  cursorActiveColor;
  QColor  cursorInactiveColor;

  // ── テキストビュアー ───────────────────────
  QFont   textViewerFont;
  QColor  textViewerNormalFg;
  QColor  textViewerNormalBg;
  QColor  textViewerSelectedFg;
  QColor  textViewerSelectedBg;
  QColor  textViewerLineNumberFg;
  QColor  textViewerLineNumberBg;

  // ── 画像ビュアー (透明部分の色) ────────────
  QColor  imageViewerSolidColor;
  QColor  imageViewerCheckerColor1;
  QColor  imageViewerCheckerColor2;

  // ── バイナリビュアー ───────────────────────
  QFont   binaryViewerFont;
  QColor  binaryViewerNormalFg;
  QColor  binaryViewerNormalBg;
  QColor  binaryViewerSelectedFg;
  QColor  binaryViewerSelectedBg;
  QColor  binaryViewerAddressFg;
  QColor  binaryViewerAddressBg;
};

// 出荷時の Light テーマ (= これまでの farman の既定値そのまま)
ColorScheme defaultLightScheme();
// 出荷時の Dark テーマ (Light を反転させたシンプルなダーク)
ColorScheme defaultDarkScheme();

// ── JSON シリアライズ ────────────────────────────
// 設定ファイル `themes.{light,dark}` ブロックや、テーマプリセット
// (`*.json`) のインポート / エクスポートで使用する。
// `from` は欠落キーがあれば `fallback` の値を維持する non-strict ロード。
QJsonObject colorSchemeToJson(const ColorScheme& s);
void        colorSchemeFromJson(const QJsonObject& obj, ColorScheme& fallback);

} // namespace Farman

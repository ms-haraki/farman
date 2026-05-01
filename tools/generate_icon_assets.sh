#!/usr/bin/env bash
# images/icon.{png,svg} から、各プラットフォーム配布物用のアイコンを生成する。
# 入力ソースは以下の優先順位で自動選択する:
#   1. images/icon.png (1024x1024 以上を推奨。PSD からの書き出し想定)
#   2. images/icon.svg (PNG が無ければフォールバック)
# PNG のほうが Photoshop 等で作ったグラデーション・影などをそのまま出せるため
# 通常はこちらを優先。SVG しか無い場合は sips でラスタライズする。
#
#   出力:
#     images/icon.icns          (macOS .app の Contents/Resources/icon.icns 用)
#     images/icon.ico           (Windows 用、ImageMagick が利用可能なら)
#     images/icon-1024.png      (汎用、Linux 等で /usr/share/icons に置く想定)
#     images/icon-512.png       (同上、512x512)
#     images/icon-256.png       (同上、256x256)
#
# 主な依存:
#   - macOS 標準: sips, iconutil (これだけで .icns まで生成できる)
#   - 任意:        magick / convert (ImageMagick) があれば .ico も作る
#
# アイコン素材を更新したら本スクリプトを手で実行して、生成物をリポジトリに
# コミットすること (CMake からは自動実行しない、ビルドのたびに走らせると
# 無駄に時間がかかるため)。

set -euo pipefail

REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PNG_SRC="$REPO_DIR/images/icon.png"
SVG_SRC="$REPO_DIR/images/icon.svg"
OUT_DIR="$REPO_DIR/images"

# 入力ソースを決定 (PNG を優先)
if [[ -f "$PNG_SRC" ]]; then
  SOURCE="$PNG_SRC"
  SOURCE_KIND="PNG"
elif [[ -f "$SVG_SRC" ]]; then
  SOURCE="$SVG_SRC"
  SOURCE_KIND="SVG"
else
  echo "ERROR: Neither $PNG_SRC nor $SVG_SRC found." >&2
  exit 1
fi

if ! command -v sips >/dev/null 2>&1 || ! command -v iconutil >/dev/null 2>&1; then
  echo "ERROR: This script requires sips and iconutil (macOS built-ins)." >&2
  exit 1
fi

# ソースが正方形でない場合は警告 (アイコンは正方形必須)
DIMS="$(sips -g pixelWidth -g pixelHeight "$SOURCE" 2>/dev/null \
        | awk '/pixelWidth/ {w=$2} /pixelHeight/ {h=$2} END {print w"x"h}')"
SRC_W="${DIMS%x*}"
SRC_H="${DIMS#*x}"
if [[ -n "$SRC_W" && -n "$SRC_H" && "$SRC_W" != "$SRC_H" ]]; then
  echo "WARNING: source $SOURCE is ${DIMS} (not square)." >&2
  echo "         icon will be force-scaled to 1024x1024 which distorts the image." >&2
  echo "         Please prepare a square (1024x1024 or larger) source." >&2
fi

echo "==> Rasterising $SOURCE_KIND source ($DIMS) to PNGs"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

ICONSET="$WORK/icon.iconset"
mkdir -p "$ICONSET"

# まず 1024x1024 のマスターを作って、そこから縮小していく。
# sips は SVG / PNG どちらも入力に取れる。
MASTER="$WORK/master.png"
sips -s format png -z 1024 1024 "$SOURCE" --out "$MASTER" >/dev/null

# iconutil の命名規則に合わせて 10 ファイル生成。
declare -a SIZES=(
  "16   icon_16x16.png"
  "32   icon_16x16@2x.png"
  "32   icon_32x32.png"
  "64   icon_32x32@2x.png"
  "128  icon_128x128.png"
  "256  icon_128x128@2x.png"
  "256  icon_256x256.png"
  "512  icon_256x256@2x.png"
  "512  icon_512x512.png"
  "1024 icon_512x512@2x.png"
)
for entry in "${SIZES[@]}"; do
  read -r size name <<< "$entry"
  sips -z "$size" "$size" "$MASTER" --out "$ICONSET/$name" >/dev/null
done

echo "==> Generating icon.icns"
iconutil -c icns "$ICONSET" -o "$OUT_DIR/icon.icns"

echo "==> Generating PNG copies for non-macOS"
for sz in 256 512 1024; do
  sips -z "$sz" "$sz" "$MASTER" --out "$OUT_DIR/icon-$sz.png" >/dev/null
done

# Windows 用 .ico (任意)
if command -v magick >/dev/null 2>&1 || command -v convert >/dev/null 2>&1; then
  echo "==> Generating icon.ico (ImageMagick)"
  TOOL=$(command -v magick || command -v convert)
  # 16/32/48/64/128/256 を 1 つの ICO にパッケージ
  "$TOOL" \
    "$ICONSET/icon_16x16.png" \
    "$ICONSET/icon_32x32.png" \
    "$WORK/master.png[48x48]" 2>/dev/null \
    "$ICONSET/icon_32x32@2x.png" \
    "$ICONSET/icon_128x128.png" \
    "$ICONSET/icon_256x256.png" \
    "$OUT_DIR/icon.ico" || {
      # 一部の引数で失敗したら最小限で再試行
      "$TOOL" "$ICONSET/icon_16x16.png" "$ICONSET/icon_32x32.png" \
              "$ICONSET/icon_128x128.png" "$ICONSET/icon_256x256.png" \
              "$OUT_DIR/icon.ico"
    }
else
  echo "==> Skipping icon.ico (ImageMagick not installed; brew install imagemagick)"
fi

echo
echo "Done."
echo "Generated files in $OUT_DIR/:"
ls -la "$OUT_DIR" | grep -E 'icon\.(icns|ico)|icon-[0-9]+\.png' || true

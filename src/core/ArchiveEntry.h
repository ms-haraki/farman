#pragma once

#include <QString>
#include <QDateTime>

namespace Farman {

// アーカイブ内 1 エントリのメタデータ。POD。
// libarchive の `archive_entry_*` から取り出したものを Qt 型に変換して保持する。
struct ArchiveEntry {
  // アーカイブ相対パス (先頭 '/' なし)。ディレクトリの場合は末尾 '/' なし。
  //   例: "src/main.cpp"  /  "src"  / ルート自体は "" (entries に格納しない)
  QString   pathInArchive;
  QString   name;           // pathInArchive のベース名 ("main.cpp" / "src")
  bool      isDir = false;
  qint64    size = -1;       // 圧縮前サイズ。ディレクトリ / 取得不可は -1
  qint64    compressedSize = -1;  // 圧縮後サイズ。取得不可は -1
  QDateTime mtime;
  QString   linkTarget;     // シンボリックリンクの参照先 (空でなければ symlink)
};

} // namespace Farman

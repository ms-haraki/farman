#pragma once

#include "ArchiveEntry.h"
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <memory>

namespace Farman {

// 1 アーカイブ分のメタデータキャッシュ。
//
// `ArchiveContext::load(archivePath)` で同期的にエントリを列挙し、
// 中身を `entries` に詰めて返す。
// `shared_ptr` で `FileListModel` (および将来は両ペイン間) に保持される。
//
// **読み取り専用**: 構築後は変更しない (FileItem が weak/shared で握っても安全)。
class ArchiveContext {
public:
  // ── 公開フィールド ────────────────────────────
  QString                       archivePath;    // アーカイブ自体の絶対パス
  QHash<QString, ArchiveEntry>  entries;        // pathInArchive (先頭 '/' なし) → entry
  QDateTime                     archiveMtime;   // 元アーカイブの mtime (再ロード判定用)

  // ── ファクトリ ────────────────────────────────
  // libarchive で archivePath を開いて全エントリのメタデータを列挙する。
  // 失敗時は nullptr。
  // **本メソッドは同期 I/O**。UI スレッドから呼ぶ場合は呼び出し側で
  // QtConcurrent + イベントループで包むこと (Phase A は MVP として同期呼び出し)。
  static std::shared_ptr<ArchiveContext> load(const QString& archivePath);

  // ── クエリ ────────────────────────────────────
  // innerDir は "/" (ルート) または "/foo" / "/foo/bar" の形式。
  // その直下にいるエントリ (entries map のうち、parent が innerDir に一致するもの)
  // を返す。順序は entries 走査順 (不定)。
  QList<const ArchiveEntry*> childrenOf(const QString& innerDir) const;

  // innerDir が有効なディレクトリ（あるいは "/" = ルート）かどうか。
  // "/" は常に true。それ以外はエントリ自体が isDir=true で存在するか、
  // または entries の中にそれを prefix とするエントリがあるか。
  bool isValidDirectory(const QString& innerDir) const;

  // ── 1 エントリ展開 (Phase B) ──────────────────
  // archivePath を libarchive で開き直し、entryPath (= ArchiveEntry の
  // pathInArchive、先頭 '/' なし形式) に該当する 1 エントリの内容を destPath
  // に書き出す。destPath の親ディレクトリは必要に応じて作成。
  // 戻り値: 成功 true。libarchive はランダムアクセスできないので、エントリ
  // 発見まで先頭から逐次読みする (大きなアーカイブでは線形時間)。
  bool extractEntryTo(const QString& entryPath, const QString& destPath) const;

private:
  // pathInArchive をディレクトリ正規形に整える内部ヘルパ:
  //   "/" → ""
  //   "/foo" → "foo"
  //   "/foo/" → "foo"
  static QString normalizeDirKey(const QString& innerDir);
};

} // namespace Farman

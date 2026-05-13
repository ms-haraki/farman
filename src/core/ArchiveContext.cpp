#include "ArchiveContext.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <archive.h>
#include <archive_entry.h>

namespace Farman {

namespace {

#ifdef Q_OS_WIN
inline const wchar_t* asWChar(const QString& s) {
  return reinterpret_cast<const wchar_t*>(s.utf16());
}
#endif

// libarchive のエントリパス (アーカイブの内部表現) を取得して
// 「先頭 '/' なし、末尾 '/' なし」のキー文字列に整える。
QString readEntryPath(struct archive_entry* entry) {
  QString path;
#ifdef Q_OS_WIN
  if (const wchar_t* wname = archive_entry_pathname_w(entry)) {
    path = QString::fromWCharArray(wname);
  } else if (const char* uname = archive_entry_pathname_utf8(entry)) {
    path = QString::fromUtf8(uname);
  } else if (const char* name = archive_entry_pathname(entry)) {
    path = QString::fromLocal8Bit(name);
  }
#else
  if (const char* uname = archive_entry_pathname_utf8(entry)) {
    path = QString::fromUtf8(uname);
  } else if (const char* name = archive_entry_pathname(entry)) {
    path = QString::fromUtf8(name);
  }
#endif
  // 末尾 '/' (ディレクトリ表記) を落とす
  while (path.size() > 0 && path.endsWith(QLatin1Char('/'))) {
    path.chop(1);
  }
  // 先頭 '/' (絶対パス入りエントリ) を落とす
  while (path.size() > 0 && path.startsWith(QLatin1Char('/'))) {
    path.remove(0, 1);
  }
  return path;
}

} // namespace

QString ArchiveContext::normalizeDirKey(const QString& innerDir) {
  if (innerDir.isEmpty() || innerDir == QStringLiteral("/")) return {};
  QString k = innerDir;
  while (k.startsWith(QLatin1Char('/'))) k.remove(0, 1);
  while (k.endsWith(QLatin1Char('/')))   k.chop(1);
  return k;
}

std::shared_ptr<ArchiveContext> ArchiveContext::load(
  const QString&         archivePath,
  QString*               errorOut,
  std::atomic<bool>*     cancelFlag,
  std::atomic<int>*      entriesRead) {
  auto ctx = std::make_shared<ArchiveContext>();
  ctx->archivePath  = archivePath;
  ctx->archiveMtime = QFileInfo(archivePath).lastModified();

  struct archive* a = archive_read_new();
  archive_read_support_format_all(a);
  archive_read_support_filter_all(a);

#ifdef Q_OS_WIN
  const int openResult = archive_read_open_filename_w(a, asWChar(archivePath), 64 * 1024);
#else
  const int openResult = archive_read_open_filename(a, archivePath.toUtf8().constData(), 64 * 1024);
#endif
  if (openResult != ARCHIVE_OK) {
    if (errorOut) {
      *errorOut = QObject::tr("Failed to open archive: %1")
                    .arg(QString::fromUtf8(archive_error_string(a)));
    }
    archive_read_free(a);
    return nullptr;
  }

  // パスワード付きアーカイブの早期検出。zip 中央ディレクトリのフラグから
  // 0 件 / >0 件 / 不明 の 3 状態で返る (tar は format 上「不明」を返す)。
  // >0 のときは v1.0 では非対応 (SPEC.md L457) なので拒否する。
  if (archive_read_has_encrypted_entries(a) > 0) {
    if (errorOut) {
      *errorOut = QObject::tr("Archive is password-protected (not supported).");
    }
    archive_read_free(a);
    return nullptr;
  }

  struct archive_entry* entry = nullptr;
  while (true) {
    // キャンセル要求のチェック (大きいアーカイブの中断用)
    if (cancelFlag && cancelFlag->load()) {
      if (errorOut) *errorOut = QObject::tr("Archive load cancelled.");
      archive_read_close(a);
      archive_read_free(a);
      return nullptr;
    }
    const int r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) break;
    if (r < ARCHIVE_WARN) break;  // 致命エラー
    if (r < ARCHIVE_OK)  continue; // 警告は黙って流す

    // パスワード付き検出 (フォーマットが直接答えない tar 等のフォールバック)
    if (archive_entry_is_encrypted(entry)) {
      if (errorOut) {
        *errorOut = QObject::tr("Archive is password-protected (not supported).");
      }
      archive_read_close(a);
      archive_read_free(a);
      return nullptr;
    }

    const QString path = readEntryPath(entry);
    if (path.isEmpty()) continue;  // ルート自身などはスキップ

    ArchiveEntry e;
    e.pathInArchive  = path;
    const int slash  = path.lastIndexOf(QLatin1Char('/'));
    e.name           = (slash < 0) ? path : path.mid(slash + 1);
    e.isDir          = (archive_entry_filetype(entry) == AE_IFDIR);
    e.size           = e.isDir ? -1 : static_cast<qint64>(archive_entry_size(entry));
    e.compressedSize = -1;  // libarchive は per-entry の圧縮後サイズを安定に提供しない
    const time_t mt  = archive_entry_mtime(entry);
    e.mtime          = (mt > 0) ? QDateTime::fromSecsSinceEpoch(mt) : QDateTime();
#ifdef Q_OS_WIN
    if (const wchar_t* wlink = archive_entry_symlink_w(entry)) {
      e.linkTarget = QString::fromWCharArray(wlink);
    } else if (const char* link = archive_entry_symlink_utf8(entry)) {
      e.linkTarget = QString::fromUtf8(link);
    }
#else
    if (const char* link = archive_entry_symlink_utf8(entry)) {
      e.linkTarget = QString::fromUtf8(link);
    } else if (const char* link2 = archive_entry_symlink(entry)) {
      e.linkTarget = QString::fromUtf8(link2);
    }
#endif

    ctx->entries.insert(path, e);
    if (entriesRead) entriesRead->fetch_add(1, std::memory_order_relaxed);

    // 親ディレクトリのエントリが明示されていないアーカイブのために、
    // 親パスから順次「合成ディレクトリ」エントリを作る (zip でよくある)。
    int cur = slash;
    while (cur > 0) {
      const QString dirPath = path.left(cur);
      if (!ctx->entries.contains(dirPath)) {
        ArchiveEntry d;
        d.pathInArchive = dirPath;
        const int s2    = dirPath.lastIndexOf(QLatin1Char('/'));
        d.name          = (s2 < 0) ? dirPath : dirPath.mid(s2 + 1);
        d.isDir         = true;
        ctx->entries.insert(dirPath, d);
      }
      cur = dirPath.lastIndexOf(QLatin1Char('/'));
    }
  }

  archive_read_close(a);
  archive_read_free(a);
  return ctx;
}

QList<const ArchiveEntry*> ArchiveContext::childrenOf(const QString& innerDir) const {
  const QString parentKey = normalizeDirKey(innerDir);
  QList<const ArchiveEntry*> out;
  for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
    const QString& p = it.key();
    const int slash  = p.lastIndexOf(QLatin1Char('/'));
    const QString parent = (slash < 0) ? QString() : p.left(slash);
    if (parent == parentKey) {
      out.append(&it.value());
    }
  }
  return out;
}

bool ArchiveContext::extractEntryTo(const QString& entryPath,
                                    const QString& destPath) const {
  // 親ディレクトリを確保
  QFileInfo destInfo(destPath);
  if (!QDir().mkpath(destInfo.absolutePath())) return false;

  struct archive* a = archive_read_new();
  archive_read_support_format_all(a);
  archive_read_support_filter_all(a);

#ifdef Q_OS_WIN
  const int openResult = archive_read_open_filename_w(a, asWChar(archivePath), 64 * 1024);
#else
  const int openResult = archive_read_open_filename(
    a, archivePath.toUtf8().constData(), 64 * 1024);
#endif
  if (openResult != ARCHIVE_OK) {
    archive_read_free(a);
    return false;
  }

  bool found = false;
  bool ok    = false;
  struct archive_entry* entry = nullptr;
  while (true) {
    const int r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) break;
    if (r < ARCHIVE_WARN) break;
    if (r < ARCHIVE_OK)  continue;

    const QString name = readEntryPath(entry);
    if (name != entryPath) continue;

    found = true;
    QFile out(destPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) break;

    bool writeOk = true;
    const void* buf = nullptr;
    size_t      sz  = 0;
    la_int64_t  off = 0;
    while (true) {
      const int rr = archive_read_data_block(a, &buf, &sz, &off);
      if (rr == ARCHIVE_EOF) break;
      if (rr < ARCHIVE_OK)  { writeOk = false; break; }
      const qint64 written = out.write(reinterpret_cast<const char*>(buf), sz);
      if (written < 0 || static_cast<size_t>(written) != sz) {
        writeOk = false; break;
      }
    }
    out.close();
    ok = writeOk;
    break;
  }

  archive_read_close(a);
  archive_read_free(a);
  if (!found || !ok) {
    QFile::remove(destPath);
    return false;
  }
  return true;
}

bool ArchiveContext::isValidDirectory(const QString& innerDir) const {
  if (innerDir.isEmpty() || innerDir == QStringLiteral("/")) return true;
  const QString key = normalizeDirKey(innerDir);
  auto it = entries.constFind(key);
  if (it != entries.cend()) return it.value().isDir;
  // 明示エントリが無くても、子孫が存在すれば「合成ディレクトリ」として有効
  for (auto i = entries.cbegin(); i != entries.cend(); ++i) {
    if (i.key().startsWith(key + QLatin1Char('/'))) return true;
  }
  return false;
}

} // namespace Farman

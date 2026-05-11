#include "ArchiveContext.h"

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

std::shared_ptr<ArchiveContext> ArchiveContext::load(const QString& archivePath) {
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
    archive_read_free(a);
    return nullptr;
  }

  struct archive_entry* entry = nullptr;
  while (true) {
    const int r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) break;
    if (r < ARCHIVE_WARN) break;  // 致命エラー
    if (r < ARCHIVE_OK)  continue; // 警告は黙って流す

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

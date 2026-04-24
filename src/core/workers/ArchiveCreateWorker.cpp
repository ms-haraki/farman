#include "ArchiveCreateWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <archive.h>
#include <archive_entry.h>
#include <sys/stat.h>

namespace Farman {

namespace {

// archive_write_open_filename は UTF-8 の char* を取る
static constexpr qint64 kReadBufSize = 64 * 1024;

} // anonymous namespace

ArchiveCreateWorker::ArchiveCreateWorker(const QString&     outputPath,
                                         Format             format,
                                         const QStringList& inputPaths,
                                         QObject*           parent)
  : WorkerBase(parent)
  , m_outputPath(outputPath)
  , m_format(format)
  , m_inputPaths(inputPaths) {
}

void ArchiveCreateWorker::run() {
  struct archive* a = archive_write_new();
  if (!a) {
    emit errorOccurred(m_outputPath, "archive_write_new failed");
    emit finished(false);
    return;
  }

  // フォーマット設定
  switch (m_format) {
    case Format::Zip:
      archive_write_set_format_zip(a);
      break;
    case Format::Tar:
      archive_write_set_format_pax_restricted(a);
      break;
    case Format::TarGz:
      archive_write_set_format_pax_restricted(a);
      archive_write_add_filter_gzip(a);
      break;
    case Format::TarBz2:
      archive_write_set_format_pax_restricted(a);
      archive_write_add_filter_bzip2(a);
      break;
    case Format::TarXz:
      archive_write_set_format_pax_restricted(a);
      archive_write_add_filter_xz(a);
      break;
  }

  if (archive_write_open_filename(a, m_outputPath.toUtf8().constData()) != ARCHIVE_OK) {
    emit errorOccurred(m_outputPath,
                       QString::fromUtf8(archive_error_string(a)));
    archive_write_free(a);
    emit finished(false);
    return;
  }

  WorkerProgress progress;
  progress.filesDone  = 0;
  progress.filesTotal = m_inputPaths.size();
  progress.processed  = 0;
  progress.total      = -1;

  bool success = true;
  for (const QString& inputPath : m_inputPaths) {
    if (isCancelled()) { success = false; break; }

    const QFileInfo info(inputPath);
    if (!info.exists()) {
      emit errorOccurred(inputPath, "Path does not exist");
      success = false;
      continue;
    }

    progress.currentFile = inputPath;
    emit progressUpdated(progress);

    const QString entryName = info.fileName();
    if (info.isDir()) {
      if (!addDirectoryRecursive(a, info.absoluteFilePath(), entryName)) {
        success = false;
      }
    } else {
      if (!addEntry(a, info.absoluteFilePath(), entryName)) {
        success = false;
      }
    }

    ++progress.filesDone;
    emit progressUpdated(progress);
  }

  archive_write_close(a);
  archive_write_free(a);

  // キャンセルされた場合は不完全なファイルを削除
  if (isCancelled()) {
    QFile::remove(m_outputPath);
    emit finished(false);
    return;
  }

  emit finished(success);
}

bool ArchiveCreateWorker::addEntry(::archive* a,
                                   const QString& absPath,
                                   const QString& entryName) {
  // Qt の QFile::Permissions をそのまま mode_t にキャストすると bits が
  // POSIX と食い違い、結果としてヘッダの file size/type が不正になって
  // 展開時に 0 バイトになる。stat(2) で直接取って archive_entry_copy_stat
  // に渡せば mode/size/mtime がまとめて正しく設定される。
  struct stat st {};
  if (::stat(absPath.toUtf8().constData(), &st) != 0) {
    emit errorOccurred(absPath, "stat failed");
    return false;
  }

  // 先にファイルを開いておく。open できない（= 権限無し）場合、ヘッダだけ
  // 書いてデータを書かない「壊れたエントリ」を残さないようここでエラー終了する。
  QFile f(absPath);
  if (!f.open(QIODevice::ReadOnly)) {
    emit errorOccurred(absPath, "Failed to open for reading");
    return false;
  }

  struct archive_entry* entry = archive_entry_new();
  archive_entry_set_pathname(entry, entryName.toUtf8().constData());
  archive_entry_copy_stat(entry, &st);

  const int hr = archive_write_header(a, entry);
  if (hr < ARCHIVE_OK) {
    emit errorOccurred(absPath,
                       QString::fromUtf8(archive_error_string(a)));
    if (hr < ARCHIVE_WARN) {
      f.close();
      archive_entry_free(entry);
      return false;
    }
  }

  QByteArray buf;
  buf.resize(kReadBufSize);
  while (!f.atEnd()) {
    if (isCancelled()) {
      f.close();
      archive_entry_free(entry);
      return false;
    }
    const qint64 got = f.read(buf.data(), buf.size());
    if (got <= 0) break;

    // archive_write_data は一度の呼び出しで指定バイト数全てを書き切らない
    // 可能性がある（short write）。残りがなくなるまでループして書く。
    const char* p = buf.constData();
    size_t remaining = static_cast<size_t>(got);
    while (remaining > 0) {
      const la_ssize_t wrote = archive_write_data(a, p, remaining);
      if (wrote < 0) {
        emit errorOccurred(absPath,
                           QString::fromUtf8(archive_error_string(a)));
        f.close();
        archive_entry_free(entry);
        return false;
      }
      if (wrote == 0) break;  // progress 無しの無限ループ防止
      p += wrote;
      remaining -= static_cast<size_t>(wrote);
    }
  }
  f.close();
  archive_entry_free(entry);
  return true;
}

bool ArchiveCreateWorker::addDirectoryRecursive(::archive* a,
                                                const QString& absPath,
                                                const QString& entryName) {
  // ディレクトリ自身のエントリ（stat 経由で mode/mtime を正しく設定）
  {
    struct stat st {};
    if (::stat(absPath.toUtf8().constData(), &st) != 0) {
      emit errorOccurred(absPath, "stat failed");
      return false;
    }
    struct archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry,
      (entryName + QLatin1Char('/')).toUtf8().constData());
    archive_entry_copy_stat(entry, &st);
    const int r = archive_write_header(a, entry);
    archive_entry_free(entry);
    if (r < ARCHIVE_OK) {
      emit errorOccurred(absPath,
                         QString::fromUtf8(archive_error_string(a)));
      if (r < ARCHIVE_WARN) return false;
    }
  }

  // 配下のエントリを列挙
  QDirIterator it(absPath, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
  bool ok = true;
  while (it.hasNext()) {
    if (isCancelled()) return false;
    const QString childAbs = it.next();
    const QFileInfo childInfo(childAbs);
    if (childInfo.isSymLink()) continue;  // シンボリックリンクは追跡しない

    const QString childEntry = entryName + QLatin1Char('/') + childInfo.fileName();
    if (childInfo.isDir()) {
      if (!addDirectoryRecursive(a, childAbs, childEntry)) ok = false;
    } else {
      if (!addEntry(a, childAbs, childEntry)) ok = false;
    }
  }
  return ok;
}

} // namespace Farman

#include "ArchiveExtractWorker.h"
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <archive.h>
#include <archive_entry.h>

namespace Farman {

namespace {

// archive の中から writer (disk) へ 1 エントリ分のデータをコピー
bool copyData(struct archive* src, struct archive* dst) {
  const void* buf = nullptr;
  size_t  size = 0;
  la_int64_t offset = 0;
  while (true) {
    const int r = archive_read_data_block(src, &buf, &size, &offset);
    if (r == ARCHIVE_EOF) return true;
    if (r < ARCHIVE_OK)   return false;
    if (archive_write_data_block(dst, buf, size, offset) < ARCHIVE_OK) {
      return false;
    }
  }
}

#ifdef Q_OS_WIN
// QString → const wchar_t* の薄いヘルパ。Windows では QString の utf16()
// をそのまま wchar_t* として扱える (Windows の wchar_t は 16-bit)。
inline const wchar_t* asWChar(const QString& s) {
  return reinterpret_cast<const wchar_t*>(s.utf16());
}
#endif

} // anonymous namespace

ArchiveExtractWorker::ArchiveExtractWorker(const QString& archivePath,
                                           const QString& outputDir,
                                           QObject*       parent)
  : WorkerBase(parent)
  , m_archivePath(archivePath)
  , m_outputDir(outputDir) {
}

void ArchiveExtractWorker::run() {
  // 展開先ディレクトリを確保
  QDir().mkpath(m_outputDir);

  struct archive* src = archive_read_new();
  archive_read_support_format_all(src);
  archive_read_support_filter_all(src);

  // Windows では archive_read_open_filename (char*) が ANSI 解釈なので、
  // 日本語パス (例: OneDrive\ドキュメント) や OneDrive 配下が開けない。
  // wchar_t* 版を使えば QString の UTF-16 表現をそのまま渡せる。
#ifdef Q_OS_WIN
  const int openResult = archive_read_open_filename_w(src, asWChar(m_archivePath), 64 * 1024);
#else
  const int openResult = archive_read_open_filename(src,
                          m_archivePath.toUtf8().constData(), 64 * 1024);
#endif
  if (openResult != ARCHIVE_OK) {
    emit errorOccurred(m_archivePath,
                       QString::fromUtf8(archive_error_string(src)));
    archive_read_free(src);
    emit finished(false);
    return;
  }

  struct archive* dst = archive_write_disk_new();
  archive_write_disk_set_options(dst,
    ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
    ARCHIVE_EXTRACT_ACL  | ARCHIVE_EXTRACT_FFLAGS);
  archive_write_disk_set_standard_lookup(dst);

  WorkerProgress progress;
  progress.filesDone  = 0;
  progress.filesTotal = 0;  // 事前には不明
  progress.processed  = 0;
  progress.total      = -1;

  bool success = true;
  struct archive_entry* entry = nullptr;
  while (true) {
    if (isCancelled()) { success = false; break; }

    const int r = archive_read_next_header(src, &entry);
    if (r == ARCHIVE_EOF) break;
    if (r < ARCHIVE_OK) {
      emit errorOccurred(m_archivePath,
                         QString::fromUtf8(archive_error_string(src)));
      if (r < ARCHIVE_WARN) { success = false; break; }
    }

    // 出力パスを outputDir 配下へ書き換える。
    // Windows ではアーカイブ内エントリ名が UTF-16 で取れる pathname_w を使う
    // (zip の EFS bit 付きや PaX UTF-8 tar に対応)。書き戻す宛先 path も
    // 同様に wchar_t 版でセットしないと write-disk 段で日本語ファイル名が
    // 化けて作成失敗する。
    QString origName;
#ifdef Q_OS_WIN
    if (const wchar_t* wname = archive_entry_pathname_w(entry)) {
      origName = QString::fromWCharArray(wname);
    } else if (const char* uname = archive_entry_pathname_utf8(entry)) {
      origName = QString::fromUtf8(uname);
    } else if (const char* name = archive_entry_pathname(entry)) {
      origName = QString::fromLocal8Bit(name);
    }
#else
    origName = QString::fromUtf8(archive_entry_pathname(entry));
#endif
    const QString destPath = QDir(m_outputDir).absoluteFilePath(origName);
#ifdef Q_OS_WIN
    archive_entry_copy_pathname_w(entry, asWChar(destPath));
#else
    archive_entry_set_pathname(entry, destPath.toUtf8().constData());
#endif

    progress.currentFile = destPath;
    emit progressUpdated(progress);

    const int hr = archive_write_header(dst, entry);
    if (hr < ARCHIVE_OK) {
      emit errorOccurred(destPath,
                         QString::fromUtf8(archive_error_string(dst)));
      if (hr < ARCHIVE_WARN) { success = false; break; }
    } else if (archive_entry_size(entry) > 0) {
      if (!copyData(src, dst)) {
        emit errorOccurred(destPath,
                           QString::fromUtf8(archive_error_string(dst)));
        success = false;
        break;
      }
    }
    if (archive_write_finish_entry(dst) < ARCHIVE_OK) {
      emit errorOccurred(destPath,
                         QString::fromUtf8(archive_error_string(dst)));
      success = false;
      break;
    }

    ++progress.filesDone;
    emit progressUpdated(progress);
  }

  archive_read_close(src);
  archive_read_free(src);
  archive_write_close(dst);
  archive_write_free(dst);

  emit finished(success && !isCancelled());
}

} // namespace Farman

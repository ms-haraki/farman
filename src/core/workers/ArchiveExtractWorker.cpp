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

  if (archive_read_open_filename(src, m_archivePath.toUtf8().constData(),
                                 64 * 1024) != ARCHIVE_OK) {
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

    // 出力パスを outputDir 配下へ書き換える
    const QString origName = QString::fromUtf8(archive_entry_pathname(entry));
    const QString destPath = QDir(m_outputDir).absoluteFilePath(origName);
    archive_entry_set_pathname(entry, destPath.toUtf8().constData());

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

#include "FileItem.h"
#include "ArchiveContext.h"
#include "utils/ArchivePath.h"
#include <QMimeDatabase>

namespace Farman {

FileItem::FileItem(const QFileInfo& info) : m_info(info) {
}

FileItem::FileItem(const ArchiveEntry& entry,
                   std::shared_ptr<ArchiveContext> ctx)
  : m_archiveEntry(entry)
  , m_archiveContext(std::move(ctx)) {
}

QString FileItem::name() const {
  if (m_archiveEntry) return m_archiveEntry->name;
  return m_info.fileName();
}

QString FileItem::absolutePath() const {
  if (m_archiveEntry && m_archiveContext) {
    // "<archive>!/<inner-path>" 形式で返す
    return ArchivePath::joinArchivePath(
      m_archiveContext->archivePath,
      QStringLiteral("/") + m_archiveEntry->pathInArchive);
  }
  return m_info.absoluteFilePath();
}

QString FileItem::suffix() const {
  if (m_archiveEntry) {
    const int dot = m_archiveEntry->name.lastIndexOf(QLatin1Char('.'));
    if (dot < 0 || dot == 0) return {};  // 先頭ドットは「拡張子無し」扱い
    return m_archiveEntry->name.mid(dot + 1).toLower();
  }
  return m_info.suffix().toLower();
}

QString FileItem::mimeType() const {
  if (m_archiveEntry) {
    // アーカイブ内エントリは実在しないので mimeTypeForFile が使えない。
    // 名前ベースで判定する。
    if (m_mimeType.isEmpty()) {
      QMimeDatabase db;
      m_mimeType = db.mimeTypeForFile(
        m_archiveEntry->name,
        QMimeDatabase::MatchExtension).name();
    }
    return m_mimeType;
  }
  if (m_mimeType.isEmpty() && m_info.exists()) {
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(m_info);
    m_mimeType = type.name();
  }
  return m_mimeType;
}

qint64 FileItem::size() const {
  if (m_archiveEntry) {
    return m_archiveEntry->isDir ? -1 : m_archiveEntry->size;
  }
  return m_info.isDir() ? -1 : m_info.size();
}

QDateTime FileItem::lastModified() const {
  if (m_archiveEntry) return m_archiveEntry->mtime;
  return m_info.lastModified();
}

bool FileItem::isDir() const {
  if (m_archiveEntry) return m_archiveEntry->isDir;
  return m_info.isDir();
}

bool FileItem::isFile() const {
  if (m_archiveEntry) return !m_archiveEntry->isDir;
  return m_info.isFile();
}

bool FileItem::isHidden() const {
  if (m_archiveEntry) {
    // アーカイブ内では Unix 系の「ドット始まり」を hidden と扱う
    return m_archiveEntry->name.startsWith(QLatin1Char('.'));
  }
  return m_info.isHidden();
}

bool FileItem::isSymLink() const {
  if (m_archiveEntry) return !m_archiveEntry->linkTarget.isEmpty();
  return m_info.isSymLink();
}

bool FileItem::isDotDot() const {
  if (m_archiveEntry) return m_archiveEntry->name == QStringLiteral("..");
  return m_info.fileName() == "..";
}

bool FileItem::isSelected() const {
  return m_selected;
}

void FileItem::setSelected(bool selected) {
  m_selected = selected;
}

const QFileInfo& FileItem::fileInfo() const {
  return m_info;
}

} // namespace Farman

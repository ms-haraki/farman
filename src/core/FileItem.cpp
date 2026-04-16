#include "FileItem.h"
#include <QMimeDatabase>

namespace Farman {

FileItem::FileItem(const QFileInfo& info) : m_info(info) {
}

QString FileItem::name() const {
  return m_info.fileName();
}

QString FileItem::absolutePath() const {
  return m_info.absoluteFilePath();
}

QString FileItem::suffix() const {
  return m_info.suffix().toLower();
}

QString FileItem::mimeType() const {
  if (m_mimeType.isEmpty() && m_info.exists()) {
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(m_info);
    m_mimeType = type.name();
  }
  return m_mimeType;
}

qint64 FileItem::size() const {
  return m_info.isDir() ? -1 : m_info.size();
}

QDateTime FileItem::lastModified() const {
  return m_info.lastModified();
}

bool FileItem::isDir() const {
  return m_info.isDir();
}

bool FileItem::isFile() const {
  return m_info.isFile();
}

bool FileItem::isHidden() const {
  return m_info.isHidden();
}

bool FileItem::isSymLink() const {
  return m_info.isSymLink();
}

bool FileItem::isDotDot() const {
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

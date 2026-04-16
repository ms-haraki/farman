#include "FileListModel.h"
#include <QDir>
#include <QFileInfo>
#include <QColor>
#include <algorithm>

namespace Farman {

FileListModel::FileListModel(QObject* parent)
  : QAbstractItemModel(parent) {
}

FileListModel::~FileListModel() = default;

QString FileListModel::currentPath() const {
  return m_currentPath;
}

bool FileListModel::setPath(const QString& path) {
  QDir dir(path);
  if (!dir.exists() || !dir.isReadable()) {
    emit loadFailed(path, "Directory does not exist or is not readable");
    return false;
  }

  beginResetModel();

  m_currentPath = dir.absolutePath();
  m_entries.clear();

  // ディレクトリエントリを読み込む
  QFileInfoList infoList = dir.entryInfoList(
    QDir::AllEntries | QDir::NoDotAndDotDot,
    QDir::NoSort  // 自前でソート
  );

  // FileItem に変換
  for (const QFileInfo& info : infoList) {
    m_entries.append(std::make_shared<FileItem>(info));
  }

  // ".." エントリを追加（親ディレクトリがある場合）
  if (dir.cdUp()) {
    QFileInfo parentInfo(dir.absolutePath());
    parentInfo = QFileInfo(m_currentPath + "/..");
    m_entries.prepend(std::make_shared<FileItem>(parentInfo));
  }

  applyFilterAndSort();

  endResetModel();

  emit pathChanged(m_currentPath);
  return true;
}

void FileListModel::refresh() {
  if (!m_currentPath.isEmpty()) {
    setPath(m_currentPath);
  }
}

void FileListModel::setSortSettings(
  SortKey       key,
  Qt::SortOrder order,
  SortKey       key2nd,
  SortDirsType  dirsType,
  bool          dotFirst,
  Qt::CaseSensitivity cs) {

  m_sortKey    = key;
  m_sortOrder  = order;
  m_sortKey2nd = key2nd;
  m_dirsType   = dirsType;
  m_dotFirst   = dotFirst;
  m_cs         = cs;

  if (!m_entries.isEmpty()) {
    beginResetModel();
    applyFilterAndSort();
    endResetModel();
  }
}

void FileListModel::setNameFilters(const QStringList& patterns) {
  m_nameFilters = patterns;
  if (!m_entries.isEmpty()) {
    beginResetModel();
    applyFilterAndSort();
    endResetModel();
  }
}

void FileListModel::setAttrFilter(AttrFilterFlags flags) {
  m_attrFilter = flags;
  if (!m_entries.isEmpty()) {
    beginResetModel();
    applyFilterAndSort();
    endResetModel();
  }
}

const FileItem* FileListModel::itemAt(const QModelIndex& index) const {
  if (!index.isValid() || index.row() >= m_entries.size()) {
    return nullptr;
  }
  return m_entries[index.row()].get();
}

const FileItem* FileListModel::itemAt(int row) const {
  if (row < 0 || row >= m_entries.size()) {
    return nullptr;
  }
  return m_entries[row].get();
}

int FileListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;  // フラットリスト
  }
  return m_entries.size();
}

int FileListModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return ColumnCount;
}

QList<int> FileListModel::selectedRows() const {
  QList<int> rows;
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i]->isSelected()) {
      rows.append(i);
    }
  }
  return rows;
}

QList<const FileItem*> FileListModel::selectedItems() const {
  QList<const FileItem*> items;
  for (const auto& entry : m_entries) {
    if (entry->isSelected()) {
      items.append(entry.get());
    }
  }
  return items;
}

void FileListModel::setSelected(int row, bool selected) {
  if (row < 0 || row >= m_entries.size()) {
    return;
  }

  m_entries[row]->setSelected(selected);

  QModelIndex idx = index(row, 0);
  emit dataChanged(idx, index(row, ColumnCount - 1));
}

void FileListModel::setSelectedAll(bool selected) {
  for (auto& entry : m_entries) {
    if (!entry->isDotDot()) {  // ".." は選択しない
      entry->setSelected(selected);
    }
  }

  if (!m_entries.isEmpty()) {
    emit dataChanged(
      index(0, 0),
      index(m_entries.size() - 1, ColumnCount - 1)
    );
  }
}

void FileListModel::toggleSelected(int row) {
  if (row < 0 || row >= m_entries.size()) {
    return;
  }

  const FileItem* item = m_entries[row].get();
  if (item->isDotDot()) {
    return;  // ".." は選択しない
  }

  setSelected(row, !item->isSelected());
}

void FileListModel::invertSelection() {
  for (auto& entry : m_entries) {
    if (!entry->isDotDot()) {
      entry->setSelected(!entry->isSelected());
    }
  }

  if (!m_entries.isEmpty()) {
    emit dataChanged(
      index(0, 0),
      index(m_entries.size() - 1, ColumnCount - 1)
    );
  }
}

bool FileListModel::isAllSelected() const {
  for (const auto& entry : m_entries) {
    if (!entry->isDotDot() && !entry->isSelected()) {
      return false;
    }
  }
  return !m_entries.isEmpty();
}

QModelIndex FileListModel::index(int row, int col, const QModelIndex& parent) const {
  if (parent.isValid() || row < 0 || row >= m_entries.size() ||
      col < 0 || col >= ColumnCount) {
    return {};
  }

  return createIndex(row, col);
}

QModelIndex FileListModel::parent(const QModelIndex& index) const {
  Q_UNUSED(index);
  return {};  // フラットリスト
}

QVariant FileListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= m_entries.size()) {
    return {};
  }

  const FileItem* item = m_entries[index.row()].get();

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case Name:
        return item->name();
      case Size:
        if (item->isDir()) {
          return QString("<DIR>");
        } else {
          qint64 bytes = item->size();
          if (bytes < 1024) {
            return QString("%1 B").arg(bytes);
          } else if (bytes < 1024 * 1024) {
            return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
          } else if (bytes < 1024 * 1024 * 1024) {
            return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
          } else {
            return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
          }
        }
      case Type:
        return item->suffix().isEmpty() ? QString("") : item->suffix();
      case LastModified:
        return item->lastModified().toString("yyyy/MM/dd HH:mm:ss");
    }
  }
  else if (role == Qt::BackgroundRole) {
    if (item->isSelected()) {
      // 選択中の背景色（青系）
      return QColor(0, 120, 215);  // Windows 10 の選択色に近い
    }
  }
  else if (role == Qt::ForegroundRole) {
    if (item->isSelected()) {
      // 選択中の文字色（白）
      return QColor(Qt::white);
    }
  }
  else if (role == FileItemRole) {
    return QVariant::fromValue(const_cast<FileItem*>(item));
  }
  else if (role == IsSelectedRole) {
    return item->isSelected();
  }
  else if (role == IsDirRole) {
    return item->isDir();
  }
  else if (role == IsHiddenRole) {
    return item->isHidden();
  }

  return {};
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return {};
  }

  switch (section) {
    case Name:         return tr("Name");
    case Size:         return tr("Size");
    case Type:         return tr("Type");
    case LastModified: return tr("Modified");
    default:           return {};
  }
}

void FileListModel::applyFilterAndSort() {
  if (m_entries.isEmpty()) {
    return;
  }

  // フィルタリング（今は実装をシンプルにするためスキップ）
  // TODO: 名前フィルタと属性フィルタの実装

  // ソート
  std::stable_sort(m_entries.begin(), m_entries.end(),
    [this](const std::shared_ptr<FileItem>& a, const std::shared_ptr<FileItem>& b) {
      // ".." は常に先頭
      if (m_dotFirst) {
        if (a->isDotDot()) return true;
        if (b->isDotDot()) return false;
      }

      // ディレクトリのソート位置
      if (m_dirsType == SortDirsType::First) {
        if (a->isDir() && !b->isDir()) return true;
        if (!a->isDir() && b->isDir()) return false;
      } else if (m_dirsType == SortDirsType::Last) {
        if (a->isDir() && !b->isDir()) return false;
        if (!a->isDir() && b->isDir()) return true;
      }

      // 主ソートキー
      int cmp = compareItems(a.get(), b.get(), m_sortKey);
      if (cmp != 0) {
        return m_sortOrder == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
      }

      // 第2ソートキー
      cmp = compareItems(a.get(), b.get(), m_sortKey2nd);
      return m_sortOrder == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
    });
}

int FileListModel::compareItems(const FileItem* a, const FileItem* b, SortKey key) const {
  switch (key) {
    case SortKey::Name: {
      QString nameA = a->name();
      QString nameB = b->name();
      return nameA.compare(nameB, m_cs);
    }
    case SortKey::Size: {
      qint64 sizeA = a->size();
      qint64 sizeB = b->size();
      if (sizeA < sizeB) return -1;
      if (sizeA > sizeB) return 1;
      return 0;
    }
    case SortKey::Type: {
      QString typeA = a->suffix();
      QString typeB = b->suffix();
      return typeA.compare(typeB, m_cs);
    }
    case SortKey::LastModified: {
      QDateTime dtA = a->lastModified();
      QDateTime dtB = b->lastModified();
      if (dtA < dtB) return -1;
      if (dtA > dtB) return 1;
      return 0;
    }
  }
  return 0;
}

} // namespace Farman

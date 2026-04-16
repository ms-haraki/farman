#include "FileListModel.h"

namespace Farman {

FileListModel::FileListModel(QObject* parent)
  : QAbstractItemModel(parent) {
}

FileListModel::~FileListModel() = default;

QString FileListModel::currentPath() const {
  return m_currentPath;
}

bool FileListModel::setPath(const QString& path) {
  // TODO: 実装
  return false;
}

void FileListModel::refresh() {
  // TODO: 実装
}

void FileListModel::setSortSettings(
  SortKey       key,
  Qt::SortOrder order,
  SortKey       key2nd,
  SortDirsType  dirsType,
  bool          dotFirst,
  Qt::CaseSensitivity cs) {
  // TODO: 実装
}

void FileListModel::setNameFilters(const QStringList& patterns) {
  // TODO: 実装
}

void FileListModel::setAttrFilter(AttrFilterFlags flags) {
  // TODO: 実装
}

const FileItem* FileListModel::itemAt(const QModelIndex& index) const {
  // TODO: 実装
  return nullptr;
}

const FileItem* FileListModel::itemAt(int row) const {
  // TODO: 実装
  return nullptr;
}

int FileListModel::rowCount(const QModelIndex& parent) const {
  // TODO: 実装
  return 0;
}

int FileListModel::columnCount(const QModelIndex& parent) const {
  return ColumnCount;
}

QList<int> FileListModel::selectedRows() const {
  // TODO: 実装
  return {};
}

QList<const FileItem*> FileListModel::selectedItems() const {
  // TODO: 実装
  return {};
}

void FileListModel::setSelected(int row, bool selected) {
  // TODO: 実装
}

void FileListModel::setSelectedAll(bool selected) {
  // TODO: 実装
}

void FileListModel::toggleSelected(int row) {
  // TODO: 実装
}

void FileListModel::invertSelection() {
  // TODO: 実装
}

QModelIndex FileListModel::index(int row, int col, const QModelIndex& parent) const {
  // TODO: 実装
  return {};
}

QModelIndex FileListModel::parent(const QModelIndex& index) const {
  return {};
}

QVariant FileListModel::data(const QModelIndex& index, int role) const {
  // TODO: 実装
  return {};
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  // TODO: 実装
  return {};
}

void FileListModel::applyFilterAndSort() {
  // TODO: 実装
}

} // namespace Farman

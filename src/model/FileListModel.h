#pragma once

#include "types.h"
#include "core/FileItem.h"
#include <QAbstractItemModel>
#include <QFileSystemWatcher>
#include <memory>

namespace Farman {

class FileListModel : public QAbstractItemModel {
  Q_OBJECT

public:
  // 列定義
  enum Column {
    Name         = 0,
    Size         = 1,
    Type         = 2,
    LastModified = 3,
    ColumnCount
  };

  // カスタムロール
  enum Role {
    FileItemRole = Qt::UserRole + 1,  // FileItem* を返す
    IsSelectedRole,
    IsDirRole,
    IsHiddenRole
  };

  explicit FileListModel(QObject* parent = nullptr);
  ~FileListModel() override;

  // ── パス操作 ──────────────────────────────
  QString currentPath() const;
  bool    setPath(const QString& path);  // false = アクセス不可
  void    refresh();

  // ── ソート ───────────────────────────────
  void setSortSettings(
    SortKey       key,
    Qt::SortOrder order       = Qt::AscendingOrder,
    SortKey       key2nd      = SortKey::Name,
    SortDirsType  dirsType    = SortDirsType::First,
    bool          dotFirst    = true,
    Qt::CaseSensitivity cs    = Qt::CaseInsensitive
  );

  // ── フィルタ ──────────────────────────────
  void setNameFilters(const QStringList& patterns);  // glob: {"*.cpp","*.h"}
  void setAttrFilter(AttrFilterFlags flags);

  // ── アイテムアクセス ──────────────────────
  const FileItem* itemAt(const QModelIndex& index) const;
  const FileItem* itemAt(int row) const;
  int             rowCount(const QModelIndex& parent = {}) const override;
  int             columnCount(const QModelIndex& parent = {}) const override;

  // ── 選択操作 ──────────────────────────────
  QList<int>             selectedRows() const;
  QList<const FileItem*> selectedItems() const;
  void setSelected(int row, bool selected);
  void setSelectedAll(bool selected);
  void toggleSelected(int row);
  void invertSelection();

  // ── QAbstractItemModel 必須実装 ───────────
  QModelIndex index(int row, int col,
                    const QModelIndex& parent = {}) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  QVariant    data(const QModelIndex& index,
                   int role = Qt::DisplayRole) const override;
  QVariant    headerData(int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const override;

signals:
  void pathChanged(const QString& newPath);
  void loadFailed(const QString& path, const QString& reason);

private:
  void applyFilterAndSort();
  int compareItems(const FileItem* a, const FileItem* b, SortKey key) const;

  QString                          m_currentPath;
  QList<std::shared_ptr<FileItem>> m_entries;
  QFileSystemWatcher               m_watcher;

  // ソート設定
  SortKey             m_sortKey    = SortKey::Name;
  SortKey             m_sortKey2nd = SortKey::Name;
  Qt::SortOrder       m_sortOrder  = Qt::AscendingOrder;
  SortDirsType        m_dirsType   = SortDirsType::First;
  bool                m_dotFirst   = true;
  Qt::CaseSensitivity m_cs         = Qt::CaseInsensitive;

  // フィルタ設定
  QStringList     m_nameFilters;
  AttrFilterFlags m_attrFilter = AttrFilter::None;
};

} // namespace Farman

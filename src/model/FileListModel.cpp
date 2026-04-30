#include "FileListModel.h"
#include "settings/Settings.h"
#include <QDir>
#include <QFileInfo>
#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <algorithm>

namespace Farman {

FileListModel::FileListModel(QObject* parent)
  : QAbstractItemModel(parent) {
}

FileListModel::~FileListModel() = default;

void FileListModel::setActive(bool active) {
  if (m_active == active) return;
  m_active = active;
  if (!m_entries.isEmpty()) {
    emit dataChanged(
      index(0, 0),
      index(m_entries.size() - 1, ColumnCount - 1),
      { Qt::ForegroundRole, Qt::BackgroundRole, Qt::FontRole });
  }
}

void FileListModel::setSinglePaneMode(bool single) {
  if (m_singlePane == single) return;
  m_singlePane = single;
  if (!m_entries.isEmpty()) {
    // サイズ列・更新日時列だけ再描画させる
    emit dataChanged(
      index(0, Size),
      index(m_entries.size() - 1, LastModified),
      { Qt::DisplayRole });
  }
}

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

  // ディレクトリエントリを読み込む。
  // 隠し・システムファイルも一旦全て取得し、表示可否は applyFilterAndSort で制御する。
  QFileInfoList infoList = dir.entryInfoList(
    QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
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

void FileListModel::toggleHiddenFiles() {
  if (m_attrFilter.testFlag(AttrFilter::ShowHidden)) {
    m_attrFilter.setFlag(AttrFilter::ShowHidden, false);
  } else {
    m_attrFilter.setFlag(AttrFilter::ShowHidden, true);
  }

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

QStringList FileListModel::selectedFilePaths() const {
  QStringList paths;
  for (const auto& entry : m_entries) {
    if (entry->isSelected() && !entry->isDotDot()) {
      paths.append(entry->absolutePath());
    }
  }
  return paths;
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
      case Name: {
        QString name = item->name();
        // ".." や "." はそのまま表示
        if (name == ".." || name == ".") {
          return name;
        }
        // ディレクトリは拡張子を切り出さず、名前全体を表示する
        // (例: "My.Project" のようなドット入りディレクトリ名でも分割しない)
        if (item->isDir()) {
          return name;
        }
        // ドットファイル（.から始まる）の場合はファイル名全体を表示
        if (name.startsWith('.')) {
          return name;
        }
        // 通常のファイルは拡張子を除いた名前を表示
        QString suffix = item->suffix();
        if (!suffix.isEmpty()) {
          // 拡張子がある場合は除去
          return name.left(name.length() - suffix.length() - 1);  // -1 は "." の分
        }
        return name;
      }
      case Type: {
        // ディレクトリは Type 列を空にする (拡張子相当の判定をしない)
        if (item->isDir()) {
          return QString("");
        }
        QString name = item->name();
        // ドットファイル（.から始まる）の場合はTypeを空にする
        if (name.startsWith('.') && name != ".." && name != ".") {
          return QString("");
        }
        // 通常のファイルは拡張子を表示
        return item->suffix().isEmpty() ? QString("") : item->suffix();
      }
      case Size:
        if (item->isDir()) {
          return QString("<DIR>");
        } else {
          const Settings& s = Settings::instance();
          const FileSizeFormat fmt = m_singlePane ? s.fileSizeFormatSingle()
                                                  : s.fileSizeFormatDual();
          const qint64 bytes = item->size();
          // 桁区切りカンマは Auto 以外で適用 (デフォルト ON)。
          const bool sep = m_singlePane ? s.fileSizeThousandsSeparatorSingle()
                                        : s.fileSizeThousandsSeparatorDual();
          const bool useSep = (fmt != FileSizeFormat::Auto) && sep;
          QLocale loc(QLocale::English);
          if (!useSep) {
            // ロケールの数値書式から 1000 区切りを外す
            loc.setNumberOptions(QLocale::OmitGroupSeparator);
          }
          switch (fmt) {
            case FileSizeFormat::Bytes:
              // 素のバイト数。OmitGroupSeparator を切替えるだけで
              // "12,345,678" と "12345678" が切替わる。
              return QStringLiteral("%1 B").arg(loc.toString(bytes));
            case FileSizeFormat::SI:
              // 1000-based KB/MB/GB
              return loc.formattedDataSize(bytes, 1, QLocale::DataSizeSIFormat);
            case FileSizeFormat::IEC:
              // 1024-based KiB/MiB/GiB
              return loc.formattedDataSize(bytes, 1, QLocale::DataSizeIecFormat);
            case FileSizeFormat::Auto:
            default:
              // Auto は桁区切りトグルを無視し、ロケール既定に任せる
              // (英語ロケールで 1024-based + KB ラベル)。
              return QLocale(QLocale::English).formattedDataSize(bytes);
          }
        }
      case LastModified: {
        const Settings& s = Settings::instance();
        const QString fmt = m_singlePane ? s.dateTimeFormatSingle()
                                         : s.dateTimeFormatDual();
        return item->lastModified().toString(
          fmt.isEmpty() ? QStringLiteral("yyyy/MM/dd HH:mm:ss") : fmt);
      }
      case Created: {
        if (item->name() == QStringLiteral("..")) return QString();
        const QFileInfo& fi = item->fileInfo();
        // birthTime は FS が対応していないと invalid を返す。その場合は空表示。
        const QDateTime t = fi.birthTime();
        if (!t.isValid()) return QString();
        const Settings& s = Settings::instance();
        const QString fmt = m_singlePane ? s.dateTimeFormatSingle()
                                         : s.dateTimeFormatDual();
        return t.toString(fmt.isEmpty() ? QStringLiteral("yyyy/MM/dd HH:mm:ss") : fmt);
      }
      case Permissions: {
        if (item->name() == QStringLiteral("..")) return QString();
        // "rwxr-xr-x" 形式に整形。Qt の QFile::Permissions ビットを参照。
        const QFile::Permissions p = item->fileInfo().permissions();
        QString s = QStringLiteral("---------");
        if (p & QFile::ReadOwner)  s[0] = QLatin1Char('r');
        if (p & QFile::WriteOwner) s[1] = QLatin1Char('w');
        if (p & QFile::ExeOwner)   s[2] = QLatin1Char('x');
        if (p & QFile::ReadGroup)  s[3] = QLatin1Char('r');
        if (p & QFile::WriteGroup) s[4] = QLatin1Char('w');
        if (p & QFile::ExeGroup)   s[5] = QLatin1Char('x');
        if (p & QFile::ReadOther)  s[6] = QLatin1Char('r');
        if (p & QFile::WriteOther) s[7] = QLatin1Char('w');
        if (p & QFile::ExeOther)   s[8] = QLatin1Char('x');
        return s;
      }
      case Attributes: {
        if (item->name() == QStringLiteral("..")) return QString();
        // クロスプラットフォームに使える QFileInfo 系のフラグだけで構成。
        // Windows のシステム/アーカイブビット等は Qt API では取れない。
        const QFileInfo& fi = item->fileInfo();
        QString s;
        if (fi.isHidden())    s.append(QLatin1Char('H'));
        if (!fi.isWritable()) s.append(QLatin1Char('R'));
        if (fi.isSymLink())   s.append(QLatin1Char('L'));
        return s.isEmpty() ? QStringLiteral("-") : s;
      }
      case Owner: {
        if (item->name() == QStringLiteral("..")) return QString();
        const QString owner = item->fileInfo().owner();
        return owner.isEmpty() ? QString::number(item->fileInfo().ownerId())
                               : owner;
      }
      case Group: {
        if (item->name() == QStringLiteral("..")) return QString();
        const QString group = item->fileInfo().group();
        return group.isEmpty() ? QString::number(item->fileInfo().groupId())
                               : group;
      }
      case LinkTarget: {
        if (item->name() == QStringLiteral("..")) return QString();
        const QFileInfo& fi = item->fileInfo();
        if (!fi.isSymLink()) return QString();
        return QStringLiteral("→ %1").arg(fi.symLinkTarget());
      }
    }
  }
  else if (role == Qt::DecorationRole) {
    // Name列にファイルアイコンを表示
    if (index.column() == Name) {
      return m_iconProvider.icon(item->fileInfo());
    }
  }
  else if (role == Qt::BackgroundRole) {
    FileCategory cat = item->isHidden() ? FileCategory::Hidden
                     : item->isDir()    ? FileCategory::Directory
                                        : FileCategory::Normal;
    const bool inactive = !m_active && Settings::instance().useInactivePaneColors();
    const QColor bg = Settings::instance().categoryColor(cat, item->isSelected(), inactive).background;
    if (bg.isValid()) return bg;
  }
  else if (role == Qt::ForegroundRole) {
    FileCategory cat = item->isHidden() ? FileCategory::Hidden
                     : item->isDir()    ? FileCategory::Directory
                                        : FileCategory::Normal;
    const bool inactive = !m_active && Settings::instance().useInactivePaneColors();
    const QColor fg = Settings::instance().categoryColor(cat, item->isSelected(), inactive).foreground;
    if (fg.isValid()) return fg;
  }
  else if (role == Qt::FontRole) {
    FileCategory cat = item->isHidden() ? FileCategory::Hidden
                     : item->isDir()    ? FileCategory::Directory
                                        : FileCategory::Normal;
    const bool inactive = !m_active && Settings::instance().useInactivePaneColors();
    if (Settings::instance().categoryColor(cat, item->isSelected(), inactive).bold) {
      // ベースにビューに設定したファイルリスト用フォントを使う。
      // ここで QGuiApplication::font() を使うと、ユーザーが Settings で
      // フォントを変えても太字カテゴリ (既定でディレクトリ) だけ反映されない。
      QFont f = Settings::instance().font();
      f.setBold(true);
      return f;
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
    case Created:      return tr("Created");
    case Permissions:  return tr("Permissions");
    case Attributes:   return tr("Attributes");
    case Owner:        return tr("Owner");
    case Group:        return tr("Group");
    case LinkTarget:   return tr("Link Target");
    default:           return {};
  }
}

void FileListModel::applyFilterAndSort() {
  if (m_entries.isEmpty()) {
    return;
  }

  // フィルタリング: 条件に合わないアイテムを削除
  auto it = m_entries.begin();
  while (it != m_entries.end()) {
    const FileItem* item = it->get();
    bool shouldRemove = false;

    // ".." は常に保持
    if (item->isDotDot()) {
      ++it;
      continue;
    }

    // 隠しファイルフィルタ
    if (!m_attrFilter.testFlag(AttrFilter::ShowHidden) && item->isHidden()) {
      shouldRemove = true;
    }

    // ディレクトリ/ファイルのみフィルタ
    if (m_attrFilter.testFlag(AttrFilter::DirsOnly) && !item->isDir()) {
      shouldRemove = true;
    }
    if (m_attrFilter.testFlag(AttrFilter::FilesOnly) && item->isDir()) {
      shouldRemove = true;
    }

    // 名前フィルタ（拡張子パターン）
    if (!m_nameFilters.isEmpty() && !item->isDir()) {
      bool matches = false;
      for (const QString& pattern : m_nameFilters) {
        if (QDir::match(pattern, item->name())) {
          matches = true;
          break;
        }
      }
      if (!matches) {
        shouldRemove = true;
      }
    }

    if (shouldRemove) {
      it = m_entries.erase(it);
    } else {
      ++it;
    }
  }

  // ソート
  std::stable_sort(m_entries.begin(), m_entries.end(),
    [this](const std::shared_ptr<FileItem>& a, const std::shared_ptr<FileItem>& b) {
      // ".." は常に先頭（ソート設定に関わらず）
      if (a->isDotDot()) return true;
      if (b->isDotDot()) return false;

      // ディレクトリのソート位置
      if (m_dirsType == SortDirsType::First) {
        if (a->isDir() && !b->isDir()) return true;
        if (!a->isDir() && b->isDir()) return false;
      } else if (m_dirsType == SortDirsType::Last) {
        if (a->isDir() && !b->isDir()) return false;
        if (!a->isDir() && b->isDir()) return true;
      }

      // ドットで始まるエントリを同一グループ（ディレクトリ／ファイル）内で先頭に。
      // ディレクトリ・ファイル双方に適用。
      if (m_dotFirst) {
        const bool aDot = a->name().startsWith('.');
        const bool bDot = b->name().startsWith('.');
        if (aDot != bDot) return aDot;
      }

      // 主ソートキー
      int cmp = compareItems(a.get(), b.get(), m_sortKey);
      if (cmp != 0) {
        return m_sortOrder == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
      }

      // 第2ソートキー
      if (m_sortKey2nd != SortKey::None) {
        cmp = compareItems(a.get(), b.get(), m_sortKey2nd);
        return m_sortOrder == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
      }

      return false;
    });
}

int FileListModel::compareItems(const FileItem* a, const FileItem* b, SortKey key) const {
  switch (key) {
    case SortKey::None:
      return 0;
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
      // ディレクトリは Type 列を持たない扱いにするため、ソート時も
      // 空文字として比較する (suffix() を見ない)。
      QString typeA = a->isDir() ? QString() : a->suffix();
      QString typeB = b->isDir() ? QString() : b->suffix();
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

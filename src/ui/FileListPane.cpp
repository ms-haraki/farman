#include "FileListPane.h"
#include "FileListDelegate.h"
#include "model/FileListModel.h"
#include "types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QTableView>
#include <QHeaderView>
#include <QStyle>

namespace Farman {

FileListPane::FileListPane(QWidget* parent)
  : QWidget(parent)
  , m_pathLabel(nullptr)
  , m_folderButton(nullptr)
  , m_view(nullptr)
  , m_sortFilterStatusLabel(nullptr)
  , m_model(nullptr)
  , m_delegate(nullptr) {

  setupUi();
}

FileListPane::~FileListPane() = default;

void FileListPane::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(2);

  // パスラベルとフォルダボタンを横並びに配置
  QWidget* pathWidget = new QWidget(this);
  QHBoxLayout* pathLayout = new QHBoxLayout(pathWidget);
  pathLayout->setContentsMargins(0, 0, 0, 0);
  pathLayout->setSpacing(0);

  m_pathLabel = new QLabel(this);
  m_pathLabel->setStyleSheet("QLabel { background-color: #e0e0e0; padding: 2px 5px; }");
  pathLayout->addWidget(m_pathLabel, 1);

  m_folderButton = new QToolButton(this);
  m_folderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  m_folderButton->setToolTip(tr("Browse folder..."));
  m_folderButton->setStyleSheet("QToolButton { background-color: #e0e0e0; border: none; padding: 2px; }");
  connect(m_folderButton, &QToolButton::clicked, this, &FileListPane::onFolderButtonClicked);
  pathLayout->addWidget(m_folderButton);

  mainLayout->addWidget(pathWidget);

  // テーブルビュー
  m_view = new QTableView(this);
  m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_view->setSelectionMode(QAbstractItemView::NoSelection);
  m_view->setAlternatingRowColors(true);
  m_view->horizontalHeader()->setStretchLastSection(true);
  m_view->horizontalHeader()->setSectionsClickable(true);
  m_view->horizontalHeader()->setSortIndicatorShown(true);
  m_view->verticalHeader()->setVisible(false);

  m_model = new FileListModel(this);
  m_view->setModel(m_model);

  m_delegate = new FileListDelegate(this);
  m_view->setItemDelegate(m_delegate);

  connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &FileListPane::onCurrentChanged);

  connect(m_view->horizontalHeader(), &QHeaderView::sectionClicked,
          this, &FileListPane::onHeaderClicked);

  m_view->setColumnWidth(FileListModel::Name, 250);
  m_view->setColumnWidth(FileListModel::Type, 80);
  m_view->setColumnWidth(FileListModel::Size, 100);
  m_view->setColumnWidth(FileListModel::LastModified, 150);

  mainLayout->addWidget(m_view);

  // ソート・フィルタ条件の現在値をリスト下部に表示
  m_sortFilterStatusLabel = new QLabel(this);
  m_sortFilterStatusLabel->setStyleSheet(
    "QLabel { background-color: #e8e8e8; color: #333; padding: 2px 5px; }"
  );
  m_sortFilterStatusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  QFont statusFont = m_sortFilterStatusLabel->font();
  statusFont.setPointSize(qMax(statusFont.pointSize() - 1, 9));
  m_sortFilterStatusLabel->setFont(statusFont);
  mainLayout->addWidget(m_sortFilterStatusLabel);

  refreshSortFilterStatus();
}

QString FileListPane::currentPath() const {
  return m_model->currentPath();
}

bool FileListPane::setPath(const QString& path) {
  bool result = m_model->setPath(path);
  if (result) {
    m_pathLabel->setText(m_model->currentPath());
    // カーソルを先頭に移動
    if (m_model->rowCount() > 0) {
      m_view->setCurrentIndex(m_model->index(0, 0));
    }

    // 現在のソート状態をヘッダーに反映
    int section = -1;
    switch (m_model->sortKey()) {
      case SortKey::None:
        // None is not a valid primary sort key, don't set indicator
        break;
      case SortKey::Name:
        section = FileListModel::Name;
        break;
      case SortKey::Size:
        section = FileListModel::Size;
        break;
      case SortKey::Type:
        section = FileListModel::Type;
        break;
      case SortKey::LastModified:
        section = FileListModel::LastModified;
        break;
    }
    if (section >= 0) {
      m_view->horizontalHeader()->setSortIndicator(section, m_model->sortOrder());
    }

    refreshSortFilterStatus();
  }
  return result;
}

namespace {

QString sortKeyLabel(SortKey k) {
  switch (k) {
    case SortKey::Name:         return QObject::tr("Name");
    case SortKey::Size:         return QObject::tr("Size");
    case SortKey::Type:         return QObject::tr("Type");
    case SortKey::LastModified: return QObject::tr("Modified");
    case SortKey::None:         return {};
  }
  return {};
}

QString dirsPlacementLabel(SortDirsType t) {
  switch (t) {
    case SortDirsType::First: return QObject::tr("Dirs First");
    case SortDirsType::Last:  return QObject::tr("Dirs Last");
    case SortDirsType::Mixed: return QObject::tr("Mixed");
  }
  return {};
}

} // anonymous namespace

void FileListPane::refreshSortFilterStatus() {
  if (!m_sortFilterStatusLabel || !m_model) return;

  const QString arrow = (m_model->sortOrder() == Qt::AscendingOrder)
                        ? QStringLiteral("↑")
                        : QStringLiteral("↓");

  // Sort 部
  QStringList sortKeys;
  sortKeys << QString("%1 %2").arg(sortKeyLabel(m_model->sortKey()), arrow);
  if (m_model->sortKey2nd() != SortKey::None) {
    sortKeys << QString("%1 %2").arg(sortKeyLabel(m_model->sortKey2nd()), arrow);
  }

  QStringList sortTail;
  sortTail << dirsPlacementLabel(m_model->sortDirsType());
  if (m_model->sortCS() == Qt::CaseSensitive) sortTail << tr("CS");
  if (m_model->sortDotFirst())                sortTail << tr("Dot-first");

  const QString sortStr = sortKeys.join(" / ") + QStringLiteral(" · ") + sortTail.join(" · ");

  // Filter 部
  QStringList filterParts;
  AttrFilterFlags attr = m_model->attrFilter();
  if (attr & AttrFilter::ShowHidden) filterParts << tr("Hidden");
  if (attr & AttrFilter::DirsOnly)   filterParts << tr("Dirs only");
  if (attr & AttrFilter::FilesOnly)  filterParts << tr("Files only");

  const QStringList names = m_model->nameFilters();
  if (!names.isEmpty()) {
    filterParts << names.join(' ');
  }

  const QString filterStr = filterParts.isEmpty() ? tr("(none)") : filterParts.join(" · ");

  m_sortFilterStatusLabel->setText(tr("Sort: %1  │  Filter: %2").arg(sortStr, filterStr));
}

void FileListPane::setActive(bool active) {
  m_delegate->setActive(active);
  m_view->viewport()->update();
}

void FileListPane::onFolderButtonClicked() {
  emit folderButtonClicked();
}

void FileListPane::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  emit currentChanged(current, previous);
}

void FileListPane::onHeaderClicked(int section) {
  // 列番号をSortKeyに変換
  SortKey sortKey;
  switch (section) {
    case FileListModel::Name:
      sortKey = SortKey::Name;
      break;
    case FileListModel::Size:
      sortKey = SortKey::Size;
      break;
    case FileListModel::Type:
      sortKey = SortKey::Type;
      break;
    case FileListModel::LastModified:
      sortKey = SortKey::LastModified;
      break;
    default:
      return;  // 無効な列
  }

  // 同じ列がクリックされた場合はソート順を反転
  Qt::SortOrder newOrder = Qt::AscendingOrder;
  if (m_model->sortKey() == sortKey && m_model->sortOrder() == Qt::AscendingOrder) {
    newOrder = Qt::DescendingOrder;
  }

  // ソート設定を更新（第2ソートキーは現在の値を保持）
  m_model->setSortSettings(sortKey, newOrder, m_model->sortKey2nd());

  // ヘッダーにソートインジケーターを表示
  m_view->horizontalHeader()->setSortIndicator(section, newOrder);

  refreshSortFilterStatus();
}

} // namespace Farman

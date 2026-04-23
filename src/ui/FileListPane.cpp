#include "FileListPane.h"
#include "FileListDelegate.h"
#include "ClickableLabel.h"
#include "model/FileListModel.h"
#include "settings/Settings.h"
#include "core/BookmarkManager.h"
#include "types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QTableView>
#include <QHeaderView>
#include <QStyle>
#include <QInputDialog>
#include <QLineEdit>
#include <QDir>
#include <QFileInfo>

namespace Farman {

FileListPane::FileListPane(QWidget* parent)
  : QWidget(parent)
  , m_pathLabel(nullptr)
  , m_bookmarkLabel(nullptr)
  , m_folderButton(nullptr)
  , m_view(nullptr)
  , m_sortFilterStatusLabel(nullptr)
  , m_model(nullptr)
  , m_delegate(nullptr) {

  setupUi();

  connect(&BookmarkManager::instance(), &BookmarkManager::bookmarksChanged,
          this, &FileListPane::refreshBookmarkIndicator);
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

  // ブックマーク登録/解除トグル用の★ラベル。常に表示。
  // QLabel 派生にすることで隣の pathLabel と同じ高さ・paddingに揃う。
  // 登録状態はスタイルシートで色分けする（refreshBookmarkIndicator）。
  m_bookmarkLabel = new ClickableLabel(this);
  m_bookmarkLabel->setText(QStringLiteral("★"));
  m_bookmarkLabel->setCursor(Qt::PointingHandCursor);
  connect(m_bookmarkLabel, &ClickableLabel::clicked,
          this, &FileListPane::onBookmarkButtonClicked);
  pathLayout->addWidget(m_bookmarkLabel, 0);

  m_pathLabel = new QLabel(this);
  pathLayout->addWidget(m_pathLabel, 1);

  m_folderButton = new QToolButton(this);
  m_folderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  m_folderButton->setToolTip(tr("Browse folder..."));
  connect(m_folderButton, &QToolButton::clicked, this, &FileListPane::onFolderButtonClicked);
  pathLayout->addWidget(m_folderButton);

  mainLayout->addWidget(pathWidget);

  // テーブルビュー
  m_view = new QTableView(this);
  m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_view->setSelectionMode(QAbstractItemView::NoSelection);
  m_view->setAlternatingRowColors(false);
  m_view->setFrameShape(QFrame::NoFrame);
  m_view->setShowGrid(false);
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
  refreshAppearance();
}

void FileListPane::refreshAppearance() {
  const QColor fg = Settings::instance().pathForeground();
  const QColor bg = Settings::instance().pathBackground();
  const QString pathStyle = QString("QLabel { color: %1; background-color: %2; padding: 2px 5px; }")
                              .arg(fg.name(), bg.name());
  const QString buttonStyle = QString("QToolButton { background-color: %1; border: none; padding: 2px; }")
                                .arg(bg.name());
  if (m_pathLabel)    m_pathLabel->setStyleSheet(pathStyle);
  if (m_folderButton) m_folderButton->setStyleSheet(buttonStyle);
  if (m_view)         m_view->viewport()->update();  // cursor 再描画

  // ブックマークボタンは登録状態で色が変わるため、実スタイル適用は
  // refreshBookmarkIndicator() 側で行う。背景色だけは揃えておきたいので
  // ここで呼び出しておく。
  refreshBookmarkIndicator();
}

QString FileListPane::currentPath() const {
  return m_model->currentPath();
}

bool FileListPane::setPath(const QString& path) {
  bool result = m_model->setPath(path);
  if (result) {
    m_pathLabel->setText(m_model->currentPath());
    refreshBookmarkIndicator();
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
  m_model->setActive(active);
  m_view->viewport()->update();
}

void FileListPane::onFolderButtonClicked() {
  emit folderButtonClicked();
}

void FileListPane::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  emit currentChanged(current, previous);
}

void FileListPane::refreshBookmarkIndicator() {
  if (!m_bookmarkLabel) return;
  const QString path = currentPath();
  const bool marked = !path.isEmpty() && BookmarkManager::instance().contains(path);

  const QColor bg = Settings::instance().pathBackground();
  // 登録済み: ゴールド / 未登録: 背景に溶け込むグレーで無効化表示。
  // padding は pathLabel と揃え、left だけ広めにしてアイコン然と見せる。
  const QString color = marked ? QStringLiteral("#d4a017")
                               : QStringLiteral("#bdbdbd");
  const QString style = QString(
    "QLabel { color: %1; background-color: %2; padding: 2px 2px 2px 5px; }")
    .arg(color, bg.name());
  m_bookmarkLabel->setStyleSheet(style);

  // デフォルトブックマークは削除不可のため tooltip を分ける
  bool isDefault = false;
  if (marked) {
    const int idx = BookmarkManager::instance().findByPath(path);
    if (idx >= 0) {
      const QList<Bookmark> all = BookmarkManager::instance().bookmarks();
      if (idx < all.size()) isDefault = all[idx].isDefault;
    }
  }
  QString tip;
  if (!marked)         tip = tr("Add bookmark for this directory");
  else if (isDefault)  tip = tr("Default bookmark (cannot be removed)");
  else                 tip = tr("Remove bookmark for this directory");
  m_bookmarkLabel->setToolTip(tip);
}

void FileListPane::onBookmarkButtonClicked() {
  toggleBookmarkForCurrentPath();
}

void FileListPane::toggleBookmarkForCurrentPath() {
  const QString path = currentPath();
  if (path.isEmpty()) return;

  const int existing = BookmarkManager::instance().findByPath(path);
  if (existing >= 0) {
    // 登録済み → 即削除
    BookmarkManager::instance().removeAt(existing);
    return;
  }

  // 未登録 → 名前入力ダイアログ（デフォルト名はパスの末尾）
  QString defaultName;
  if (path == QDir::homePath()) {
    defaultName = tr("Home");
  } else {
    defaultName = QFileInfo(path).fileName();
    if (defaultName.isEmpty()) defaultName = path;
  }

  bool ok = false;
  const QString name = QInputDialog::getText(
    this, tr("Add Bookmark"),
    tr("Name for %1:").arg(path),
    QLineEdit::Normal, defaultName, &ok);
  if (!ok) return;
  BookmarkManager::instance().add(name, path);
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

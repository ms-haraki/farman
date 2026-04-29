#include "FileListPane.h"
#include "FileListDelegate.h"
#include "FileListView.h"
#include "ClickableLabel.h"
#include "core/FileItem.h"
#include "BookmarkEditDialog.h"
#include "model/FileListModel.h"
#include "settings/Settings.h"
#include "core/BookmarkManager.h"
#include "utils/Dialogs.h"
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
#include <QFileSystemWatcher>
#include <QTimer>

namespace Farman {

FileListPane::FileListPane(QWidget* parent)
  : QWidget(parent)
  , m_addressLabel(nullptr)
  , m_bookmarkLabel(nullptr)
  , m_folderButton(nullptr)
  , m_view(nullptr)
  , m_sortFilterStatusLabel(nullptr)
  , m_model(nullptr)
  , m_delegate(nullptr) {

  setupUi();

  connect(&BookmarkManager::instance(), &BookmarkManager::bookmarksChanged,
          this, &FileListPane::refreshBookmarkIndicator);

  // 外部 (Finder 等) からのカレントディレクトリ変更を検知して自動更新する。
  // QFileSystemWatcher の directoryChanged は短時間に複数回飛ぶことがあるので
  // 単発タイマーで debounce してから model を refresh する。
  m_dirWatcher = new QFileSystemWatcher(this);
  m_refreshDebounce = new QTimer(this);
  m_refreshDebounce->setSingleShot(true);
  m_refreshDebounce->setInterval(150);
  connect(m_dirWatcher, &QFileSystemWatcher::directoryChanged, this,
          [this](const QString&) { m_refreshDebounce->start(); });
  connect(m_refreshDebounce, &QTimer::timeout, this,
          &FileListPane::onExternalDirectoryChanged);
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

  m_addressLabel = new QLabel(this);
  // パスが長くてもペイン幅を押し広げないよう、水平方向の推奨幅は無視させる。
  // テキストは end を elide で表示して親幅に収まるようにする。
  m_addressLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_addressLabel->setMinimumWidth(0);
  m_addressLabel->setTextFormat(Qt::PlainText);
  pathLayout->addWidget(m_addressLabel, 1);

  m_folderButton = new QToolButton(this);
  m_folderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  m_folderButton->setToolTip(tr("Browse folder..."));
  connect(m_folderButton, &QToolButton::clicked, this, &FileListPane::onFolderButtonClicked);
  pathLayout->addWidget(m_folderButton);

  mainLayout->addWidget(pathWidget);

  // テーブルビュー (D&D 対応の FileListView を使う)
  m_view = new FileListView(this);
  // Drag-out 用の URL プロバイダ。farman の選択状態 (FileItem::isSelected) を
  // 優先し、選択が無ければカーソル行 1 件を返す。'..' は除外。
  m_view->setUrlsProvider([this]() -> QList<QUrl> {
    QList<QUrl> urls;
    if (!m_model) return urls;
    bool anySelected = false;
    const int rows = m_model->rowCount();
    for (int i = 0; i < rows; ++i) {
      const FileItem* it = m_model->itemAt(i);
      if (it && it->isSelected()) { anySelected = true; break; }
    }
    if (anySelected) {
      for (int i = 0; i < rows; ++i) {
        const FileItem* it = m_model->itemAt(i);
        if (it && it->isSelected() && !it->isDotDot()) {
          urls.append(QUrl::fromLocalFile(it->absolutePath()));
        }
      }
    } else {
      const QModelIndex cur = m_view->currentIndex();
      if (cur.isValid()) {
        const FileItem* it = m_model->itemAt(cur.row());
        if (it && !it->isDotDot()) {
          urls.append(QUrl::fromLocalFile(it->absolutePath()));
        }
      }
    }
    return urls;
  });
  // Drop 受信時は FileManagerPanel まで bubble させる
  connect(m_view, &FileListView::externalUrlsDropped, this,
          &FileListPane::externalUrlsDropped);
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

  // 構築時の Qt 既定の行高を覚えておく。Settings で行高 = 0 (Auto) を
  // 選んだときにこの値に戻す。
  m_defaultRowHeight = m_view->verticalHeader()->defaultSectionSize();

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
  const QColor fg = Settings::instance().addressForeground();
  const QColor bg = Settings::instance().addressBackground();
  const QString addressStyle = QString("QLabel { color: %1; background-color: %2; padding: 2px 5px; }")
                                 .arg(fg.name(), bg.name());
  const QString buttonStyle = QString("QToolButton { background-color: %1; border: none; padding: 2px; }")
                                .arg(bg.name());
  if (m_addressLabel) {
    m_addressLabel->setStyleSheet(addressStyle);
    m_addressLabel->setFont(Settings::instance().addressFont());
  }
  if (m_folderButton) m_folderButton->setStyleSheet(buttonStyle);
  if (m_view) {
    m_view->setFont(Settings::instance().font());
    // Settings の行高を反映 (0 = Auto → 構築時のデフォルトに戻す)
    const int customH = Settings::instance().fileListRowHeight();
    const int targetH = (customH > 0) ? customH : m_defaultRowHeight;
    if (targetH > 0) {
      m_view->verticalHeader()->setDefaultSectionSize(targetH);
    }
    m_view->viewport()->update();  // cursor 再描画
  }

  // ブックマークボタンは登録状態で色が変わるため、実スタイル適用は
  // refreshBookmarkIndicator() 側で行う。背景色だけは揃えておきたいので
  // ここで呼び出しておく。
  refreshBookmarkIndicator();
}

QString FileListPane::currentPath() const {
  return m_model->currentPath();
}

QTableView* FileListPane::view() const {
  return m_view;
}

void FileListPane::onExternalDirectoryChanged() {
  if (!m_model) return;
  // 外部からファイル/ディレクトリが追加・削除されたあと、model を再読み込み。
  // model::refresh() は beginResetModel/endResetModel を呼ぶため currentIndex
  // が無効化されるので、ファイル名ベースでカーソルを保存・復元する。
  QString cursorName;
  int     cursorRow = -1;
  if (m_view) {
    const QModelIndex idx = m_view->currentIndex();
    if (idx.isValid()) {
      cursorRow = idx.row();
      const QVariant v = m_model->data(m_model->index(idx.row(), FileListModel::Name));
      cursorName = v.toString();
    }
  }

  m_model->refresh();
  refreshSortFilterStatus();

  if (m_view) {
    const int rows = m_model->rowCount();
    if (rows > 0) {
      int target = -1;
      if (!cursorName.isEmpty()) {
        for (int r = 0; r < rows; ++r) {
          if (m_model->data(m_model->index(r, FileListModel::Name)).toString()
              == cursorName) {
            target = r;
            break;
          }
        }
      }
      if (target < 0) target = qBound(0, cursorRow, rows - 1);
      m_view->setCurrentIndex(m_model->index(target, FileListModel::Name));
    }
  }
}

bool FileListPane::setPath(const QString& path) {
  bool result = m_model->setPath(path);
  if (result) {
    m_addressLabel->setText(m_model->currentPath());
    refreshBookmarkIndicator();
    // 外部変更ウォッチャの監視対象を新しいパスに切り替える
    if (m_dirWatcher) {
      const QStringList prev = m_dirWatcher->directories();
      if (!prev.isEmpty()) m_dirWatcher->removePaths(prev);
      const QString cur = m_model->currentPath();
      if (!cur.isEmpty()) m_dirWatcher->addPath(cur);
    }
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

  const QColor bg = Settings::instance().addressBackground();
  // 登録済み: ゴールド / 未登録: 背景に溶け込むグレーで無効化表示。
  // padding はアドレスラベルと揃え、left だけ広めにしてアイコン然と見せる。
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

  // 未登録 → BookmarkEditDialog で追加（Edit と同じフォーム、パスも編集可能）
  QString defaultName;
  if (path == QDir::homePath()) {
    defaultName = tr("Home");
  } else {
    defaultName = QFileInfo(path).fileName();
    if (defaultName.isEmpty()) defaultName = path;
  }

  BookmarkEditDialog dlg(defaultName, path, this);
  dlg.setWindowTitle(tr("Add Bookmark"));
  if (dlg.exec() != QDialog::Accepted) return;
  BookmarkManager::instance().add(dlg.name(), dlg.path());
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

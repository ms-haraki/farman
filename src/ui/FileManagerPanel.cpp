#include "FileManagerPanel.h"
#include "FileListPane.h"
#include "LogPane.h"
#include "ProgressDialog.h"
#include "core/Logger.h"
#include "SortFilterDialog.h"
#include "TransferConfirmDialog.h"
#include "OverwriteDialog.h"
#include "AttributesDialog.h"
#include "BulkRenameDialog.h"
#include "DeleteConfirmDialog.h"
#include "CreateArchiveDialog.h"
#include "ExtractArchiveDialog.h"
#include "core/workers/ArchiveCreateWorker.h"
#include "core/workers/ArchiveExtractWorker.h"
#include "model/FileListModel.h"
#include "core/FileItem.h"
#include "core/workers/CopyWorker.h"
#include "core/workers/MoveWorker.h"
#include "core/workers/RemoveWorker.h"
#include "settings/Settings.h"
#include "utils/Dialogs.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QKeyEvent>
#include <QTableView>
#include <QHeaderView>
#include <QFile>

namespace Farman {

FileManagerPanel::FileManagerPanel(QWidget* parent)
  : QWidget(parent)
  , m_splitter(nullptr)
  , m_leftPane(nullptr)
  , m_rightPane(nullptr)
  , m_activePane(PaneType::Left)
  , m_singlePaneMode(false) {

  setupUi();
}

FileManagerPanel::~FileManagerPanel() = default;

void FileManagerPanel::setupUi() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // Splitter for two panes
  m_splitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_splitter);

  // ===== Left Pane =====
  m_leftPane = new FileListPane(this);
  connect(m_leftPane, &FileListPane::currentChanged, this, &FileManagerPanel::onLeftPaneCurrentChanged);
  connect(m_leftPane, &FileListPane::folderButtonClicked, this, &FileManagerPanel::onLeftFolderButtonClicked);
  // ステータスバー更新: カーソル移動・モデル変化・選択変化を監視
  connect(m_leftPane, &FileListPane::currentChanged, this, [this](const QModelIndex&, const QModelIndex&) {
    emitActivePaneStatus();
  });
  connect(m_leftPane->model(), &QAbstractItemModel::dataChanged, this, [this](auto&&...) {
    emitActivePaneStatus();
  });
  connect(m_leftPane->model(), &QAbstractItemModel::modelReset, this, [this] {
    emitActivePaneStatus();
  });
  m_splitter->addWidget(m_leftPane);

  // ===== Right Pane =====
  m_rightPane = new FileListPane(this);
  connect(m_rightPane, &FileListPane::currentChanged, this, &FileManagerPanel::onRightPaneCurrentChanged);
  connect(m_rightPane, &FileListPane::folderButtonClicked, this, &FileManagerPanel::onRightFolderButtonClicked);
  connect(m_rightPane, &FileListPane::currentChanged, this, [this](const QModelIndex&, const QModelIndex&) {
    emitActivePaneStatus();
  });
  connect(m_rightPane->model(), &QAbstractItemModel::dataChanged, this, [this](auto&&...) {
    emitActivePaneStatus();
  });
  connect(m_rightPane->model(), &QAbstractItemModel::modelReset, this, [this] {
    emitActivePaneStatus();
  });
  m_splitter->addWidget(m_rightPane);

  // Splitterのサイズを均等に
  m_splitter->setSizes(QList<int>() << 600 << 600);

  // ===== Log Pane =====
  m_logPane = new LogPane(this);
  m_logPane->setVisible(Settings::instance().logVisible());
  setLogPaneHeight(Settings::instance().logPaneHeight());
  layout->addWidget(m_logPane);
}

void FileManagerPanel::setLogPaneVisible(bool visible) {
  if (m_logPane) m_logPane->setVisible(visible);
}

bool FileManagerPanel::isLogPaneVisible() const {
  return m_logPane && m_logPane->isVisible();
}

void FileManagerPanel::setLogPaneHeight(int px) {
  if (!m_logPane) return;
  if (px < 40) px = 40;
  m_logPane->setFixedHeight(px);
}

void FileManagerPanel::loadInitialPath() {
  auto& settings = Settings::instance();

  auto resolveInitialPath = [&](PaneType pane) -> QString {
    const QString home = QDir::homePath();
    switch (settings.initialPathMode(pane)) {
      case InitialPathMode::Default:
        return home;
      case InitialPathMode::LastSession: {
        const QString last = settings.paneSettings(pane).path;
        return (!last.isEmpty() && QDir(last).exists()) ? last : home;
      }
      case InitialPathMode::Custom: {
        const QString custom = settings.customInitialPath(pane);
        return (!custom.isEmpty() && QDir(custom).exists()) ? custom : home;
      }
    }
    return home;
  };

  navigatePane(PaneType::Left,  resolveInitialPath(PaneType::Left));
  navigatePane(PaneType::Right, resolveInitialPath(PaneType::Right));

  // 左ペインをアクティブに
  setActivePane(PaneType::Left);

  updatePathSignal();
}

void FileManagerPanel::applySettings() {
  // Behavior タブでデフォルト設定が変更された際の再適用。
  // setSortSettings / setAttrFilter / refresh は beginResetModel/endResetModel を
  // 呼ぶため selection model の currentIndex が無効化され、戻ってきたときに
  // カーソル下線が消えてしまう。前後でファイル名ベースに保存・復元する。
  auto saveCursor = [](FileListPane* pane) -> QString {
    auto* view = pane->view();
    const QModelIndex idx = view->currentIndex();
    if (!idx.isValid()) return {};
    return pane->model()->data(pane->model()->index(idx.row(), FileListModel::Name)).toString();
  };
  auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
    auto* view  = pane->view();
    auto* model = pane->model();
    const int rows = model->rowCount();
    if (rows == 0) return;
    int target = -1;
    if (!fileName.isEmpty()) {
      for (int r = 0; r < rows; ++r) {
        if (model->data(model->index(r, FileListModel::Name)).toString() == fileName) {
          target = r;
          break;
        }
      }
    }
    if (target < 0) target = qBound(0, fallbackRow, rows - 1);
    const QModelIndex idx = model->index(target, FileListModel::Name);
    view->setCurrentIndex(idx);
  };

  const QString leftName  = saveCursor(m_leftPane);
  const QString rightName = saveCursor(m_rightPane);
  const int leftRow  = m_leftPane->view()->currentIndex().row();
  const int rightRow = m_rightPane->view()->currentIndex().row();

  // パス単位の override があればそちらを優先する。
  applyPathSortFilter(PaneType::Left);
  applyPathSortFilter(PaneType::Right);

  // パス表示・カーソル色など外観の再適用
  m_leftPane->refreshAppearance();
  m_rightPane->refreshAppearance();

  m_leftPane->model()->refresh();
  m_rightPane->model()->refresh();

  restoreCursor(m_leftPane,  leftName,  leftRow);
  restoreCursor(m_rightPane, rightName, rightRow);

  // ダイアログから戻った後にアクティブペインへフォーカスを戻す
  activePane()->view()->setFocus();
}

void FileManagerPanel::applyPathSortFilter(PaneType paneType) {
  FileListPane* pane = (paneType == PaneType::Left) ? m_leftPane : m_rightPane;
  FileListModel* model = pane->model();
  auto& settings = Settings::instance();
  const QString path = pane->currentPath();

  PaneSettings s = settings.hasPathOverride(path)
                   ? settings.pathOverride(path)
                   : settings.paneSettings(paneType);

  model->setSortSettings(
    s.sortKey, s.sortOrder, s.sortKey2nd,
    s.sortDirsType, s.sortDotFirst, s.sortCS
  );
  model->setAttrFilter(s.attrFilter);
  model->setNameFilters(s.nameFilters);

  // Header sort indicator を現在のソート状態に合わせる
  int section = -1;
  switch (s.sortKey) {
    case SortKey::Name:         section = FileListModel::Name;         break;
    case SortKey::Size:         section = FileListModel::Size;         break;
    case SortKey::Type:         section = FileListModel::Type;         break;
    case SortKey::LastModified: section = FileListModel::LastModified; break;
    case SortKey::None: break;
  }
  if (section >= 0) {
    pane->view()->horizontalHeader()->setSortIndicator(section, s.sortOrder);
  }

  pane->refreshSortFilterStatus();
}

bool FileManagerPanel::navigatePane(PaneType paneType, const QString& path) {
  FileListPane* pane = (paneType == PaneType::Left) ? m_leftPane : m_rightPane;
  FileListModel* model = pane->model();
  auto& settings = Settings::instance();

  PaneSettings s = settings.hasPathOverride(path)
                   ? settings.pathOverride(path)
                   : settings.paneSettings(paneType);

  model->setSortSettings(
    s.sortKey, s.sortOrder, s.sortKey2nd,
    s.sortDirsType, s.sortDotFirst, s.sortCS
  );
  model->setAttrFilter(s.attrFilter);
  model->setNameFilters(s.nameFilters);

  const bool ok = pane->setPath(path);
  if (ok) {
    DirectoryHistory& hist = (paneType == PaneType::Left) ? m_leftHistory : m_rightHistory;
    hist.push(pane->currentPath());
    Logger::instance().info(QStringLiteral("%1 pane: %2")
      .arg(paneType == PaneType::Left ? QStringLiteral("Left") : QStringLiteral("Right"))
      .arg(pane->currentPath()));
  }
  return ok;
}

DirectoryHistory& FileManagerPanel::history(PaneType pane) {
  return (pane == PaneType::Left) ? m_leftHistory : m_rightHistory;
}

const DirectoryHistory& FileManagerPanel::history(PaneType pane) const {
  return (pane == PaneType::Left) ? m_leftHistory : m_rightHistory;
}

bool FileManagerPanel::navigateActivePaneTo(const QString& path) {
  if (path.isEmpty()) return false;

  // パスがファイルなら、その親ディレクトリへ遷移したうえでカーソルを
  // そのファイルに合わせる。ディレクトリならそのまま遷移。
  QFileInfo info(path);
  QString dirPath = path;
  QString fileName;
  if (info.exists() && !info.isDir()) {
    dirPath  = info.absolutePath();
    fileName = info.fileName();
  }

  if (!navigatePane(m_activePane, dirPath)) return false;

  if (!fileName.isEmpty()) {
    FileListPane* pane = activePane();
    FileListModel* model = pane->model();
    for (int i = 0; i < model->rowCount(); ++i) {
      const FileItem* item = model->itemAt(i);
      if (item && item->name() == fileName) {
        pane->view()->setCurrentIndex(model->index(i, 0));
        break;
      }
    }
  }
  updatePathSignal();
  return true;
}

void FileManagerPanel::syncOtherToActive() {
  const PaneType other = (m_activePane == PaneType::Left) ? PaneType::Right : PaneType::Left;
  const QString path = activePane()->currentPath();
  if (path.isEmpty()) return;
  navigatePane(other, path);
  updatePathSignal();
}

void FileManagerPanel::syncActiveToOther() {
  FileListPane* otherPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;
  const QString path = otherPane->currentPath();
  if (path.isEmpty()) return;
  navigatePane(m_activePane, path);
  updatePathSignal();
}

bool FileManagerPanel::handleKeyEvent(QKeyEvent* event) {
  // Tabキーでペイン切り替え（2ペイン表示時のみ）
  if (event->key() == Qt::Key_Tab && !m_singlePaneMode) {
    handleTabKey();
    return true;
  }

  // 左キーの処理
  if (event->key() == Qt::Key_Left) {
    if (m_singlePaneMode) {
      // シングルペインモード: 親ディレクトリに移動
      handleBackspaceKey();
      return true;
    } else {
      // 2ペインモード
      if (m_activePane == PaneType::Left) {
        // 左ペインで←キー: 親ディレクトリに移動
        handleBackspaceKey();
        return true;
      } else {
        // 右ペインで←キー: 左ペインに切り替え
        setActivePane(PaneType::Left);
        return true;
      }
    }
  }

  // 右キーの処理
  if (event->key() == Qt::Key_Right) {
    if (m_singlePaneMode) {
      // シングルペインモード: 何もしない
      return true;
    } else {
      // 2ペインモード
      if (m_activePane == PaneType::Right) {
        // 右ペインで→キー: 親ディレクトリに移動
        handleBackspaceKey();
        return true;
      } else {
        // 左ペインで→キー: 右ペインに切り替え
        setActivePane(PaneType::Right);
        return true;
      }
    }
  }

  // Ctrl+A (Windows/Linux) or Cmd+A (Mac) for select all
  if ((event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::MetaModifier)
      && event->key() == Qt::Key_A) {
    handleSelectAllKey();
    return true;
  }

  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  switch (event->key()) {
    case Qt::Key_Up: {
      QModelIndex current = pane->view()->currentIndex();
      if (!current.isValid()) {
        return true;
      }
      int rows = model->rowCount();
      if (current.row() > 0) {
        pane->view()->setCurrentIndex(model->index(current.row() - 1, 0));
      } else if (Settings::instance().cursorLoop() && rows > 0) {
        pane->view()->setCurrentIndex(model->index(rows - 1, 0));
      }
      return true;
    }

    case Qt::Key_Down: {
      QModelIndex current = pane->view()->currentIndex();
      if (!current.isValid()) {
        return true;
      }
      int rows = model->rowCount();
      if (current.row() < rows - 1) {
        pane->view()->setCurrentIndex(model->index(current.row() + 1, 0));
      } else if (Settings::instance().cursorLoop() && rows > 0) {
        pane->view()->setCurrentIndex(model->index(0, 0));
      }
      return true;
    }

    case Qt::Key_Home: {
      if (model->rowCount() > 0) {
        pane->view()->setCurrentIndex(model->index(0, 0));
      }
      return true;
    }

    case Qt::Key_End: {
      int lastRow = model->rowCount() - 1;
      if (lastRow >= 0) {
        pane->view()->setCurrentIndex(model->index(lastRow, 0));
      }
      return true;
    }

    case Qt::Key_Return:
    case Qt::Key_Enter:
      handleEnterKey();
      return true;

    case Qt::Key_Backspace:
      handleBackspaceKey();
      return true;

    case Qt::Key_Space:
      handleSpaceKey();
      return true;

    case Qt::Key_Asterisk:
      handleAsteriskKey();
      return true;

    default:
      break;
  }

  // 印字可能な文字キー: Shift キー押下時のみ頭文字検索を発動させる
  // (Qt 標準の keyboardSearch は短時間内の入力を蓄積するため連打時に挙動が崩れる)
  if (!event->text().isEmpty()) {
    const QChar ch = event->text().at(0);
    if (ch.isPrint()) {
      if (event->modifiers() & Qt::ShiftModifier) {
        const int rowCount = model->rowCount();
        if (rowCount > 0) {
          const int startRow = pane->view()->currentIndex().isValid()
                                   ? pane->view()->currentIndex().row()
                                   : -1;
          for (int i = 1; i <= rowCount; ++i) {
            const int row = (startRow + i) % rowCount;
            const QString name = model->data(model->index(row, 0), Qt::DisplayRole).toString();
            bool match = name.startsWith(ch, Qt::CaseInsensitive);
            // ドットから始まるファイル/ディレクトリ (.gitignore, .claude 等) は
            // 設定で有効な場合のみ 2 文字目もマッチ対象にする
            if (!match && Settings::instance().typeAheadIncludeDotfiles()
                && name.size() >= 2 && name.at(0) == QLatin1Char('.')) {
              match = name.at(1).toLower() == ch.toLower();
            }
            if (match) {
              pane->view()->setCurrentIndex(model->index(row, 0));
              break;
            }
          }
        }
      }
      // Shift の有無に関わらず文字キーは消費 (Qt 標準の検索を発動させない)
      return true;
    }
  }

  return false;
}

void FileManagerPanel::onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_leftPane->view()->viewport()->update();
}

void FileManagerPanel::onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(current);
  Q_UNUSED(previous);
  m_rightPane->view()->viewport()->update();
}

void FileManagerPanel::handleEnterKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  const FileItem* item = model->itemAt(currentIndex);
  if (!item) {
    return;
  }

  if (item->isDir()) {
    // ディレクトリに入る
    QString newPath = item->absolutePath();
    if (navigatePane(m_activePane, newPath)) {
      updatePathSignal();
    }
  } else {
    // ファイルの場合はシグナルを発行
    emit fileActivated(item->absolutePath());
  }
}

void FileManagerPanel::handleBackspaceKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QString currentPath = model->currentPath();
  if (currentPath.isEmpty()) {
    return;
  }

  // 親に戻った際にカーソルを元いた子ディレクトリに合わせる
  const QString childDirName = QFileInfo(currentPath).fileName();

  QDir dir(currentPath);
  if (!dir.cdUp()) {
    // ルートディレクトリにいる場合は何もしない
    return;
  }

  QString parentPath = dir.absolutePath();
  if (navigatePane(m_activePane, parentPath)) {
    if (!childDirName.isEmpty()) {
      FileListModel* parentModel = pane->model();
      for (int i = 0; i < parentModel->rowCount(); ++i) {
        const FileItem* item = parentModel->itemAt(i);
        if (item && item->name() == childDirName) {
          pane->view()->setCurrentIndex(parentModel->index(i, 0));
          break;
        }
      }
    }
    updatePathSignal();
  }
}

void FileManagerPanel::handleSpaceKey() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルを次の行に移動
  int nextRow = row + 1;
  if (nextRow < model->rowCount()) {
    QModelIndex nextIndex = model->index(nextRow, 0);
    pane->view()->setCurrentIndex(nextIndex);
  }
}

void FileManagerPanel::toggleSelection() {
  FileListPane* pane = activePane();
  FileListModel* model = pane->model();

  QModelIndex currentIndex = pane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  int row = currentIndex.row();
  model->toggleSelected(row);

  // カーソルは移動しない（Space との違い）
}

void FileManagerPanel::handleAsteriskKey() {
  FileListModel* model = activePane()->model();
  // 選択を反転
  model->invertSelection();
}

void FileManagerPanel::handleSelectAllKey() {
  FileListModel* model = activePane()->model();
  // 全て選択されている場合は全解除、それ以外は全選択
  if (model->isAllSelected()) {
    model->setSelectedAll(false);
  } else {
    model->setSelectedAll(true);
  }
}

void FileManagerPanel::handleTabKey() {
  // アクティブペインを切り替え（2ペイン表示時のみ）
  if (!m_singlePaneMode) {
    if (m_activePane == PaneType::Left) {
      setActivePane(PaneType::Right);
    } else {
      setActivePane(PaneType::Left);
    }
  }
}

void FileManagerPanel::togglePaneMode() {
  setSinglePaneMode(!m_singlePaneMode);
}

void FileManagerPanel::setSinglePaneMode(bool single) {
  m_singlePaneMode = single;

  if (single) {
    // 1ペインモード: 非アクティブなペインを非表示
    if (m_activePane == PaneType::Left) {
      m_rightPane->hide();
      m_leftPane->show();
    } else {
      m_leftPane->hide();
      m_rightPane->show();
    }
  } else {
    // 2ペインモード: 両方のペインを表示
    m_leftPane->show();
    m_rightPane->show();
  }
}

FileListPane* FileManagerPanel::activePane() const {
  return (m_activePane == PaneType::Left) ? m_leftPane : m_rightPane;
}

void FileManagerPanel::setActivePane(PaneType pane) {
  m_activePane = pane;

  // ペインのアクティブ状態を更新
  if (pane == PaneType::Left) {
    m_leftPane->setActive(true);
    m_rightPane->setActive(false);
  } else {
    m_leftPane->setActive(false);
    m_rightPane->setActive(true);
  }

  // アクティブペインにフォーカスを設定
  activePane()->view()->setFocus();

  // ステータスバーをアクティブペインの状態に追従
  emitActivePaneStatus();
}

void FileManagerPanel::emitActivePaneStatus() {
  FileListPane* pane = activePane();
  if (!pane) {
    emit activeFocusedPathChanged(QString());
    emit activeSummaryChanged(QString());
    return;
  }
  FileListModel* model = pane->model();
  if (!model) {
    emit activeFocusedPathChanged(QString());
    emit activeSummaryChanged(QString());
    return;
  }

  // フォーカス中ファイルの絶対パス
  QString focusedPath;
  const QModelIndex idx = pane->view()->currentIndex();
  if (idx.isValid()) {
    if (const FileItem* item = model->itemAt(idx)) {
      focusedPath = item->absolutePath();
    }
  }
  emit activeFocusedPathChanged(focusedPath);

  // 要約: 総アイテム数 ('..' 除外) / 選択数 / 選択合計サイズ
  int totalCount = 0;
  int selectedCount = 0;
  qint64 selectedBytes = 0;
  const int rows = model->rowCount();
  for (int r = 0; r < rows; ++r) {
    const FileItem* item = model->itemAt(r);
    if (!item || item->isDotDot()) continue;
    ++totalCount;
    if (item->isSelected()) {
      ++selectedCount;
      const qint64 sz = item->size();
      if (sz > 0) selectedBytes += sz;
    }
  }
  QString summary;
  if (selectedCount > 0) {
    summary = tr("%1 / %2 items selected (%3)")
                .arg(selectedCount)
                .arg(totalCount)
                .arg(QLocale(QLocale::English).formattedDataSize(selectedBytes));
  } else {
    summary = tr("%1 items").arg(totalCount);
  }
  emit activeSummaryChanged(summary);
}

void FileManagerPanel::onLeftFolderButtonClicked() {
  QString currentPath = m_leftPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (navigatePane(PaneType::Left, selectedPath)) {
      updatePathSignal();
    }
  }
}

void FileManagerPanel::onRightFolderButtonClicked() {
  QString currentPath = m_rightPane->currentPath();
  QString selectedPath = QFileDialog::getExistingDirectory(
    this,
    tr("Select Folder"),
    currentPath,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );

  if (!selectedPath.isEmpty() && selectedPath != currentPath) {
    if (navigatePane(PaneType::Right, selectedPath)) {
      updatePathSignal();
    }
  }
}

QString FileManagerPanel::currentPath() const {
  return activePane()->currentPath();
}

QString FileManagerPanel::leftPath() const {
  return m_leftPane->currentPath();
}

QString FileManagerPanel::rightPath() const {
  return m_rightPane->currentPath();
}

void FileManagerPanel::updatePathSignal() {
  emit pathChanged(m_leftPane->currentPath(), m_rightPane->currentPath());
}

void FileManagerPanel::copySelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;

  FileListModel* srcModel = srcPane->model();
  QString destDir = destPane->currentPath();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // 確認ダイアログ（コピー元・一覧・コピー先・上書きモード）
  TransferConfirmDialog confirm(
    TransferConfirmDialog::Copy,
    srcPane->currentPath(),
    selectedFiles,
    destDir,
    this
  );
  if (confirm.exec() != QDialog::Accepted) {
    return;
  }
  const OverwriteMode overwriteMode = confirm.overwriteMode();

  // Create worker and dialog
  CopyWorker* worker = new CopyWorker(selectedFiles, destDir, this);
  worker->setOverwriteMode(overwriteMode);
  worker->setAutoRenameTemplate(confirm.autoRenameTemplate());
  connect(worker, &WorkerBase::overwriteRequired, this,
    [this](const QString& srcPath, const QString& dstPath, OverwriteDecision* decision) {
      OverwriteDialog dlg(srcPath, dstPath, this);
      dlg.exec();
      *decision = dlg.decision();
    },
    Qt::BlockingQueuedConnection
  );
  ProgressDialog* dialog = new ProgressDialog(tr("Copying files..."), this);
  dialog->setWorker(worker);

  const int copiedCount = selectedFiles.size();
  const QString copiedDest = destDir;

  connect(worker, &WorkerBase::finished, this,
      [this, dialog, srcPane, destPane, copiedCount, copiedDest](bool success) {
    dialog->accept();
    Logger::instance().log(success ? Logger::Info : Logger::Error,
      QStringLiteral("Copy %1: %2 item(s) → %3")
        .arg(success ? QStringLiteral("done") : QStringLiteral("failed"))
        .arg(copiedCount)
        .arg(copiedDest));

    // Save cursor positions
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QModelIndex destCurrentIndex = destPane->view()->currentIndex();
    QString srcCurrentFile;
    QString destCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();
    int destCurrentRow = destCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }
    if (destCurrentIndex.isValid()) {
      const FileItem* item = destPane->model()->itemAt(destCurrentIndex);
      if (item) destCurrentFile = item->name();
    }

    // Refresh both panes
    QString srcPath = srcPane->currentPath();
    QString destPath = destPane->currentPath();
    srcPane->setPath(srcPath);
    destPane->setPath(destPath);

    // Restore cursor positions
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);
    restoreCursor(destPane, destCurrentFile, destCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::moveSelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;

  FileListModel* srcModel = srcPane->model();
  QString destDir = destPane->currentPath();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // 確認ダイアログ
  TransferConfirmDialog confirm(
    TransferConfirmDialog::Move,
    srcPane->currentPath(),
    selectedFiles,
    destDir,
    this
  );
  if (confirm.exec() != QDialog::Accepted) {
    return;
  }
  const OverwriteMode overwriteMode = confirm.overwriteMode();

  // Create worker and dialog
  MoveWorker* worker = new MoveWorker(selectedFiles, destDir, this);
  worker->setOverwriteMode(overwriteMode);
  worker->setAutoRenameTemplate(confirm.autoRenameTemplate());
  connect(worker, &WorkerBase::overwriteRequired, this,
    [this](const QString& srcPath, const QString& dstPath, OverwriteDecision* decision) {
      OverwriteDialog dlg(srcPath, dstPath, this);
      dlg.exec();
      *decision = dlg.decision();
    },
    Qt::BlockingQueuedConnection
  );
  ProgressDialog* dialog = new ProgressDialog(tr("Moving files..."), this);
  dialog->setWorker(worker);

  const int movedCount = selectedFiles.size();
  const QString movedDest = destDir;

  connect(worker, &WorkerBase::finished, this,
      [this, dialog, srcPane, destPane, movedCount, movedDest](bool success) {
    dialog->accept();
    Logger::instance().log(success ? Logger::Info : Logger::Error,
      QStringLiteral("Move %1: %2 item(s) → %3")
        .arg(success ? QStringLiteral("done") : QStringLiteral("failed"))
        .arg(movedCount)
        .arg(movedDest));

    // Save cursor positions
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QModelIndex destCurrentIndex = destPane->view()->currentIndex();
    QString srcCurrentFile;
    QString destCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();
    int destCurrentRow = destCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }
    if (destCurrentIndex.isValid()) {
      const FileItem* item = destPane->model()->itemAt(destCurrentIndex);
      if (item) destCurrentFile = item->name();
    }

    // Refresh both panes
    QString srcPath = srcPane->currentPath();
    QString destPath = destPane->currentPath();
    srcPane->setPath(srcPath);
    destPane->setPath(destPath);

    // Restore cursor positions
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);
    restoreCursor(destPane, destCurrentFile, destCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::deleteSelectedFiles() {
  FileListPane* srcPane = activePane();
  FileListModel* srcModel = srcPane->model();

  // Get selected files, or current item if no selection
  QStringList selectedFiles = srcModel->selectedFilePaths();
  if (selectedFiles.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        selectedFiles.append(item->absolutePath());
      }
    }
  }

  if (selectedFiles.isEmpty()) {
    return;
  }

  // Ask for confirmation
  QString message;
  if (selectedFiles.size() == 1) {
    QFileInfo info(selectedFiles[0]);
    message = tr("Are you sure you want to delete '%1'?").arg(info.fileName());
  } else {
    message = tr("Are you sure you want to delete %1 items?").arg(selectedFiles.size());
  }

  DeleteConfirmDialog confirmDlg(
    message, Settings::instance().defaultDeleteToTrash(), this);
  if (confirmDlg.exec() != QDialog::Accepted) {
    return;
  }
  const bool toTrash = confirmDlg.toTrash();

  // Create worker and dialog
  RemoveWorker* worker = new RemoveWorker(selectedFiles, toTrash, this);
  ProgressDialog* dialog = new ProgressDialog(tr("Deleting files..."), this);
  dialog->setWorker(worker);

  const int delCount = selectedFiles.size();
  const bool delToTrash = toTrash;

  connect(worker, &WorkerBase::finished, this,
      [this, dialog, srcPane, delCount, delToTrash](bool success) {
    dialog->accept();
    Logger::instance().log(success ? Logger::Info : Logger::Error,
      QStringLiteral("%1 %2: %3 item(s)")
        .arg(delToTrash ? QStringLiteral("Trash") : QStringLiteral("Delete"))
        .arg(success ? QStringLiteral("done") : QStringLiteral("failed"))
        .arg(delCount));

    // Save cursor position
    QModelIndex srcCurrentIndex = srcPane->view()->currentIndex();
    QString srcCurrentFile;
    int srcCurrentRow = srcCurrentIndex.row();

    if (srcCurrentIndex.isValid()) {
      const FileItem* item = srcPane->model()->itemAt(srcCurrentIndex);
      if (item) srcCurrentFile = item->name();
    }

    // Refresh the pane
    QString srcPath = srcPane->currentPath();
    srcPane->setPath(srcPath);

    // Restore cursor position
    auto restoreCursor = [](FileListPane* pane, const QString& fileName, int fallbackRow) {
      FileListModel* model = pane->model();

      // Try to find the file by name
      if (!fileName.isEmpty()) {
        for (int i = 0; i < model->rowCount(); ++i) {
          const FileItem* item = model->itemAt(i);
          if (item && item->name() == fileName) {
            pane->view()->setCurrentIndex(model->index(i, 0));
            return;
          }
        }
      }

      // Fallback to row number (clamped to valid range)
      if (model->rowCount() > 0) {
        int row = qMin(fallbackRow, model->rowCount() - 1);
        pane->view()->setCurrentIndex(model->index(row, 0));
      }
    };

    restoreCursor(srcPane, srcCurrentFile, srcCurrentRow);

    // Clear selections
    srcPane->model()->setSelectedAll(false);

    // Restore focus to active pane
    activePane()->view()->setFocus();
  });

  worker->start();
  dialog->exec();
}

void FileManagerPanel::createDirectory() {
  FileListPane* srcPane = activePane();
  QString currentPath = srcPane->currentPath();

  // Ask for directory name
  bool ok = false;
  QString dirName = inputText(
    this,
    tr("Create Directory"),
    tr("Enter directory name:"),
    QString(),
    &ok
  );

  if (!ok || dirName.isEmpty()) {
    return;
  }

  // Create directory
  QString newDirPath = currentPath + "/" + dirName;
  QDir dir;
  if (!dir.mkpath(newDirPath)) {
    Logger::instance().error(QStringLiteral("Mkdir failed: %1").arg(newDirPath));
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("Failed to create directory: %1").arg(dirName)
    );
    return;
  }
  Logger::instance().info(QStringLiteral("Mkdir: %1").arg(newDirPath));

  // Refresh and move cursor to new directory
  srcPane->setPath(currentPath);

  // Find and select the new directory
  FileListModel* model = srcPane->model();
  for (int i = 0; i < model->rowCount(); ++i) {
    const FileItem* item = model->itemAt(i);
    if (item && item->name() == dirName) {
      srcPane->view()->setCurrentIndex(model->index(i, 0));
      break;
    }
  }

  srcPane->view()->setFocus();
}

void FileManagerPanel::createFile() {
  FileListPane* srcPane = activePane();
  QString currentPath = srcPane->currentPath();

  bool ok = false;
  QString fileName = inputText(
    this,
    tr("Create File"),
    tr("Enter file name:"),
    QString(),
    &ok
  );

  if (!ok || fileName.isEmpty()) {
    return;
  }

  const QString newFilePath = currentPath + "/" + fileName;
  if (QFileInfo::exists(newFilePath)) {
    QMessageBox::critical(
      this, tr("Error"),
      tr("A file or directory with the name '%1' already exists.").arg(fileName)
    );
    return;
  }

  // 空ファイルを作成
  QFile file(newFilePath);
  if (!file.open(QIODevice::WriteOnly)) {
    Logger::instance().error(QStringLiteral("Create file failed: %1").arg(newFilePath));
    QMessageBox::critical(
      this, tr("Error"),
      tr("Failed to create file: %1").arg(fileName)
    );
    return;
  }
  file.close();
  Logger::instance().info(QStringLiteral("Created file: %1").arg(newFilePath));

  // リフレッシュして新規ファイルにカーソルを移動
  srcPane->setPath(currentPath);
  FileListModel* model = srcPane->model();
  for (int i = 0; i < model->rowCount(); ++i) {
    const FileItem* item = model->itemAt(i);
    if (item && item->name() == fileName) {
      srcPane->view()->setCurrentIndex(model->index(i, 0));
      break;
    }
  }

  srcPane->view()->setFocus();
}

void FileManagerPanel::changeAttributes() {
  FileListPane* srcPane = activePane();
  FileListModel* model = srcPane->model();

  // 選択 or カレントのファイル/ディレクトリのパスを収集
  QStringList paths = model->selectedFilePaths();
  if (paths.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = model->itemAt(currentIndex);
      if (item && !item->isDotDot()) {
        paths.append(item->absolutePath());
      }
    }
  }
  if (paths.isEmpty()) return;

  AttributesDialog dlg(paths, this);
  if (dlg.exec() == QDialog::Accepted) {
    // カーソルが乗っていたファイル名を控えてからリフレッシュし、同名の行を再選択
    QString currentName;
    const QModelIndex currentIdx = srcPane->view()->currentIndex();
    if (currentIdx.isValid()) {
      const FileItem* item = model->itemAt(currentIdx);
      if (item) currentName = item->name();
    }
    model->refresh();
    if (!currentName.isEmpty()) {
      for (int i = 0; i < model->rowCount(); ++i) {
        const FileItem* item = model->itemAt(i);
        if (item && item->name() == currentName) {
          srcPane->view()->setCurrentIndex(model->index(i, 0));
          break;
        }
      }
    }
    srcPane->view()->setFocus();
  }
}

void FileManagerPanel::createArchive() {
  FileListPane* srcPane  = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;
  FileListModel* srcModel = srcPane->model();

  // 選択ファイル or カレント
  QStringList paths = srcModel->selectedFilePaths();
  if (paths.isEmpty()) {
    QModelIndex currentIndex = srcPane->view()->currentIndex();
    if (currentIndex.isValid()) {
      const FileItem* item = srcModel->itemAt(currentIndex);
      if (item && !item->isDotDot()) paths.append(item->absolutePath());
    }
  }
  if (paths.isEmpty()) return;

  // 既定の出力先は圧縮対象と同じディレクトリ（アクティブペインのカレント）
  CreateArchiveDialog dlg(paths, srcPane->currentPath(), this);
  if (dlg.exec() != QDialog::Accepted) return;

  QString     outputPath = dlg.outputPath();
  const auto  format     = dlg.format();

  if (outputPath.isEmpty()) return;

  // 既存ファイルがある場合はリネーム入力を求める。キャンセル（空 or reject）で中止。
  while (QFileInfo::exists(outputPath)) {
    const QString currentName = QFileInfo(outputPath).fileName();
    bool ok = false;
    const QString newName = inputText(
      this, tr("File Exists"),
      tr("'%1' already exists. Enter a different name:").arg(currentName),
      currentName, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    outputPath = QDir(QFileInfo(outputPath).absolutePath())
                   .absoluteFilePath(newName.trimmed());
  }

  ArchiveCreateWorker* worker = new ArchiveCreateWorker(
    outputPath, format, paths, this);
  ProgressDialog* dialog = new ProgressDialog(tr("Creating archive..."), this);
  dialog->setWorker(worker);

  connect(worker, &WorkerBase::finished, this,
    [this, dialog, srcPane, destPane, outputPath](bool ok) {
    dialog->accept();
    Logger::instance().log(ok ? Logger::Info : Logger::Error,
      QStringLiteral("Archive %1: %2")
        .arg(ok ? QStringLiteral("created") : QStringLiteral("create failed"))
        .arg(outputPath));
    // 出力先が src/dest どちらかのペインと一致していれば refresh してカーソル移動
    const QString outputDir = QFileInfo(outputPath).absolutePath();
    const QString fileName  = QFileInfo(outputPath).fileName();
    auto refreshIfMatches = [&](FileListPane* pane) {
      if (pane->currentPath() != outputDir) return;
      pane->setPath(outputDir);
      FileListModel* model = pane->model();
      for (int i = 0; i < model->rowCount(); ++i) {
        const FileItem* item = model->itemAt(i);
        if (item && item->name() == fileName) {
          pane->view()->setCurrentIndex(model->index(i, 0));
          break;
        }
      }
    };
    refreshIfMatches(srcPane);
    refreshIfMatches(destPane);
    activePane()->view()->setFocus();
  });
  worker->start();
  dialog->exec();
}

void FileManagerPanel::extractArchive() {
  FileListPane* srcPane  = activePane();
  FileListPane* destPane = (m_activePane == PaneType::Left) ? m_rightPane : m_leftPane;
  FileListModel* srcModel = srcPane->model();

  QModelIndex currentIndex = srcPane->view()->currentIndex();
  if (!currentIndex.isValid()) return;
  const FileItem* item = srcModel->itemAt(currentIndex);
  if (!item || item->isDir() || item->isDotDot()) {
    QMessageBox::information(
      this, tr("Extract Archive"),
      tr("Select an archive file to extract."));
    return;
  }

  const QString archivePath = item->absolutePath();
  // 展開先の既定はアーカイブと同じディレクトリ
  const QString defaultDir = QFileInfo(archivePath).absolutePath();

  ExtractArchiveDialog dlg(archivePath, defaultDir, this);
  if (dlg.exec() != QDialog::Accepted) return;

  const QString outputDir = dlg.outputDirectory();
  if (outputDir.isEmpty()) return;

  // アーカイブ名から拡張子を剥がしてベース名を作る
  QString baseName = QFileInfo(archivePath).fileName();
  static const QStringList kKnownExts = {
    QStringLiteral(".tar.gz"),  QStringLiteral(".tar.bz2"),
    QStringLiteral(".tar.xz"),  QStringLiteral(".tgz"),
    QStringLiteral(".tbz2"),    QStringLiteral(".txz"),
    QStringLiteral(".tar"),     QStringLiteral(".zip"),
  };
  for (const QString& e : kKnownExts) {
    if (baseName.endsWith(e, Qt::CaseInsensitive)) {
      baseName.chop(e.size());
      break;
    }
  }
  if (baseName.isEmpty()) baseName = QStringLiteral("extracted");

  // サブディレクトリを確定。既存ならリネーム入力、キャンセルで中止。
  QString targetDir = QDir(outputDir).absoluteFilePath(baseName);
  while (QFileInfo::exists(targetDir)) {
    bool ok = false;
    const QString newName = inputText(
      this, tr("Directory Exists"),
      tr("'%1' already exists. Enter a different name:").arg(baseName),
      baseName, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    baseName  = newName.trimmed();
    targetDir = QDir(outputDir).absoluteFilePath(baseName);
  }

  ArchiveExtractWorker* worker = new ArchiveExtractWorker(
    archivePath, targetDir, this);
  ProgressDialog* dialog = new ProgressDialog(tr("Extracting archive..."), this);
  dialog->setWorker(worker);

  connect(worker, &WorkerBase::finished, this,
    [this, dialog, srcPane, destPane, outputDir, baseName, archivePath](bool ok) {
    Logger::instance().log(ok ? Logger::Info : Logger::Error,
      QStringLiteral("Archive %1: %2")
        .arg(ok ? QStringLiteral("extracted") : QStringLiteral("extract failed"))
        .arg(archivePath));
    dialog->accept();
    // 展開したサブディレクトリを含む親ディレクトリと一致するペインを refresh し、
    // サブディレクトリ名にカーソルを合わせる
    auto refreshIfMatches = [&](FileListPane* pane) {
      if (pane->currentPath() != outputDir) return;
      pane->setPath(outputDir);
      FileListModel* model = pane->model();
      for (int i = 0; i < model->rowCount(); ++i) {
        const FileItem* item = model->itemAt(i);
        if (item && item->name() == baseName) {
          pane->view()->setCurrentIndex(model->index(i, 0));
          break;
        }
      }
    };
    refreshIfMatches(srcPane);
    refreshIfMatches(destPane);
    activePane()->view()->setFocus();
  });
  worker->start();
  dialog->exec();
}

void FileManagerPanel::openSortFilterDialog() {
  FileListPane* pane = activePane();
  const QString path = pane->currentPath();
  if (path.isEmpty()) {
    return;
  }

  auto& settings = Settings::instance();
  const bool alreadySaved = settings.hasPathOverride(path);

  // 初期値は override があればそれ、なければペインの既定
  PaneSettings initial = alreadySaved
                         ? settings.pathOverride(path)
                         : settings.paneSettings(m_activePane);

  SortFilterDialog dialog(path, initial, alreadySaved, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  PaneSettings result = dialog.result();

  // 保存 or 削除
  if (dialog.saveForDirectory()) {
    settings.setPathOverride(path, result);
    settings.save();
  } else if (alreadySaved) {
    settings.removePathOverride(path);
    settings.save();
  }

  // 現在のビューに即座に適用
  FileListModel* model = pane->model();
  model->setSortSettings(
    result.sortKey, result.sortOrder, result.sortKey2nd,
    result.sortDirsType, result.sortDotFirst, result.sortCS
  );
  model->setAttrFilter(result.attrFilter);
  model->setNameFilters(result.nameFilters);

  int section = -1;
  switch (result.sortKey) {
    case SortKey::Name:         section = FileListModel::Name;         break;
    case SortKey::Size:         section = FileListModel::Size;         break;
    case SortKey::Type:         section = FileListModel::Type;         break;
    case SortKey::LastModified: section = FileListModel::LastModified; break;
    case SortKey::None: break;
  }
  if (section >= 0) {
    pane->view()->horizontalHeader()->setSortIndicator(section, result.sortOrder);
  }

  pane->refreshSortFilterStatus();
  pane->view()->setFocus();
}

void FileManagerPanel::bulkRenameItems() {
  FileListPane* srcPane  = activePane();
  FileListModel* srcModel = srcPane->model();

  // 対象集め: 選択ファイルがあればそれら、無ければカーソル行 1 件
  QStringList names;
  for (int i = 0; i < srcModel->rowCount(); ++i) {
    const FileItem* it = srcModel->itemAt(i);
    if (!it || it->isDotDot()) continue;
    if (it->isSelected()) names.append(it->name());
  }
  if (names.isEmpty()) {
    QModelIndex cur = srcPane->view()->currentIndex();
    if (cur.isValid()) {
      const FileItem* it = srcModel->itemAt(cur);
      if (it && !it->isDotDot()) names.append(it->name());
    }
  }
  if (names.isEmpty()) return;

  const QString dirPath = srcPane->currentPath();

  BulkRenameDialog dialog(dirPath, names, this);
  if (dialog.exec() != QDialog::Accepted) return;

  const auto pairs = dialog.renames();
  int ok = 0;
  int ng = 0;
  for (const auto& p : pairs) {
    const QString oldPath = dirPath + QLatin1Char('/') + p.first;
    const QString newPath = dirPath + QLatin1Char('/') + p.second;
    QFileInfo fi(oldPath);
    bool success = false;
    if (fi.isDir()) {
      QDir d;
      success = d.rename(oldPath, newPath);
    } else {
      QFile f;
      success = f.rename(oldPath, newPath);
    }
    if (success) {
      Logger::instance().info(QStringLiteral("Bulk rename: %1 → %2")
                                .arg(p.first, p.second));
      ++ok;
    } else {
      Logger::instance().error(QStringLiteral("Bulk rename failed: %1 → %2")
                                 .arg(p.first, p.second));
      ++ng;
    }
  }
  if (ng > 0) {
    QMessageBox::warning(this, tr("Bulk Rename"),
      tr("%1 file(s) renamed, %2 failed.").arg(ok).arg(ng));
  }

  // ペインを再読み込みしてカーソルを保持
  const QString currentPath = srcPane->currentPath();
  srcPane->setPath(currentPath);
}

void FileManagerPanel::renameItem() {
  FileListPane* srcPane = activePane();
  FileListModel* srcModel = srcPane->model();

  QModelIndex currentIndex = srcPane->view()->currentIndex();
  if (!currentIndex.isValid()) {
    return;
  }

  const FileItem* item = srcModel->itemAt(currentIndex);
  if (!item || item->isDotDot()) {
    return;
  }

  QString oldName = item->name();
  QString oldPath = item->absolutePath();

  // Ask for new name
  bool ok = false;
  QString newName = inputText(
    this,
    tr("Rename"),
    tr("Enter new name:"),
    oldName,
    &ok
  );

  if (!ok || newName.isEmpty() || newName == oldName) {
    return;
  }

  // Check if new name already exists
  QString parentPath = QFileInfo(oldPath).path();
  QString newPath = parentPath + "/" + newName;

  if (QFileInfo::exists(newPath)) {
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("A file or directory with the name '%1' already exists.").arg(newName)
    );
    return;
  }

  // Rename
  bool success = false;
  if (item->isDir()) {
    QDir dir;
    success = dir.rename(oldPath, newPath);
  } else {
    QFile file;
    success = file.rename(oldPath, newPath);
  }

  if (!success) {
    Logger::instance().error(QStringLiteral("Rename failed: %1 → %2")
      .arg(oldPath, newPath));
    QMessageBox::critical(
      this,
      tr("Error"),
      tr("Failed to rename '%1' to '%2'.").arg(oldName, newName)
    );
    return;
  }
  Logger::instance().info(QStringLiteral("Rename: %1 → %2").arg(oldName, newName));

  // Refresh and move cursor to renamed item
  QString currentPath = srcPane->currentPath();
  srcPane->setPath(currentPath);

  // Find and select the renamed item
  for (int i = 0; i < srcModel->rowCount(); ++i) {
    const FileItem* renamedItem = srcModel->itemAt(i);
    if (renamedItem && renamedItem->name() == newName) {
      srcPane->view()->setCurrentIndex(srcModel->index(i, 0));
      break;
    }
  }

  srcPane->view()->setFocus();
}

} // namespace Farman

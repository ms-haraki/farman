#include "SearchDialog.h"
#include "core/workers/SearchWorker.h"
#include "settings/Settings.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QDateTime>
#include <QLocale>
#include <QStyle>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

namespace {

QString humanSize(qint64 bytes) {
  return QLocale::system().formattedDataSize(bytes);
}

} // anonymous namespace

SearchDialog::SearchDialog(const QString& initialPath, QWidget* parent)
  : QDialog(parent)
  , m_pathEdit(nullptr)
  , m_browseButton(nullptr)
  , m_patternEdit(nullptr)
  , m_excludeEdit(nullptr)
  , m_subdirsCheck(nullptr)
  , m_searchButton(nullptr)
  , m_resultsTable(nullptr)
  , m_statusLabel(nullptr)
  , m_closeButton(nullptr)
  , m_worker(nullptr)
  , m_searching(false) {
  setupUi(initialPath);
}

SearchDialog::~SearchDialog() {
  stopSearch();
}

void SearchDialog::setupUi(const QString& initialPath) {
  setWindowTitle(tr("Search Files"));
  setModal(true);
  resize(720, 520);

  const QString altP = QKeySequence(Qt::ALT | Qt::Key_P).toString(QKeySequence::NativeText);
  const QString altM = QKeySequence(Qt::ALT | Qt::Key_M).toString(QKeySequence::NativeText);
  const QString altS = QKeySequence(Qt::ALT | Qt::Key_S).toString(QKeySequence::NativeText);
  const QString altX = QKeySequence(Qt::ALT | Qt::Key_X).toString(QKeySequence::NativeText);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  QFormLayout* form = new QFormLayout();
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  // Start path + Browse
  QWidget* pathRow = new QWidget(this);
  QHBoxLayout* pathRowLayout = new QHBoxLayout(pathRow);
  pathRowLayout->setContentsMargins(0, 0, 0, 0);
  m_pathEdit = new QLineEdit(initialPath, this);
  m_pathEdit->setFocusPolicy(Qt::StrongFocus);
  m_browseButton = new QPushButton(this);
  m_browseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  m_browseButton->setToolTip(tr("Browse folder..."));
  m_browseButton->setFocusPolicy(Qt::StrongFocus);
  pathRowLayout->addWidget(m_pathEdit, 1);
  pathRowLayout->addWidget(m_browseButton);
  form->addRow(tr("Start path (%1):").arg(altP), pathRow);

  // Name pattern
  m_patternEdit = new QLineEdit(this);
  m_patternEdit->setPlaceholderText(tr("e.g. *.txt *.cpp (space-separated, empty for all)"));
  m_patternEdit->setFocusPolicy(Qt::StrongFocus);
  form->addRow(tr("Name pattern (%1):").arg(altM), m_patternEdit);

  // Exclude dirs
  m_excludeEdit = new QLineEdit(this);
  m_excludeEdit->setText(Settings::instance().searchExcludeDirs().join(QLatin1Char(' ')));
  m_excludeEdit->setPlaceholderText(tr("e.g. .* node_modules (space-separated)"));
  m_excludeEdit->setToolTip(
    tr("Directory names (glob) to skip when recursing. Default comes "
       "from Settings → Behavior. Changes here apply to this search only."));
  m_excludeEdit->setFocusPolicy(Qt::StrongFocus);
  form->addRow(tr("Exclude dirs (%1):").arg(altX), m_excludeEdit);

  // Include subdirectories
  m_subdirsCheck = new QCheckBox(tr("Include subdirectories (%1)").arg(altS), this);
  m_subdirsCheck->setChecked(true);
  m_subdirsCheck->setFocusPolicy(Qt::StrongFocus);
  form->addRow(QString(), m_subdirsCheck);

  mainLayout->addLayout(form);

  // Search / Stop button
  QHBoxLayout* searchRow = new QHBoxLayout();
  m_searchButton = new QPushButton(tr("Search"), this);
  applyAltShortcut(m_searchButton, Qt::Key_F);  // Find
  m_searchButton->setDefault(true);
  searchRow->addStretch(1);
  searchRow->addWidget(m_searchButton);
  mainLayout->addLayout(searchRow);

  // Results table
  m_resultsTable = new QTableWidget(this);
  m_resultsTable->setColumnCount(4);
  m_resultsTable->setHorizontalHeaderLabels(
    {tr("Name"), tr("Path"), tr("Size"), tr("Modified")});
  m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_resultsTable->verticalHeader()->setVisible(false);
  m_resultsTable->horizontalHeader()->setStretchLastSection(false);
  m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_resultsTable->setShowGrid(false);
  m_resultsTable->setTabKeyNavigation(false);  // Tab はボタンへ抜ける
  m_resultsTable->setFocusPolicy(Qt::StrongFocus);
  mainLayout->addWidget(m_resultsTable, 1);

  m_statusLabel = new QLabel(tr("Ready."), this);
  mainLayout->addWidget(m_statusLabel);

  // Bottom buttons
  QHBoxLayout* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  m_closeButton = new QPushButton(tr("Close"), this);
  applyAltShortcut(m_closeButton, Qt::Key_C);
  btnLayout->addWidget(m_closeButton);
  mainLayout->addLayout(btnLayout);

  // Signals
  connect(m_browseButton,  &QPushButton::clicked, this, &SearchDialog::onBrowse);
  connect(m_searchButton,  &QPushButton::clicked, this, &SearchDialog::onSearchOrStop);
  connect(m_closeButton,   &QPushButton::clicked, this, &QDialog::reject);
  // ダブルクリックで結果行のファイルへ移動
  connect(m_resultsTable,  &QTableWidget::itemDoubleClicked,
          this, [this](QTableWidgetItem*) { onGoTo(); });
  // Enter は QDialog の default button（Search）が先に拾ってしまうので、
  // 結果テーブル上のキー入力を eventFilter で先取りする。
  m_resultsTable->installEventFilter(this);

  // Tab 順: path → browse → pattern → exclude → subdirs → Search → table → Close
  setTabOrder(m_pathEdit,      m_browseButton);
  setTabOrder(m_browseButton,  m_patternEdit);
  setTabOrder(m_patternEdit,   m_excludeEdit);
  setTabOrder(m_excludeEdit,   m_subdirsCheck);
  setTabOrder(m_subdirsCheck,  m_searchButton);
  setTabOrder(m_searchButton,  m_resultsTable);
  setTabOrder(m_resultsTable,  m_closeButton);

  m_patternEdit->setFocus();
}

bool SearchDialog::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_resultsTable && event->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
      if (m_resultsTable->currentRow() >= 0) {
        onGoTo();
        return true;
      }
    }
  }
  return QDialog::eventFilter(obj, event);
}

void SearchDialog::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() & Qt::AltModifier) {
    switch (event->key()) {
      case Qt::Key_P:
        m_pathEdit->setFocus();
        m_pathEdit->selectAll();
        event->accept();
        return;
      case Qt::Key_M:
        m_patternEdit->setFocus();
        m_patternEdit->selectAll();
        event->accept();
        return;
      case Qt::Key_X:
        m_excludeEdit->setFocus();
        m_excludeEdit->selectAll();
        event->accept();
        return;
      case Qt::Key_S:
        m_subdirsCheck->setChecked(!m_subdirsCheck->isChecked());
        event->accept();
        return;
      default: break;
    }
  }
  QDialog::keyPressEvent(event);
}

void SearchDialog::onBrowse() {
  const QString start = m_pathEdit->text().trimmed().isEmpty()
                          ? QString()
                          : m_pathEdit->text().trimmed();
  const QString selected = QFileDialog::getExistingDirectory(
    this, tr("Select Start Directory"), start,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!selected.isEmpty()) {
    m_pathEdit->setText(selected);
  }
}

void SearchDialog::onSearchOrStop() {
  if (m_searching) {
    stopSearch();
  } else {
    startSearch();
  }
}

void SearchDialog::startSearch() {
  const QString rootPath = m_pathEdit->text().trimmed();
  if (rootPath.isEmpty()) return;

  // パターン: 空白で分割
  auto splitPatterns = [](const QString& text) {
    return text.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  };
  const QStringList patterns        = splitPatterns(m_patternEdit->text());
  const QStringList excludePatterns = splitPatterns(m_excludeEdit->text());
  const bool includeSubdirs = m_subdirsCheck->isChecked();

  // 結果テーブルをクリア
  m_resultsTable->setRowCount(0);
  m_statusLabel->setText(tr("Searching..."));

  m_worker = new SearchWorker(rootPath, patterns, excludePatterns,
                              includeSubdirs, this);
  connect(m_worker, &SearchWorker::resultFound, this, &SearchDialog::onResultFound);
  connect(m_worker, &WorkerBase::finished,      this, &SearchDialog::onFinished);
  m_searching = true;
  m_searchButton->setText(tr("Stop"));
  m_worker->start();
}

void SearchDialog::stopSearch() {
  if (m_worker) {
    m_worker->requestCancel();
    m_worker->wait();
    m_worker->deleteLater();
    m_worker = nullptr;
  }
  m_searching = false;
  if (m_searchButton) m_searchButton->setText(tr("Search"));
}

void SearchDialog::onResultFound(const QString& path) {
  appendResultRow(path);
  m_statusLabel->setText(tr("Searching... %1 found").arg(m_resultsTable->rowCount()));
}

void SearchDialog::onFinished(bool /*success*/) {
  m_searching = false;
  if (m_searchButton) m_searchButton->setText(tr("Search"));
  m_statusLabel->setText(tr("Done. %1 found.").arg(m_resultsTable->rowCount()));
  if (m_worker) {
    m_worker->deleteLater();
    m_worker = nullptr;
  }
}

void SearchDialog::appendResultRow(const QString& path) {
  const QFileInfo info(path);
  const int row = m_resultsTable->rowCount();
  m_resultsTable->insertRow(row);

  auto* nameItem = new QTableWidgetItem(info.fileName());
  auto* pathItem = new QTableWidgetItem(info.absolutePath());
  auto* sizeItem = new QTableWidgetItem(humanSize(info.size()));
  auto* timeItem = new QTableWidgetItem(
    info.lastModified().toString(QStringLiteral("yyyy/MM/dd HH:mm")));

  // パスを隠しデータとしてフルパスを保持
  nameItem->setData(Qt::UserRole, info.absoluteFilePath());

  m_resultsTable->setItem(row, 0, nameItem);
  m_resultsTable->setItem(row, 1, pathItem);
  m_resultsTable->setItem(row, 2, sizeItem);
  m_resultsTable->setItem(row, 3, timeItem);
  if (row == 0) {
    m_resultsTable->setCurrentCell(0, 0);
  }
}

void SearchDialog::onGoTo() {
  const int row = m_resultsTable->currentRow();
  if (row < 0) return;
  const QTableWidgetItem* item = m_resultsTable->item(row, 0);
  if (!item) return;
  m_selectedPath = item->data(Qt::UserRole).toString();
  accept();
}

} // namespace Farman

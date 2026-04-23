#include "BookmarkListDialog.h"
#include "core/BookmarkManager.h"
#include "settings/Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QAbstractItemView>

namespace Farman {

BookmarkListDialog::BookmarkListDialog(QWidget* parent)
  : QDialog(parent)
  , m_table(nullptr)
  , m_goButton(nullptr)
  , m_renameButton(nullptr)
  , m_deleteButton(nullptr)
  , m_upButton(nullptr)
  , m_downButton(nullptr) {

  setWindowTitle(tr("Bookmarks"));
  resize(560, 380);
  setupUi();
  refreshList();

  connect(&BookmarkManager::instance(), &BookmarkManager::bookmarksChanged,
          this, &BookmarkListDialog::refreshList);
}

void BookmarkListDialog::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  m_table = new QTableWidget(this);
  m_table->setColumnCount(2);
  m_table->setHorizontalHeaderLabels({tr("Name"), tr("Path")});
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setAlternatingRowColors(true);
  m_table->verticalHeader()->setVisible(false);
  m_table->horizontalHeader()->setStretchLastSection(true);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_table->setShowGrid(false);

  connect(m_table, &QTableWidget::itemDoubleClicked,
          this, [this](QTableWidgetItem*) { onItemDoubleClicked(); });
  connect(m_table, &QTableWidget::currentCellChanged,
          this, [this](int, int, int, int) { onSelectionChanged(); });
  mainLayout->addWidget(m_table, 1);

  QHBoxLayout* btnLayout = new QHBoxLayout();

  m_goButton     = new QPushButton(tr("Go"),     this);
  m_renameButton = new QPushButton(tr("Rename"), this);
  m_deleteButton = new QPushButton(tr("Delete"), this);
  m_upButton     = new QPushButton(tr("↑"),      this);
  m_downButton   = new QPushButton(tr("↓"),      this);
  QPushButton* closeButton = new QPushButton(tr("Close"), this);

  m_goButton->setDefault(true);
  m_upButton->setToolTip(tr("Move selected bookmark up"));
  m_downButton->setToolTip(tr("Move selected bookmark down"));

  connect(m_goButton,     &QPushButton::clicked, this, &BookmarkListDialog::onGo);
  connect(m_renameButton, &QPushButton::clicked, this, &BookmarkListDialog::onRename);
  connect(m_deleteButton, &QPushButton::clicked, this, &BookmarkListDialog::onDelete);
  connect(m_upButton,     &QPushButton::clicked, this, &BookmarkListDialog::onMoveUp);
  connect(m_downButton,   &QPushButton::clicked, this, &BookmarkListDialog::onMoveDown);
  connect(closeButton,    &QPushButton::clicked, this, &QDialog::reject);

  btnLayout->addWidget(m_goButton);
  btnLayout->addWidget(m_renameButton);
  btnLayout->addWidget(m_deleteButton);
  btnLayout->addWidget(m_upButton);
  btnLayout->addWidget(m_downButton);
  btnLayout->addStretch(1);
  btnLayout->addWidget(closeButton);

  mainLayout->addLayout(btnLayout);

  onSelectionChanged();
}

int BookmarkListDialog::currentRow() const {
  return m_table ? m_table->currentRow() : -1;
}

void BookmarkListDialog::refreshList() {
  if (!m_table) return;
  const int prev = m_table->currentRow();

  const QList<Bookmark> list = BookmarkManager::instance().bookmarks();
  m_table->setRowCount(list.size());
  for (int i = 0; i < list.size(); ++i) {
    const Bookmark& b = list[i];
    auto* nameItem = new QTableWidgetItem(b.name);
    auto* pathItem = new QTableWidgetItem(b.path);
    // 行全体が選択されるので Flag はデフォルトで OK（編集は setEditTriggers で抑止済み）
    m_table->setItem(i, 0, nameItem);
    m_table->setItem(i, 1, pathItem);
  }

  // Name 列は初回のみ自動フィット（以後はユーザー操作に任せる）
  static bool resized = false;
  if (!resized) {
    m_table->resizeColumnToContents(0);
    resized = true;
  }

  if (m_table->rowCount() > 0) {
    int row = qBound(0, prev, m_table->rowCount() - 1);
    if (prev < 0) row = 0;
    m_table->setCurrentCell(row, 0);
  }
  onSelectionChanged();
}

void BookmarkListDialog::onSelectionChanged() {
  const int row = currentRow();
  const bool hasRow = row >= 0;
  const QList<Bookmark> list = BookmarkManager::instance().bookmarks();
  const bool isDefault = hasRow && row < list.size() && list[row].isDefault;

  if (m_goButton)     m_goButton->setEnabled(hasRow);
  if (m_renameButton) m_renameButton->setEnabled(hasRow);
  // デフォルトブックマークは削除禁止
  if (m_deleteButton) m_deleteButton->setEnabled(hasRow && !isDefault);
  if (m_upButton)     m_upButton->setEnabled(hasRow && row > 0);
  if (m_downButton)   m_downButton->setEnabled(
    hasRow && m_table && row < m_table->rowCount() - 1);
}

void BookmarkListDialog::onGo() {
  const int row = currentRow();
  if (row < 0) return;
  const QList<Bookmark> list = BookmarkManager::instance().bookmarks();
  if (row >= list.size()) return;
  m_selectedPath = list[row].path;
  accept();
}

void BookmarkListDialog::onItemDoubleClicked() {
  onGo();
}

void BookmarkListDialog::onRename() {
  const int row = currentRow();
  if (row < 0) return;
  const QList<Bookmark> list = BookmarkManager::instance().bookmarks();
  if (row >= list.size()) return;

  bool ok = false;
  const QString newName = QInputDialog::getText(
    this, tr("Rename Bookmark"),
    tr("Name:"), QLineEdit::Normal,
    list[row].name, &ok);
  if (!ok) return;
  BookmarkManager::instance().rename(row, newName);
}

void BookmarkListDialog::onDelete() {
  const int row = currentRow();
  if (row < 0) return;
  const QList<Bookmark> list = BookmarkManager::instance().bookmarks();
  if (row >= list.size()) return;

  const QString label = list[row].name.isEmpty() ? list[row].path : list[row].name;
  const auto reply = QMessageBox::question(
    this, tr("Delete Bookmark"),
    tr("Delete bookmark '%1'?").arg(label),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (reply != QMessageBox::Yes) return;

  BookmarkManager::instance().removeAt(row);
}

void BookmarkListDialog::onMoveUp() {
  const int row = currentRow();
  if (row <= 0) return;
  BookmarkManager::instance().move(row, row - 1);
  if (m_table) m_table->setCurrentCell(row - 1, 0);
}

void BookmarkListDialog::onMoveDown() {
  const int row = currentRow();
  if (row < 0 || !m_table) return;
  if (row >= m_table->rowCount() - 1) return;
  BookmarkManager::instance().move(row, row + 1);
  m_table->setCurrentCell(row + 1, 0);
}

} // namespace Farman

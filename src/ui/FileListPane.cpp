#include "FileListPane.h"
#include "FileListDelegate.h"
#include "model/FileListModel.h"
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
  m_view->verticalHeader()->setVisible(false);

  m_model = new FileListModel(this);
  m_view->setModel(m_model);

  m_delegate = new FileListDelegate(this);
  m_view->setItemDelegate(m_delegate);

  connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &FileListPane::onCurrentChanged);

  m_view->setColumnWidth(FileListModel::Name, 250);
  m_view->setColumnWidth(FileListModel::Size, 100);
  m_view->setColumnWidth(FileListModel::Type, 80);
  m_view->setColumnWidth(FileListModel::LastModified, 150);

  mainLayout->addWidget(m_view);
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
  }
  return result;
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

} // namespace Farman

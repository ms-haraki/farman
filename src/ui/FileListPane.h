#pragma once

#include <QWidget>

class QLabel;
class QToolButton;
class QTableView;

namespace Farman {

class FileListModel;
class FileListDelegate;

class FileListPane : public QWidget {
  Q_OBJECT

public:
  explicit FileListPane(QWidget* parent = nullptr);
  ~FileListPane() override;

  // アクセサ
  QTableView* view() const { return m_view; }
  FileListModel* model() const { return m_model; }
  FileListDelegate* delegate() const { return m_delegate; }

  // パス操作
  QString currentPath() const;
  bool setPath(const QString& path);

  // アクティブ状態
  void setActive(bool active);

signals:
  void folderButtonClicked();
  void currentChanged(const QModelIndex& current, const QModelIndex& previous);

private slots:
  void onFolderButtonClicked();
  void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

private:
  void setupUi();

  QLabel* m_pathLabel;
  QToolButton* m_folderButton;
  QTableView* m_view;
  FileListModel* m_model;
  FileListDelegate* m_delegate;
};

} // namespace Farman

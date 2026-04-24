#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QTableWidget;
class QLabel;

namespace Farman {

class SearchWorker;

// ファイル検索ダイアログ。
// - Start path / Name pattern (glob) / Include subdirectories
// - Search/Stop で別スレッド検索、結果を逐次テーブルに追加
// - ダブルクリック or Go to で accept し、呼び出し側は selectedPath() を取る
// - selectedPath() はファイルの絶対パス。呼び出し側でそのディレクトリへ
//   ペインを移動し、該当ファイルにカーソルを合わせる。
class SearchDialog : public QDialog {
  Q_OBJECT

public:
  SearchDialog(const QString& initialPath, QWidget* parent = nullptr);
  ~SearchDialog() override;

  QString selectedPath() const { return m_selectedPath; }

protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onBrowse();
  void onSearchOrStop();
  void onGoTo();
  void onResultFound(const QString& path);
  void onFinished(bool success);

private:
  void setupUi(const QString& initialPath);
  void startSearch();
  void stopSearch();
  void appendResultRow(const QString& path);

  QLineEdit*    m_pathEdit;
  QPushButton*  m_browseButton;
  QLineEdit*    m_patternEdit;
  QLineEdit*    m_excludeEdit;
  QCheckBox*    m_subdirsCheck;
  QPushButton*  m_searchButton;
  QTableWidget* m_resultsTable;
  QLabel*       m_statusLabel;
  QPushButton*  m_closeButton;

  SearchWorker* m_worker;
  QString       m_selectedPath;
  bool          m_searching;
};

} // namespace Farman

#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QTableWidget;
class QPushButton;

namespace Farman {

// ディレクトリ履歴を一覧表示して選択するダイアログ。
// アクティブペインの履歴（最大10件、最新順）を表示し、Go / ダブルクリック
// で選択パスを確定して閉じる。Close で破棄。
class HistoryDialog : public QDialog {
  Q_OBJECT

public:
  HistoryDialog(const QStringList& entries, QWidget* parent = nullptr);
  ~HistoryDialog() override = default;

  QString selectedPath() const { return m_selectedPath; }

private slots:
  void onGo();
  void onSelectionChanged();

private:
  void setupUi(const QStringList& entries);

  QTableWidget* m_table;
  QPushButton*  m_goButton;
  QString       m_selectedPath;
};

} // namespace Farman

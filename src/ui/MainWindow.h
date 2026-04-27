#pragma once

#include <QMainWindow>
#include "types.h"
#include "../settings/Settings.h"
#include "../keybinding/CommandRegistry.h"
#include "../keybinding/KeyBindingManager.h"

class QStackedWidget;
class QLabel;

namespace Farman {

class FileManagerPanel;
class ViewerPanel;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onFileActivated(const QString& filePath);
  void onPathChanged(const QString& leftPath, const QString& rightPath);
  void onSettingsChanged();

private:
  void setupUi();
  void showFileManager();
  void showViewer(const QString& filePath);
  void showSettingsDialog();
  void registerCommands();
  void createMenus();
  void showAboutDialog();

  // どちらのパネルがアクティブかでステータスバーの表示元を切り替えるため、
  // それぞれの最新ステータスをキャッシュしておく
  void updateStatusBar();
  QString m_fmStatusPath;
  QString m_fmStatusSummary;
  QString m_viewerStatusPath;
  QString m_viewerStatusSummary;

  QStackedWidget* m_stack;
  FileManagerPanel* m_fileManagerPanel;
  ViewerPanel* m_viewerPanel;
  // ステータスバー (左: フォーカス中ファイルの絶対パス / 右: 件数・選択要約)
  QLabel*      m_statusPathLabel    = nullptr;
  QLabel*      m_statusSummaryLabel = nullptr;
};

} // namespace Farman

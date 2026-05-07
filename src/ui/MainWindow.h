#pragma once

#include <QMainWindow>
#include "types.h"
#include "../settings/Settings.h"
#include "../keybinding/CommandRegistry.h"
#include "../keybinding/KeyBindingManager.h"
#include "ViewerPanel.h"

class QAction;
class QStackedWidget;
class QLabel;

namespace Farman {

class FileManagerPanel;
class ShortcutListDialog;

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
  void onSettingsChanged();

private:
  void setupUi();
  void showFileManager();
  void showViewer(const QString& filePath);
  // ビュアーを呼び出し側で固定して開く（任意ビュアー機能から使う）
  void showViewerWith(const QString& filePath, ViewerPanel::ViewerKind kind);
  void showSettingsDialog();
  void registerCommands();
  void createMenus();
  void showAboutDialog();
  // ショートカット一覧ウィンドウのトグル表示。
  void toggleShortcutList();

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
  // ステータスバー (左: フォーカス中ファイルの絶対パス / 中央: Sync Browse 状態 /
  // 右: 件数・選択要約)
  QLabel*      m_statusPathLabel        = nullptr;
  QLabel*      m_statusSummaryLabel     = nullptr;
  QLabel*      m_statusSyncBrowseLabel  = nullptr;

  // View メニューの Sync Browse トグル項目 (FileManagerPanel 状態と同期)。
  QAction*     m_syncBrowseAction = nullptr;

  // ウィンドウタイトルのベース部分 (例: "farman 0.9.0")。Sync Browse が ON
  // のときにサフィックス "[Sync]" を追加するため、再構築用に保持しておく。
  QString      m_windowTitleBase;
  void         updateWindowTitle();

  // ショートカット一覧ウィンドウ (遅延生成)。
  ShortcutListDialog* m_shortcutListDialog = nullptr;

  // Tools メニュー (外部アプリ連携)。UserCommandManager の userCommandsChanged で
  // rebuildToolsMenu() が呼ばれ、addCmd() ベースで再構築する。
  QMenu* m_toolsMenu = nullptr;
  void   rebuildToolsMenu();
  // rebuildToolsMenu() で生成した QAction を保持する。再構築時は古いものを
  // FileManagerPanel から removeAction してから deleteLater する。これをやらないと
  // 同一ショートカット (例: T) が複数の QAction に紐付いて
  // "Ambiguous shortcut overload" エラーで発火しなくなる。
  QList<QAction*> m_userCmdActions;
};

} // namespace Farman

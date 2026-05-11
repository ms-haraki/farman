#pragma once

#include <QWidget>
#include "types.h"
#include "core/DirectoryHistory.h"

class QSplitter;
class QKeyEvent;

namespace Farman {

class FileListPane;
class FileListModel;
class LogPane;

class FileManagerPanel : public QWidget {
  Q_OBJECT

public:
  explicit FileManagerPanel(QWidget* parent = nullptr);
  ~FileManagerPanel() override;

  // 初期化
  void loadInitialPath();
  void applySettings();  // 設定を両ペインに適用

  // アクセサ
  FileListPane* activePane() const;
  FileListPane* leftPane() const { return m_leftPane; }
  FileListPane* rightPane() const { return m_rightPane; }
  QString currentPath() const;
  QString leftPath() const;
  QString rightPath() const;

  // ペイン操作
  void setActivePane(PaneType pane);
  void setSinglePaneMode(bool single);
  bool isSinglePaneMode() const { return m_singlePaneMode; }
  void togglePaneMode();

  // 同期ブラウズ (Sync Browse): 片方のペインで cd すると、もう一方も
  // 起点からの相対パスで追従する。シングルペイン時は自動的に OFF にし、
  // トグル操作も無効になる。永続化はしない (起動時は常に OFF)。
  bool isSyncBrowseEnabled() const { return m_syncBrowseEnabled; }
  void setSyncBrowseEnabled(bool enabled);
  void toggleSyncBrowse();

  // キーイベント処理
  bool handleKeyEvent(QKeyEvent* event);

protected:
  // 非アクティブペイン側のビューをマウスクリックされたら、そのペインを
  // アクティブにする。
  bool eventFilter(QObject* watched, QEvent* event) override;

public:

  // ファイル操作
  void copySelectedFiles();
  void moveSelectedFiles();
  void deleteSelectedFiles();
  void createDirectory();
  void createFile();
  void renameItem();
  // 選択した複数 (or カーソル行) を BulkRenameDialog で一括リネーム
  void bulkRenameItems();
  void changeAttributes();
  void createArchive();
  void extractArchive();

  // アクティブペインのソート・フィルタ編集ダイアログを開く
  void openSortFilterDialog();

  // 任意のパスへアクティブペインを遷移させる（失敗時は false）
  bool navigateActivePaneTo(const QString& path);

  // 反対側のペインをアクティブと同じディレクトリへ移動
  void syncOtherToActive();
  // 反対側のペインのディレクトリをアクティブへ取り込む（アクティブを反対側と同じに）
  void syncActiveToOther();

  // 外部 / 反対側ペインからのドロップを受け、Copy / Move / Cancel を尋ねて
  // 該当 worker を起動する。destPane が drop を受け取ったペイン。
  void handleExternalDrop(FileListPane* destPane, const QList<QUrl>& urls);

  // ── ディレクトリ履歴 ─────────────────────
  DirectoryHistory&       history(PaneType pane);
  const DirectoryHistory& history(PaneType pane) const;

  // ── 選択操作（コマンドから直接呼ばれる） ─────────
  // カーソル行を選択トグル（カーソル据え置き）
  void toggleSelection();

  // ── ログペイン ──────────────────────────
  void setLogPaneVisible(bool visible);
  bool isLogPaneVisible() const;
  void setLogPaneHeight(int px);

signals:
  void pathChanged(const QString& leftPath, const QString& rightPath);
  // ファイルを開く要求。
  //   filePath:    実際にディスクから読むパス
  //   displayPath: ステータス・タイトルに出す表示用パス。空のときは filePath
  //                をそのまま使う。アーカイブ内エントリの一時展開ケースで
  //                "/path/x.zip!/inner" を見せたい場合のみ別物になる。
  void fileActivated(const QString& filePath,
                     const QString& displayPath = QString());
  // ステータスバー連携用: アクティブペインのフォーカス中ファイルの絶対パス。
  // 何も選択されていない場合は空文字列。
  void activeFocusedPathChanged(const QString& path);
  // ステータスバー連携用: アクティブペインの要約 (件数 / 選択数 / サイズ)。
  void activeSummaryChanged(const QString& summary);
  // 同期ブラウズの ON/OFF が切り替わったとき (UI 更新トリガ)。
  void syncBrowseChanged(bool enabled);
  // 1 / 2 ペインモードが切り替わったとき (ツールバーのトグル状態同期に使う)。
  void singlePaneModeChanged(bool single);
  // ログペインの表示が切り替わったとき (同上)。
  void logPaneVisibleChanged(bool visible);

private slots:
  void onLeftPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onRightPaneCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void onLeftFolderButtonClicked();
  void onRightFolderButtonClicked();

private:
  void setupUi();
  void handleEnterKey();
  // アクティブペインの状態をステータスバー向けに再計算してシグナル送出する
  void emitActivePaneStatus();
  void handleBackspaceKey();
  void handleSpaceKey();
  void handleAsteriskKey();
  void handleSelectAllKey();
  void handleTabKey();
  void updatePathSignal();

  // ペインのパスに対応する保存済み override があればそれを、なければ既定を適用する
  void applyPathSortFilter(PaneType paneType);
  // ペインのパス遷移を一元化するヘルパ（applyPathSortFilter 適用まで面倒を見る）
  bool navigatePane(PaneType paneType, const QString& path);

  // Sync Browse: navigatePane 成功時に呼ばれ、もう一方のペインに同じ
  // 「相対変化」を適用する。oldPath → newPath の差分を取り、もう一方の
  // 現在地から同じ動きをさせる。ターゲットが存在しなければ何もしない。
  // 再帰呼び出しを防ぐため m_syncBrowseInProgress フラグでガード。
  void maybeSyncFollow(PaneType navigatedPane,
                       const QString& oldPath,
                       const QString& newPath);

  QSplitter* m_splitter;
  FileListPane* m_leftPane;
  FileListPane* m_rightPane;
  LogPane*      m_logPane = nullptr;

  PaneType m_activePane;
  bool m_singlePaneMode;

  // ── Sync Browse ─────────────────────
  // ON/OFF はメニュー (View → Sync Browse) または `y` キーで切替。
  // アンカー (起点) は持たず、片方のペインがディレクトリを移動した瞬間に
  // 「同じ相対変化」をもう一方のペインの現在地から再生する方式。
  // 浮遊トグルボタンは将来ツールバー実装のタイミングで再検討する。
  bool    m_syncBrowseEnabled    = false;
  bool    m_syncBrowseInProgress = false;

  DirectoryHistory m_leftHistory;
  DirectoryHistory m_rightHistory;
};

} // namespace Farman

#pragma once

#include <QDialog>
#include <QString>

class QFormLayout;
class QLabel;
class QPushButton;

namespace Farman {

class PropertiesWorker;

// カーソル位置のファイル / ディレクトリの詳細を表示する読み取り専用ダイアログ。
// `Alt+Enter` (file.properties) で開かれる。
//
// ディレクトリの場合は別スレッド (PropertiesWorker) で再帰的に
// サイズ計算しつつ、途中経過を表示する。閉じる前にキャンセルすると
// 計算は中断される。
class PropertiesDialog : public QDialog {
  Q_OBJECT

public:
  PropertiesDialog(const QString& path, QWidget* parent = nullptr);
  ~PropertiesDialog() override;

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onStatsUpdated(qint64 totalBytes, int fileCount, int dirCount);
  void onWorkerFinished(bool success);

private:
  void setupUi();
  void populateStaticInfo();

  QString          m_path;
  bool             m_isDir = false;

  // 行ごとの表示/非表示を切り替えるために form layout を保持する
  // (Windows では POSIX 系項目を行ごと隠すため)。
  QFormLayout*     m_form = nullptr;

  // 一覧表示用のラベル群
  QLabel*          m_nameLabel        = nullptr;
  QLabel*          m_pathLabel        = nullptr;
  QLabel*          m_typeLabel        = nullptr;
  QLabel*          m_sizeLabel        = nullptr;     // 動的更新
  QLabel*          m_modifiedLabel    = nullptr;
  QLabel*          m_createdLabel     = nullptr;
  QLabel*          m_accessedLabel    = nullptr;
  QLabel*          m_permissionsLabel = nullptr;
  QLabel*          m_ownerLabel       = nullptr;
  QLabel*          m_groupLabel       = nullptr;
  QLabel*          m_linkTargetLabel  = nullptr;

  // ディレクトリ集計用ワーカー (ファイルのときは nullptr)
  PropertiesWorker* m_worker = nullptr;

  QPushButton*     m_closeButton = nullptr;
};

} // namespace Farman

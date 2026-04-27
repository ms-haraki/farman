#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QToolButton;

namespace Farman {

class BehaviorTab : public QWidget {
  Q_OBJECT

public:
  BehaviorTab(const QString& leftCurrentPath,
              const QString& rightCurrentPath,
              QWidget* parent = nullptr);
  ~BehaviorTab() override = default;

  void save();

private slots:
  void onWindowSizeModeChanged(int index);
  void onWindowPositionModeChanged(int index);
  void onLeftInitialPathModeChanged(int index);
  void onRightInitialPathModeChanged(int index);
  void onLeftBrowseInitialPath();
  void onRightBrowseInitialPath();

private:
  void setupUi();
  void loadSettings();

  // ダイアログ起動時の各ペインのカレントディレクトリ。
  // Custom Path 未設定時のデフォルトや、Default/LastSession 選択時の表示値として使う。
  QString m_leftCurrentPath;
  QString m_rightCurrentPath;

  // Sort settings
  QComboBox*  m_sortKeyCombo;
  QComboBox*  m_sortOrderCombo;
  QComboBox*  m_sortKey2ndCombo;
  QComboBox*  m_sortDirsTypeCombo;
  QCheckBox*  m_sortDotFirstCheck;
  QCheckBox*  m_sortCaseSensitiveCheck;

  // Log settings
  QCheckBox*  m_logVisibleCheck       = nullptr;
  QSpinBox*   m_logPaneHeightSpin     = nullptr;
  QCheckBox*  m_logToFileCheck        = nullptr;
  QLineEdit*  m_logDirectoryEdit      = nullptr;
  class QToolButton* m_logDirectoryBrowse = nullptr;
  // 保持日数。0 を「永久」として扱うため、専用チェックボックス + SpinBox の組み合わせ
  QCheckBox*  m_logRetentionForeverCheck = nullptr;
  QSpinBox*   m_logRetentionDaysSpin     = nullptr;

  // Display settings
  QCheckBox*  m_showHiddenCheck;
  QCheckBox*  m_cursorLoopCheck;
  QCheckBox*  m_persistHistoryCheck;
  QCheckBox*  m_typeAheadDotfilesCheck;

  // File operation settings
  QLineEdit*  m_autoRenameTemplateEdit;
  QCheckBox*  m_defaultDeleteToTrashCheck;
  QLineEdit*  m_searchExcludeDirsEdit;

  // Startup / Behavior settings
  QComboBox*   m_leftInitialPathModeCombo;
  QLineEdit*   m_leftCustomPathEdit;
  QToolButton* m_leftBrowseButton;
  QComboBox*   m_rightInitialPathModeCombo;
  QLineEdit*   m_rightCustomPathEdit;
  QToolButton* m_rightBrowseButton;
  QCheckBox*   m_confirmOnExitCheck;

  // Application language
  QComboBox*   m_languageCombo = nullptr;

  // Window settings
  QComboBox*  m_windowSizeModeCombo;
  QSpinBox*   m_windowWidthSpin;
  QSpinBox*   m_windowHeightSpin;
  QComboBox*  m_windowPositionModeCombo;
  QSpinBox*   m_windowXSpin;
  QSpinBox*   m_windowYSpin;
};

} // namespace Farman

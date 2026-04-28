#pragma once

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;
class QShortcut;

namespace Farman {

class KeybindingTab;
class AppearanceTab;
class BehaviorTab;
class GeneralTab;
class ViewersTab;

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  SettingsDialog(const QString& leftCurrentPath,
                 const QString& rightCurrentPath,
                 QWidget* parent = nullptr);
  ~SettingsDialog() override = default;

protected:
  void keyPressEvent(QKeyEvent* event) override;

signals:
  void settingsChanged();

private slots:
  void onOk();
  void onApply();
  void onClearBinding();
  void onResetToDefaults();
  // 「Reset All Settings」ボタン: キーバインドを除く全 Settings を
  // デフォルトに戻す (確認ダイアログ付き)。
  void onResetAllSettings();

private:
  void setupUi();

  QString m_leftCurrentPath;
  QString m_rightCurrentPath;

  QTabWidget*     m_tabWidget;
  KeybindingTab*  m_keybindingTab;
  AppearanceTab*  m_appearanceTab;
  BehaviorTab*    m_behaviorTab;
  GeneralTab*     m_generalTab;
  ViewersTab*     m_viewersTab;
  QDialogButtonBox* m_buttonBox;
  QShortcut*      m_clearShortcut;
  QShortcut*      m_resetShortcut;
};

} // namespace Farman

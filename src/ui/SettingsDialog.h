#pragma once

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;
class QShortcut;

namespace Farman {

class KeybindingTab;
class AppearanceTab;
class BehaviorTab;

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SettingsDialog(QWidget* parent = nullptr);
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

private:
  void setupUi();

  QTabWidget*     m_tabWidget;
  KeybindingTab*  m_keybindingTab;
  AppearanceTab*  m_appearanceTab;
  BehaviorTab*    m_behaviorTab;
  QDialogButtonBox* m_buttonBox;
  QShortcut*      m_clearShortcut;
  QShortcut*      m_resetShortcut;
};

} // namespace Farman

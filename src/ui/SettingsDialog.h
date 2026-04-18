#pragma once

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;

namespace Farman {

class KeybindingTab;
class AppearanceTab;
class BehaviorTab;

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SettingsDialog(QWidget* parent = nullptr);
  ~SettingsDialog() override = default;

signals:
  void settingsChanged();

private slots:
  void onOk();
  void onApply();

private:
  void setupUi();

  QTabWidget*     m_tabWidget;
  KeybindingTab*  m_keybindingTab;
  AppearanceTab*  m_appearanceTab;
  BehaviorTab*    m_behaviorTab;
  QDialogButtonBox* m_buttonBox;
};

} // namespace Farman

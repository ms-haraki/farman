#include "SettingsDialog.h"
#include "KeybindingTab.h"
#include "AppearanceTab.h"
#include "BehaviorTab.h"
#include "settings/Settings.h"
#include "keybinding/KeyBindingManager.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>

namespace Farman {

SettingsDialog::SettingsDialog(QWidget* parent)
  : QDialog(parent)
  , m_tabWidget(nullptr)
  , m_keybindingTab(nullptr)
  , m_appearanceTab(nullptr)
  , m_behaviorTab(nullptr)
  , m_buttonBox(nullptr) {
  setupUi();
}

void SettingsDialog::setupUi() {
  setWindowTitle(tr("Settings"));
  resize(800, 600);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Tab widget
  m_tabWidget = new QTabWidget(this);

  m_keybindingTab = new KeybindingTab(this);
  m_appearanceTab = new AppearanceTab(this);
  m_behaviorTab = new BehaviorTab(this);

  m_tabWidget->addTab(m_keybindingTab, tr("Keybindings"));
  m_tabWidget->addTab(m_appearanceTab, tr("Appearance"));
  m_tabWidget->addTab(m_behaviorTab, tr("Behavior"));

  mainLayout->addWidget(m_tabWidget);

  // OK/Cancel/Apply buttons
  m_buttonBox = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
    this
  );

  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onOk);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
          this, &SettingsDialog::onApply);

  mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::onOk() {
  onApply();
  accept();
}

void SettingsDialog::onApply() {
  // Save all tabs
  m_keybindingTab->save();
  m_appearanceTab->save();
  m_behaviorTab->save();

  // Save settings to file
  Settings::instance().save();

  // Save keybindings to settings
  KeyBindingManager::instance().saveToSettings();

  // Notify that settings have changed
  emit settingsChanged();
}

} // namespace Farman

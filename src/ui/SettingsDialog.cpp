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
#include <QKeyEvent>
#include <QShortcut>
#include <QLabel>
#include <QDebug>

namespace Farman {

SettingsDialog::SettingsDialog(const QString& leftCurrentPath,
                               const QString& rightCurrentPath,
                               QWidget* parent)
  : QDialog(parent)
  , m_tabWidget(nullptr)
  , m_keybindingTab(nullptr)
  , m_appearanceTab(nullptr)
  , m_behaviorTab(nullptr)
  , m_buttonBox(nullptr)
  , m_leftCurrentPath(leftCurrentPath)
  , m_rightCurrentPath(rightCurrentPath) {
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
  m_behaviorTab = new BehaviorTab(m_leftCurrentPath, m_rightCurrentPath, this);

  m_tabWidget->addTab(m_behaviorTab, tr("1. Behavior"));
  m_tabWidget->addTab(m_appearanceTab, tr("2. Appearance"));
  m_tabWidget->addTab(m_keybindingTab, tr("3. Keybindings"));

  // Info label for keyboard shortcuts
  int tabCount = m_tabWidget->count();
  QLabel* infoLabel = new QLabel(tr("Use Alt+number to switch tabs, or Ctrl+Tab/Ctrl+Shift+Tab"), this);
  QFont infoFont = infoLabel->font();
  infoFont.setPointSize(infoFont.pointSize() - 1);
  infoLabel->setFont(infoFont);
  infoLabel->setStyleSheet("color: palette(mid); padding: 4px;");
  mainLayout->addWidget(infoLabel);

  mainLayout->addWidget(m_tabWidget);

  // Setup keyboard shortcuts for tab switching
  // Alt+1 through Alt+9 for direct tab access (up to 9 tabs)
  const QList<Qt::Key> numberKeys = {
    Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5,
    Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9
  };

  for (int i = 0; i < qMin(tabCount, numberKeys.size()); ++i) {
    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::ALT | numberKeys[i]), this);
    // Capture i by value to avoid lambda capture issues
    connect(shortcut, &QShortcut::activated, [this, i]() {
      m_tabWidget->setCurrentIndex(i);
    });
  }

  // Ctrl+Tab for next tab
  QShortcut* shortcutNext = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
  connect(shortcutNext, &QShortcut::activated, [this]() {
    int currentIndex = m_tabWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % m_tabWidget->count();
    m_tabWidget->setCurrentIndex(nextIndex);
  });

  // Ctrl+Shift+Tab for previous tab
  QShortcut* shortcutPrev = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
  connect(shortcutPrev, &QShortcut::activated, [this]() {
    int currentIndex = m_tabWidget->currentIndex();
    int prevIndex = (currentIndex - 1 + m_tabWidget->count()) % m_tabWidget->count();
    m_tabWidget->setCurrentIndex(prevIndex);
  });

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

  // Keyboard shortcuts for Keybindings tab
  m_clearShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
  connect(m_clearShortcut, &QShortcut::activated, this, &SettingsDialog::onClearBinding);

  m_resetShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
  connect(m_resetShortcut, &QShortcut::activated, this, &SettingsDialog::onResetToDefaults);
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

void SettingsDialog::onClearBinding() {
  qDebug() << "SettingsDialog::onClearBinding()";
  // Only work if Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    m_keybindingTab->clearCurrentBinding();
  }
}

void SettingsDialog::onResetToDefaults() {
  qDebug() << "SettingsDialog::onResetToDefaults()";
  // Only work if Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    m_keybindingTab->resetToDefaults();
  }
}

void SettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Only process shortcuts when Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    // Ctrl+D (Cmd+D on macOS) for Clear Binding
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D) {
      qDebug() << "Ctrl+D pressed via keyPressEvent";
      onClearBinding();
      event->accept();
      return;
    }
    // Ctrl+R (Cmd+R on macOS) for Reset to Defaults
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_R) {
      qDebug() << "Ctrl+R pressed via keyPressEvent";
      onResetToDefaults();
      event->accept();
      return;
    }
  }

  // Pass unhandled events to parent class
  QDialog::keyPressEvent(event);
}

} // namespace Farman

#include "BehaviorTab.h"
#include "settings/Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

namespace Farman {

BehaviorTab::BehaviorTab(QWidget* parent)
  : QWidget(parent)
  , m_sortKeyCombo(nullptr)
  , m_sortOrderCombo(nullptr)
  , m_sortKey2ndCombo(nullptr)
  , m_sortDirsTypeCombo(nullptr)
  , m_sortDotFirstCheck(nullptr)
  , m_sortCaseSensitiveCheck(nullptr)
  , m_showHiddenCheck(nullptr)
  , m_cursorLoopCheck(nullptr)
  , m_restoreLastPathCheck(nullptr)
  , m_confirmOnExitCheck(nullptr)
  , m_windowSizeModeCombo(nullptr)
  , m_windowWidthSpin(nullptr)
  , m_windowHeightSpin(nullptr)
  , m_windowPositionModeCombo(nullptr)
  , m_windowXSpin(nullptr)
  , m_windowYSpin(nullptr) {
  setupUi();
  loadSettings();
}

void BehaviorTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Sort settings
  QGroupBox* sortGroup = new QGroupBox(tr("Default Sort Settings"), this);
  QVBoxLayout* sortGroupLayout = new QVBoxLayout(sortGroup);

  QLabel* sortLabel = new QLabel(
    tr("These settings define the default sorting behavior for new panes. "
       "You can override them per-pane during use."),
    this
  );
  sortLabel->setWordWrap(true);
  sortGroupLayout->addWidget(sortLabel);

  // Grid layout for sort controls (2x2 grid)
  QGridLayout* sortGrid = new QGridLayout();
  sortGrid->setColumnStretch(1, 1);
  sortGrid->setColumnStretch(3, 1);

  // Row 0: Sort by + Then by
  m_sortKeyCombo = new QComboBox(this);
  m_sortKeyCombo->addItem(tr("Name"), static_cast<int>(SortKey::Name));
  m_sortKeyCombo->addItem(tr("Size"), static_cast<int>(SortKey::Size));
  m_sortKeyCombo->addItem(tr("Type"), static_cast<int>(SortKey::Type));
  m_sortKeyCombo->addItem(tr("Last Modified"), static_cast<int>(SortKey::LastModified));
  m_sortKeyCombo->setToolTip(tr("Primary sort key for file lists"));
  sortGrid->addWidget(new QLabel(tr("Sort by:"), this), 0, 0);
  sortGrid->addWidget(m_sortKeyCombo, 0, 1);

  m_sortKey2ndCombo = new QComboBox(this);
  m_sortKey2ndCombo->addItem(tr("None"), static_cast<int>(SortKey::None));
  m_sortKey2ndCombo->addItem(tr("Name"), static_cast<int>(SortKey::Name));
  m_sortKey2ndCombo->addItem(tr("Size"), static_cast<int>(SortKey::Size));
  m_sortKey2ndCombo->addItem(tr("Type"), static_cast<int>(SortKey::Type));
  m_sortKey2ndCombo->addItem(tr("Last Modified"), static_cast<int>(SortKey::LastModified));
  m_sortKey2ndCombo->setToolTip(tr("Secondary sort key used when primary keys are equal"));
  sortGrid->addWidget(new QLabel(tr("Then by:"), this), 0, 2);
  sortGrid->addWidget(m_sortKey2ndCombo, 0, 3);

  // Row 1: Order + Directory Placement
  m_sortOrderCombo = new QComboBox(this);
  m_sortOrderCombo->addItem(tr("Ascending"), static_cast<int>(Qt::AscendingOrder));
  m_sortOrderCombo->addItem(tr("Descending"), static_cast<int>(Qt::DescendingOrder));
  m_sortOrderCombo->setToolTip(tr("Sort direction"));
  sortGrid->addWidget(new QLabel(tr("Order:"), this), 1, 0);
  sortGrid->addWidget(m_sortOrderCombo, 1, 1);

  m_sortDirsTypeCombo = new QComboBox(this);
  m_sortDirsTypeCombo->addItem(tr("Directories First"), static_cast<int>(SortDirsType::First));
  m_sortDirsTypeCombo->addItem(tr("Directories Last"), static_cast<int>(SortDirsType::Last));
  m_sortDirsTypeCombo->addItem(tr("Mixed with Files"), static_cast<int>(SortDirsType::Mixed));
  m_sortDirsTypeCombo->setToolTip(tr("Where to place directories in the sorted list"));
  sortGrid->addWidget(new QLabel(tr("Directory Placement:"), this), 1, 2);
  sortGrid->addWidget(m_sortDirsTypeCombo, 1, 3);

  sortGroupLayout->addLayout(sortGrid);

  // Checkboxes
  m_sortDotFirstCheck = new QCheckBox(tr("Sort dot files first"), this);
  m_sortDotFirstCheck->setToolTip(tr("Place files/folders starting with '.' at the beginning"));
  sortGroupLayout->addWidget(m_sortDotFirstCheck);

  m_sortCaseSensitiveCheck = new QCheckBox(tr("Case sensitive sorting"), this);
  m_sortCaseSensitiveCheck->setToolTip(tr("Enable case-sensitive sorting (A-Z then a-z)"));
  sortGroupLayout->addWidget(m_sortCaseSensitiveCheck);

  mainLayout->addWidget(sortGroup);

  // Display settings
  QGroupBox* displayGroup = new QGroupBox(tr("Display Settings"), this);
  QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

  m_showHiddenCheck = new QCheckBox(tr("Show hidden files by default"), this);
  m_showHiddenCheck->setToolTip(tr("Show files starting with '.' in file lists"));
  displayLayout->addWidget(m_showHiddenCheck);

  mainLayout->addWidget(displayGroup);

  // Navigation settings
  QGroupBox* navigationGroup = new QGroupBox(tr("Navigation Settings"), this);
  QVBoxLayout* navigationLayout = new QVBoxLayout(navigationGroup);

  m_cursorLoopCheck = new QCheckBox(tr("Loop cursor between top and bottom"), this);
  m_cursorLoopCheck->setToolTip(
    tr("Pressing Down on the last row moves the cursor to the top, "
       "and Up on the first row moves it to the bottom"));
  navigationLayout->addWidget(m_cursorLoopCheck);

  mainLayout->addWidget(navigationGroup);

  // Startup settings
  QGroupBox* startupGroup = new QGroupBox(tr("Startup Settings"), this);
  QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);

  m_restoreLastPathCheck = new QCheckBox(tr("Restore last opened paths on startup"), this);
  m_restoreLastPathCheck->setToolTip(tr("Remember and restore the last opened directories when starting the application"));
  startupLayout->addWidget(m_restoreLastPathCheck);

  m_confirmOnExitCheck = new QCheckBox(tr("Confirm on exit"), this);
  m_confirmOnExitCheck->setToolTip(tr("Show confirmation dialog when closing the application"));
  startupLayout->addWidget(m_confirmOnExitCheck);

  mainLayout->addWidget(startupGroup);

  // Window settings
  QGroupBox* windowGroup = new QGroupBox(tr("Window Settings"), this);
  QHBoxLayout* windowLayout = new QHBoxLayout(windowGroup);

  // --- Window size column ---
  QGroupBox* sizeGroup = new QGroupBox(tr("Size"), this);
  QFormLayout* sizeFormLayout = new QFormLayout(sizeGroup);

  m_windowSizeModeCombo = new QComboBox(this);
  m_windowSizeModeCombo->addItem(tr("Default (1200x600)"), static_cast<int>(WindowSizeMode::Default));
  m_windowSizeModeCombo->addItem(tr("Last Session"), static_cast<int>(WindowSizeMode::LastSession));
  m_windowSizeModeCombo->addItem(tr("Custom"), static_cast<int>(WindowSizeMode::Custom));
  connect(m_windowSizeModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &BehaviorTab::onWindowSizeModeChanged);
  sizeFormLayout->addRow(tr("Window Size:"), m_windowSizeModeCombo);

  QWidget* sizeWidget = new QWidget(this);
  QHBoxLayout* sizeLayout = new QHBoxLayout(sizeWidget);
  sizeLayout->setContentsMargins(0, 0, 0, 0);

  m_windowWidthSpin = new QSpinBox(this);
  m_windowWidthSpin->setRange(800, 9999);
  m_windowWidthSpin->setSuffix(tr(" px"));
  sizeLayout->addWidget(new QLabel(tr("Width:"), this));
  sizeLayout->addWidget(m_windowWidthSpin);

  m_windowHeightSpin = new QSpinBox(this);
  m_windowHeightSpin->setRange(600, 9999);
  m_windowHeightSpin->setSuffix(tr(" px"));
  sizeLayout->addWidget(new QLabel(tr("Height:"), this));
  sizeLayout->addWidget(m_windowHeightSpin);
  sizeLayout->addStretch();

  sizeFormLayout->addRow(tr("Custom Size:"), sizeWidget);

  // --- Window position column ---
  QGroupBox* posGroup = new QGroupBox(tr("Position"), this);
  QFormLayout* posFormLayout = new QFormLayout(posGroup);

  m_windowPositionModeCombo = new QComboBox(this);
  m_windowPositionModeCombo->addItem(tr("Default (Center)"), static_cast<int>(WindowPositionMode::Default));
  m_windowPositionModeCombo->addItem(tr("Last Session"), static_cast<int>(WindowPositionMode::LastSession));
  m_windowPositionModeCombo->addItem(tr("Custom"), static_cast<int>(WindowPositionMode::Custom));
  connect(m_windowPositionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &BehaviorTab::onWindowPositionModeChanged);
  posFormLayout->addRow(tr("Window Position:"), m_windowPositionModeCombo);

  QWidget* posWidget = new QWidget(this);
  QHBoxLayout* posLayout = new QHBoxLayout(posWidget);
  posLayout->setContentsMargins(0, 0, 0, 0);

  m_windowXSpin = new QSpinBox(this);
  m_windowXSpin->setRange(0, 9999);
  m_windowXSpin->setSuffix(tr(" px"));
  posLayout->addWidget(new QLabel(tr("X:"), this));
  posLayout->addWidget(m_windowXSpin);

  m_windowYSpin = new QSpinBox(this);
  m_windowYSpin->setRange(0, 9999);
  m_windowYSpin->setSuffix(tr(" px"));
  posLayout->addWidget(new QLabel(tr("Y:"), this));
  posLayout->addWidget(m_windowYSpin);
  posLayout->addStretch();

  posFormLayout->addRow(tr("Custom Position:"), posWidget);

  windowLayout->addWidget(sizeGroup);
  windowLayout->addWidget(posGroup);

  mainLayout->addWidget(windowGroup);

  mainLayout->addStretch();
}

void BehaviorTab::loadSettings() {
  auto& settings = Settings::instance();

  // Load default sort settings from left pane (as template)
  PaneSettings pane = settings.paneSettings(PaneType::Left);

  // Sort key
  for (int i = 0; i < m_sortKeyCombo->count(); ++i) {
    if (m_sortKeyCombo->itemData(i).toInt() == static_cast<int>(pane.sortKey)) {
      m_sortKeyCombo->setCurrentIndex(i);
      break;
    }
  }

  // Sort order
  for (int i = 0; i < m_sortOrderCombo->count(); ++i) {
    if (m_sortOrderCombo->itemData(i).toInt() == static_cast<int>(pane.sortOrder)) {
      m_sortOrderCombo->setCurrentIndex(i);
      break;
    }
  }

  // Sort key 2nd
  for (int i = 0; i < m_sortKey2ndCombo->count(); ++i) {
    if (m_sortKey2ndCombo->itemData(i).toInt() == static_cast<int>(pane.sortKey2nd)) {
      m_sortKey2ndCombo->setCurrentIndex(i);
      break;
    }
  }

  // Sort dirs type
  for (int i = 0; i < m_sortDirsTypeCombo->count(); ++i) {
    if (m_sortDirsTypeCombo->itemData(i).toInt() == static_cast<int>(pane.sortDirsType)) {
      m_sortDirsTypeCombo->setCurrentIndex(i);
      break;
    }
  }

  // Sort options
  m_sortDotFirstCheck->setChecked(pane.sortDotFirst);
  m_sortCaseSensitiveCheck->setChecked(pane.sortCS == Qt::CaseSensitive);

  // Display settings
  m_showHiddenCheck->setChecked(static_cast<bool>(pane.attrFilter & AttrFilter::ShowHidden));

  // Navigation settings
  m_cursorLoopCheck->setChecked(settings.cursorLoop());

  // Startup settings
  m_restoreLastPathCheck->setChecked(settings.restoreLastPath());
  m_confirmOnExitCheck->setChecked(settings.confirmOnExit());

  // Window settings
  for (int i = 0; i < m_windowSizeModeCombo->count(); ++i) {
    if (m_windowSizeModeCombo->itemData(i).toInt() == static_cast<int>(settings.windowSizeMode())) {
      m_windowSizeModeCombo->setCurrentIndex(i);
      break;
    }
  }

  QSize customSize = settings.customWindowSize();
  m_windowWidthSpin->setValue(customSize.width());
  m_windowHeightSpin->setValue(customSize.height());

  for (int i = 0; i < m_windowPositionModeCombo->count(); ++i) {
    if (m_windowPositionModeCombo->itemData(i).toInt() == static_cast<int>(settings.windowPositionMode())) {
      m_windowPositionModeCombo->setCurrentIndex(i);
      break;
    }
  }

  QPoint customPos = settings.customWindowPosition();
  m_windowXSpin->setValue(customPos.x());
  m_windowYSpin->setValue(customPos.y());

  // Update spin box enabled state
  onWindowSizeModeChanged(m_windowSizeModeCombo->currentIndex());
  onWindowPositionModeChanged(m_windowPositionModeCombo->currentIndex());
}

void BehaviorTab::save() {
  auto& settings = Settings::instance();

  // Get current pane settings
  PaneSettings leftPane = settings.paneSettings(PaneType::Left);
  PaneSettings rightPane = settings.paneSettings(PaneType::Right);

  // Update sort settings for both panes
  SortKey sortKey = static_cast<SortKey>(m_sortKeyCombo->currentData().toInt());
  Qt::SortOrder sortOrder = static_cast<Qt::SortOrder>(m_sortOrderCombo->currentData().toInt());
  SortKey sortKey2nd = static_cast<SortKey>(m_sortKey2ndCombo->currentData().toInt());
  SortDirsType sortDirsType = static_cast<SortDirsType>(m_sortDirsTypeCombo->currentData().toInt());
  bool sortDotFirst = m_sortDotFirstCheck->isChecked();
  Qt::CaseSensitivity sortCS = m_sortCaseSensitiveCheck->isChecked() ?
                               Qt::CaseSensitive : Qt::CaseInsensitive;

  leftPane.sortKey = sortKey;
  leftPane.sortOrder = sortOrder;
  leftPane.sortKey2nd = sortKey2nd;
  leftPane.sortDirsType = sortDirsType;
  leftPane.sortDotFirst = sortDotFirst;
  leftPane.sortCS = sortCS;

  rightPane.sortKey = sortKey;
  rightPane.sortOrder = sortOrder;
  rightPane.sortKey2nd = sortKey2nd;
  rightPane.sortDirsType = sortDirsType;
  rightPane.sortDotFirst = sortDotFirst;
  rightPane.sortCS = sortCS;

  // Update display settings
  if (m_showHiddenCheck->isChecked()) {
    leftPane.attrFilter |= AttrFilter::ShowHidden;
    rightPane.attrFilter |= AttrFilter::ShowHidden;
  } else {
    leftPane.attrFilter &= ~static_cast<AttrFilterFlags>(AttrFilter::ShowHidden);
    rightPane.attrFilter &= ~static_cast<AttrFilterFlags>(AttrFilter::ShowHidden);
  }

  // Save pane settings
  settings.setPaneSettings(PaneType::Left, leftPane);
  settings.setPaneSettings(PaneType::Right, rightPane);

  // Save navigation settings
  settings.setCursorLoop(m_cursorLoopCheck->isChecked());

  // Save startup settings
  settings.setRestoreLastPath(m_restoreLastPathCheck->isChecked());
  settings.setConfirmOnExit(m_confirmOnExitCheck->isChecked());

  // Save window settings
  WindowSizeMode sizeMode = static_cast<WindowSizeMode>(m_windowSizeModeCombo->currentData().toInt());
  settings.setWindowSizeMode(sizeMode);
  settings.setCustomWindowSize(QSize(m_windowWidthSpin->value(), m_windowHeightSpin->value()));

  WindowPositionMode posMode = static_cast<WindowPositionMode>(m_windowPositionModeCombo->currentData().toInt());
  settings.setWindowPositionMode(posMode);
  settings.setCustomWindowPosition(QPoint(m_windowXSpin->value(), m_windowYSpin->value()));
}

void BehaviorTab::onWindowSizeModeChanged(int index) {
  WindowSizeMode mode = static_cast<WindowSizeMode>(m_windowSizeModeCombo->itemData(index).toInt());
  bool enableCustomSize = (mode == WindowSizeMode::Custom);
  m_windowWidthSpin->setEnabled(enableCustomSize);
  m_windowHeightSpin->setEnabled(enableCustomSize);
}

void BehaviorTab::onWindowPositionModeChanged(int index) {
  WindowPositionMode mode = static_cast<WindowPositionMode>(m_windowPositionModeCombo->itemData(index).toInt());
  bool enableCustomPos = (mode == WindowPositionMode::Custom);
  m_windowXSpin->setEnabled(enableCustomPos);
  m_windowYSpin->setEnabled(enableCustomPos);
}

} // namespace Farman

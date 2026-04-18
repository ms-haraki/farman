#include "BehaviorTab.h"
#include "settings/Settings.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>

namespace Farman {

BehaviorTab::BehaviorTab(QWidget* parent)
  : QWidget(parent)
  , m_sortKeyCombo(nullptr)
  , m_sortOrderCombo(nullptr)
  , m_sortDirsTypeCombo(nullptr)
  , m_sortDotFirstCheck(nullptr)
  , m_sortCaseSensitiveCheck(nullptr)
  , m_showHiddenCheck(nullptr)
  , m_restoreLastPathCheck(nullptr) {
  setupUi();
  loadSettings();
}

void BehaviorTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Sort settings
  QGroupBox* sortGroup = new QGroupBox(tr("Default Sort Settings"), this);
  QFormLayout* sortLayout = new QFormLayout(sortGroup);

  QLabel* sortLabel = new QLabel(
    tr("These settings define the default sorting behavior for new panes. "
       "You can override them per-pane during use."),
    this
  );
  sortLabel->setWordWrap(true);
  sortLayout->addRow(sortLabel);

  m_sortKeyCombo = new QComboBox(this);
  m_sortKeyCombo->addItem(tr("Name"), static_cast<int>(SortKey::Name));
  m_sortKeyCombo->addItem(tr("Size"), static_cast<int>(SortKey::Size));
  m_sortKeyCombo->addItem(tr("Type"), static_cast<int>(SortKey::Type));
  m_sortKeyCombo->addItem(tr("Last Modified"), static_cast<int>(SortKey::LastModified));
  m_sortKeyCombo->setToolTip(tr("Primary sort key for file lists"));
  sortLayout->addRow(tr("Sort by:"), m_sortKeyCombo);

  m_sortOrderCombo = new QComboBox(this);
  m_sortOrderCombo->addItem(tr("Ascending"), static_cast<int>(Qt::AscendingOrder));
  m_sortOrderCombo->addItem(tr("Descending"), static_cast<int>(Qt::DescendingOrder));
  m_sortOrderCombo->setToolTip(tr("Sort direction"));
  sortLayout->addRow(tr("Order:"), m_sortOrderCombo);

  m_sortDirsTypeCombo = new QComboBox(this);
  m_sortDirsTypeCombo->addItem(tr("Directories First"), static_cast<int>(SortDirsType::First));
  m_sortDirsTypeCombo->addItem(tr("Directories Last"), static_cast<int>(SortDirsType::Last));
  m_sortDirsTypeCombo->addItem(tr("Mixed with Files"), static_cast<int>(SortDirsType::Mixed));
  m_sortDirsTypeCombo->setToolTip(tr("Where to place directories in the sorted list"));
  sortLayout->addRow(tr("Directory Placement:"), m_sortDirsTypeCombo);

  m_sortDotFirstCheck = new QCheckBox(tr("Sort dot files first"), this);
  m_sortDotFirstCheck->setToolTip(tr("Place files/folders starting with '.' at the beginning"));
  sortLayout->addRow(QString(), m_sortDotFirstCheck);

  m_sortCaseSensitiveCheck = new QCheckBox(tr("Case sensitive sorting"), this);
  m_sortCaseSensitiveCheck->setToolTip(tr("Enable case-sensitive sorting (A-Z then a-z)"));
  sortLayout->addRow(QString(), m_sortCaseSensitiveCheck);

  mainLayout->addWidget(sortGroup);

  // Display settings
  QGroupBox* displayGroup = new QGroupBox(tr("Display Settings"), this);
  QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

  m_showHiddenCheck = new QCheckBox(tr("Show hidden files by default"), this);
  m_showHiddenCheck->setToolTip(tr("Show files starting with '.' in file lists"));
  displayLayout->addWidget(m_showHiddenCheck);

  mainLayout->addWidget(displayGroup);

  // Startup settings
  QGroupBox* startupGroup = new QGroupBox(tr("Startup Settings"), this);
  QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);

  m_restoreLastPathCheck = new QCheckBox(tr("Restore last opened paths on startup"), this);
  m_restoreLastPathCheck->setToolTip(tr("Remember and restore the last opened directories when starting the application"));
  startupLayout->addWidget(m_restoreLastPathCheck);

  mainLayout->addWidget(startupGroup);

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

  // Startup settings
  m_restoreLastPathCheck->setChecked(settings.restoreLastPath());
}

void BehaviorTab::save() {
  auto& settings = Settings::instance();

  // Get current pane settings
  PaneSettings leftPane = settings.paneSettings(PaneType::Left);
  PaneSettings rightPane = settings.paneSettings(PaneType::Right);

  // Update sort settings for both panes
  SortKey sortKey = static_cast<SortKey>(m_sortKeyCombo->currentData().toInt());
  Qt::SortOrder sortOrder = static_cast<Qt::SortOrder>(m_sortOrderCombo->currentData().toInt());
  SortDirsType sortDirsType = static_cast<SortDirsType>(m_sortDirsTypeCombo->currentData().toInt());
  bool sortDotFirst = m_sortDotFirstCheck->isChecked();
  Qt::CaseSensitivity sortCS = m_sortCaseSensitiveCheck->isChecked() ?
                               Qt::CaseSensitive : Qt::CaseInsensitive;

  leftPane.sortKey = sortKey;
  leftPane.sortOrder = sortOrder;
  leftPane.sortDirsType = sortDirsType;
  leftPane.sortDotFirst = sortDotFirst;
  leftPane.sortCS = sortCS;

  rightPane.sortKey = sortKey;
  rightPane.sortOrder = sortOrder;
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

  // Save startup settings
  settings.setRestoreLastPath(m_restoreLastPathCheck->isChecked());
}

} // namespace Farman

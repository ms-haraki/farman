#include "BehaviorTab.h"
#include "settings/Settings.h"
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>
#include <QDir>
#include <QLabel>
#include <QStyle>

namespace Farman {

BehaviorTab::BehaviorTab(const QString& leftCurrentPath,
                         const QString& rightCurrentPath,
                         QWidget* parent)
  : QWidget(parent)
  , m_leftCurrentPath(leftCurrentPath)
  , m_rightCurrentPath(rightCurrentPath)
  , m_sortKeyCombo(nullptr)
  , m_sortOrderCombo(nullptr)
  , m_sortKey2ndCombo(nullptr)
  , m_sortDirsTypeCombo(nullptr)
  , m_sortDotFirstCheck(nullptr)
  , m_sortCaseSensitiveCheck(nullptr)
  , m_showHiddenCheck(nullptr)
  , m_cursorLoopCheck(nullptr)
  , m_persistHistoryCheck(nullptr)
  , m_autoRenameTemplateEdit(nullptr)
  , m_defaultDeleteToTrashCheck(nullptr)
  , m_searchExcludeDirsEdit(nullptr)
  , m_leftInitialPathModeCombo(nullptr)
  , m_leftCustomPathEdit(nullptr)
  , m_leftBrowseButton(nullptr)
  , m_rightInitialPathModeCombo(nullptr)
  , m_rightCustomPathEdit(nullptr)
  , m_rightBrowseButton(nullptr)
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

  m_persistHistoryCheck = new QCheckBox(tr("Persist directory history across sessions"), this);
  m_persistHistoryCheck->setToolTip(
    tr("Save each pane's recent directory list on exit and restore it on next launch"));

  // 2 列均等幅の Grid に左寄せで並べる
  QGridLayout* navGrid = new QGridLayout();
  navGrid->setColumnStretch(0, 1);
  navGrid->setColumnStretch(1, 1);
  navGrid->addWidget(m_cursorLoopCheck,     0, 0, Qt::AlignLeft);
  navGrid->addWidget(m_persistHistoryCheck, 0, 1, Qt::AlignLeft);
  navigationLayout->addLayout(navGrid);

  mainLayout->addWidget(navigationGroup);

  // File operation settings（auto-rename suffix と trash 既定を 2 列均等で横並び）
  QGroupBox* fileOpsGroup = new QGroupBox(tr("File Operations"), this);
  QGridLayout* fileOpsLayout = new QGridLayout(fileOpsGroup);
  fileOpsLayout->setColumnStretch(0, 1);
  fileOpsLayout->setColumnStretch(1, 1);

  m_autoRenameTemplateEdit = new QLineEdit(this);
  m_autoRenameTemplateEdit->setToolTip(
    tr("Default suffix template for auto-rename on copy/move conflicts. "
       "Use {n} as the counter placeholder (e.g., ' ({n})' → 'foo (1).txt')."));
  m_autoRenameTemplateEdit->setPlaceholderText(QStringLiteral(" ({n})"));
  m_autoRenameTemplateEdit->setMaximumWidth(160);

  m_defaultDeleteToTrashCheck = new QCheckBox(tr("Move to Trash by default when deleting"), this);
  m_defaultDeleteToTrashCheck->setToolTip(
    tr("Pre-selects the Trash option in the delete confirmation dialog. "
       "Uncheck to default to permanent delete. The choice can still be "
       "overridden in the dialog per operation."));

  // 左カラム: ラベル + 入力欄をまとめて 1 セルに
  QWidget* renameCell = new QWidget(this);
  QHBoxLayout* renameCellLayout = new QHBoxLayout(renameCell);
  renameCellLayout->setContentsMargins(0, 0, 0, 0);
  renameCellLayout->addWidget(new QLabel(tr("Auto-rename suffix:"), this));
  renameCellLayout->addWidget(m_autoRenameTemplateEdit);
  renameCellLayout->addStretch(1);

  fileOpsLayout->addWidget(renameCell,                  0, 0, Qt::AlignLeft);
  fileOpsLayout->addWidget(m_defaultDeleteToTrashCheck, 0, 1, Qt::AlignLeft);

  // Search 除外ディレクトリ（2 列をまたいで横長に）
  m_searchExcludeDirsEdit = new QLineEdit(this);
  m_searchExcludeDirsEdit->setToolTip(
    tr("Directory names (glob, space-separated) to skip when searching.\n"
       "Used as the initial value in the Search dialog's Exclude dirs field."));
  m_searchExcludeDirsEdit->setPlaceholderText(tr("e.g. .* node_modules build"));

  QWidget* excludeCell = new QWidget(this);
  QHBoxLayout* excludeCellLayout = new QHBoxLayout(excludeCell);
  excludeCellLayout->setContentsMargins(0, 0, 0, 0);
  excludeCellLayout->addWidget(new QLabel(tr("Search exclude dirs:"), this));
  excludeCellLayout->addWidget(m_searchExcludeDirsEdit, 1);

  fileOpsLayout->addWidget(excludeCell, 1, 0, 1, 2);

  mainLayout->addWidget(fileOpsGroup);

  // Startup settings
  QGroupBox* startupGroup = new QGroupBox(tr("Startup Settings"), this);
  QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);

  // Per-pane initial path (left / right) — Window Settings と同様の 2 カラム
  QGroupBox* initialPathsGroup = new QGroupBox(tr("Initial Directory"), this);
  QHBoxLayout* initialPathsLayout = new QHBoxLayout(initialPathsGroup);

  auto buildPanePathBox = [this](
      const QString& title,
      QComboBox*& modeCombo,
      QLineEdit*& pathEdit,
      QToolButton*& browseBtn) -> QGroupBox* {
    QGroupBox* box = new QGroupBox(title, this);
    QFormLayout* form = new QFormLayout(box);

    modeCombo = new QComboBox(this);
    modeCombo->addItem(tr("Default (Home)"),   static_cast<int>(InitialPathMode::Default));
    modeCombo->addItem(tr("Last Session"),     static_cast<int>(InitialPathMode::LastSession));
    modeCombo->addItem(tr("Custom"),           static_cast<int>(InitialPathMode::Custom));
    form->addRow(tr("Mode:"), modeCombo);

    QWidget* pathRow = new QWidget(this);
    QHBoxLayout* pathRowLayout = new QHBoxLayout(pathRow);
    pathRowLayout->setContentsMargins(0, 0, 0, 0);
    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText(tr("/path/to/directory"));
    browseBtn = new QToolButton(this);
    browseBtn->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    browseBtn->setToolTip(tr("Browse for folder..."));
    pathRowLayout->addWidget(pathEdit, 1);
    pathRowLayout->addWidget(browseBtn);
    form->addRow(tr("Custom Path:"), pathRow);

    return box;
  };

  initialPathsLayout->addWidget(buildPanePathBox(
    tr("Left Pane"),
    m_leftInitialPathModeCombo, m_leftCustomPathEdit, m_leftBrowseButton));
  initialPathsLayout->addWidget(buildPanePathBox(
    tr("Right Pane"),
    m_rightInitialPathModeCombo, m_rightCustomPathEdit, m_rightBrowseButton));

  connect(m_leftInitialPathModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &BehaviorTab::onLeftInitialPathModeChanged);
  connect(m_rightInitialPathModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &BehaviorTab::onRightInitialPathModeChanged);
  connect(m_leftBrowseButton,  &QToolButton::clicked,
          this, &BehaviorTab::onLeftBrowseInitialPath);
  connect(m_rightBrowseButton, &QToolButton::clicked,
          this, &BehaviorTab::onRightBrowseInitialPath);

  startupLayout->addWidget(initialPathsGroup);

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
  m_persistHistoryCheck->setChecked(settings.persistHistory());

  // File operations
  m_autoRenameTemplateEdit->setText(settings.autoRenameTemplate());
  m_defaultDeleteToTrashCheck->setChecked(settings.defaultDeleteToTrash());
  m_searchExcludeDirsEdit->setText(settings.searchExcludeDirs().join(QLatin1Char(' ')));

  // Startup settings
  auto selectModeByData = [](QComboBox* combo, int value) {
    for (int i = 0; i < combo->count(); ++i) {
      if (combo->itemData(i).toInt() == value) {
        combo->setCurrentIndex(i);
        return;
      }
    }
  };
  selectModeByData(m_leftInitialPathModeCombo,
                   static_cast<int>(settings.initialPathMode(PaneType::Left)));
  selectModeByData(m_rightInitialPathModeCombo,
                   static_cast<int>(settings.initialPathMode(PaneType::Right)));
  // Custom Path 欄の表示はモードに応じて onXxxInitialPathModeChanged が決める
  onLeftInitialPathModeChanged(m_leftInitialPathModeCombo->currentIndex());
  onRightInitialPathModeChanged(m_rightInitialPathModeCombo->currentIndex());

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
  settings.setPersistHistory(m_persistHistoryCheck->isChecked());

  // Save file operation settings
  settings.setAutoRenameTemplate(m_autoRenameTemplateEdit->text());
  settings.setDefaultDeleteToTrash(m_defaultDeleteToTrashCheck->isChecked());
  {
    const QStringList excludeList = m_searchExcludeDirsEdit->text().trimmed()
      .split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    settings.setSearchExcludeDirs(excludeList);
  }

  // Save startup settings
  settings.setInitialPathMode(
    PaneType::Left,
    static_cast<InitialPathMode>(m_leftInitialPathModeCombo->currentData().toInt())
  );
  settings.setInitialPathMode(
    PaneType::Right,
    static_cast<InitialPathMode>(m_rightInitialPathModeCombo->currentData().toInt())
  );
  settings.setCustomInitialPath(PaneType::Left,  m_leftCustomPathEdit->text().trimmed());
  settings.setCustomInitialPath(PaneType::Right, m_rightCustomPathEdit->text().trimmed());
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

void BehaviorTab::onLeftInitialPathModeChanged(int index) {
  InitialPathMode mode = static_cast<InitialPathMode>(
    m_leftInitialPathModeCombo->itemData(index).toInt());
  const bool enableCustom = (mode == InitialPathMode::Custom);
  m_leftCustomPathEdit->setEnabled(enableCustom);
  m_leftBrowseButton->setEnabled(enableCustom);

  // Custom + 保存済み値あり → 保存値を表示、それ以外 → ペインのカレントディレクトリ
  const QString saved = Settings::instance().customInitialPath(PaneType::Left);
  if (mode == InitialPathMode::Custom && !saved.isEmpty()) {
    m_leftCustomPathEdit->setText(saved);
  } else {
    m_leftCustomPathEdit->setText(m_leftCurrentPath);
  }
}

void BehaviorTab::onRightInitialPathModeChanged(int index) {
  InitialPathMode mode = static_cast<InitialPathMode>(
    m_rightInitialPathModeCombo->itemData(index).toInt());
  const bool enableCustom = (mode == InitialPathMode::Custom);
  m_rightCustomPathEdit->setEnabled(enableCustom);
  m_rightBrowseButton->setEnabled(enableCustom);

  const QString saved = Settings::instance().customInitialPath(PaneType::Right);
  if (mode == InitialPathMode::Custom && !saved.isEmpty()) {
    m_rightCustomPathEdit->setText(saved);
  } else {
    m_rightCustomPathEdit->setText(m_rightCurrentPath);
  }
}

void BehaviorTab::onLeftBrowseInitialPath() {
  const QString start = m_leftCustomPathEdit->text().isEmpty()
                        ? QDir::homePath()
                        : m_leftCustomPathEdit->text();
  const QString selected = QFileDialog::getExistingDirectory(
    this, tr("Select initial directory for left pane"), start,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );
  if (!selected.isEmpty()) {
    m_leftCustomPathEdit->setText(selected);
  }
}

void BehaviorTab::onRightBrowseInitialPath() {
  const QString start = m_rightCustomPathEdit->text().isEmpty()
                        ? QDir::homePath()
                        : m_rightCustomPathEdit->text();
  const QString selected = QFileDialog::getExistingDirectory(
    this, tr("Select initial directory for right pane"), start,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
  );
  if (!selected.isEmpty()) {
    m_rightCustomPathEdit->setText(selected);
  }
}

} // namespace Farman

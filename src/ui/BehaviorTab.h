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
  explicit BehaviorTab(QWidget* parent = nullptr);
  ~BehaviorTab() override = default;

  void save();

private:
  void setupUi();
  void loadSettings();

  // Sort settings
  QComboBox*  m_sortKeyCombo;
  QComboBox*  m_sortOrderCombo;
  QComboBox*  m_sortKey2ndCombo;
  QComboBox*  m_sortDirsTypeCombo;
  QCheckBox*  m_sortDotFirstCheck;
  QCheckBox*  m_sortCaseSensitiveCheck;

  // Display settings
  QCheckBox*  m_showHiddenCheck;
  QCheckBox*  m_cursorLoopCheck;
  QCheckBox*  m_persistHistoryCheck;
  QCheckBox*  m_typeAheadDotfilesCheck;

  // File operation settings
  QLineEdit*  m_autoRenameTemplateEdit;
  QCheckBox*  m_defaultDeleteToTrashCheck;
  QCheckBox*  m_progressAutoCloseCheck = nullptr;
  QLineEdit*  m_searchExcludeDirsEdit;

};

} // namespace Farman

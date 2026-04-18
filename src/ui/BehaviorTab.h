#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;

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
  QComboBox*  m_sortDirsTypeCombo;
  QCheckBox*  m_sortDotFirstCheck;
  QCheckBox*  m_sortCaseSensitiveCheck;

  // Display settings
  QCheckBox*  m_showHiddenCheck;

  // Behavior settings
  QCheckBox*  m_restoreLastPathCheck;
};

} // namespace Farman

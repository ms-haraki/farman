#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QSpinBox;

namespace Farman {

class BehaviorTab : public QWidget {
  Q_OBJECT

public:
  explicit BehaviorTab(QWidget* parent = nullptr);
  ~BehaviorTab() override = default;

  void save();

private slots:
  void onWindowSizeModeChanged(int index);
  void onWindowPositionModeChanged(int index);

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

  // Window settings
  QComboBox*  m_windowSizeModeCombo;
  QSpinBox*   m_windowWidthSpin;
  QSpinBox*   m_windowHeightSpin;
  QComboBox*  m_windowPositionModeCombo;
  QSpinBox*   m_windowXSpin;
  QSpinBox*   m_windowYSpin;
};

} // namespace Farman

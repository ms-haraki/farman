#pragma once

#include "settings/Settings.h"
#include <QWidget>
#include <QFont>
#include <QColor>

class QPushButton;
class QComboBox;
class QListWidget;
class QTextEdit;
class QGroupBox;

namespace Farman {

class AppearanceTab : public QWidget {
  Q_OBJECT

public:
  explicit AppearanceTab(QWidget* parent = nullptr);
  ~AppearanceTab() override = default;

  void save();

private slots:
  void onSelectFont();
  void onAddColorRule();
  void onEditColorRule();
  void onRemoveColorRule();
  void onColorRuleSelectionChanged();
  void updatePreview();

private:
  void setupUi();
  void loadSettings();
  QString formatColorRule(const ColorRule& rule) const;

  // Font settings
  QPushButton*   m_fontButton;
  QFont          m_selectedFont;

  // File size format
  QComboBox*     m_fileSizeFormatCombo;

  // Date/time format
  QComboBox*     m_dateTimeFormatCombo;

  // Color rules
  QListWidget*   m_colorRulesList;
  QPushButton*   m_addRuleButton;
  QPushButton*   m_editRuleButton;
  QPushButton*   m_removeRuleButton;

  // Preview
  QTextEdit*     m_previewText;

  // Pending changes
  QList<ColorRule> m_pendingColorRules;
  QString          m_pendingDateTimeFormat;
};

} // namespace Farman

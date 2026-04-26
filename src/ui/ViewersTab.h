#pragma once

#include <QFont>
#include <QWidget>

class QComboBox;
class QPushButton;
class QTabWidget;

namespace Farman {

// 各ビュアー種別ごとの設定をまとめるトップレベルタブ。
// 内部で QTabWidget を持ち、Text / Image / Binary のサブタブで分割する。
class ViewersTab : public QWidget {
  Q_OBJECT

public:
  explicit ViewersTab(QWidget* parent = nullptr);
  ~ViewersTab() override = default;

  void save();

private:
  void setupUi();
  void loadSettings();

  QWidget* buildTextViewerPage();
  QWidget* buildImageViewerPage();
  QWidget* buildBinaryViewerPage();

  QTabWidget* m_subTabs = nullptr;

  // Binary viewer widgets
  QComboBox*   m_binaryUnitCombo     = nullptr;
  QComboBox*   m_binaryEndianCombo   = nullptr;
  QComboBox*   m_binaryEncodingCombo = nullptr;
  QPushButton* m_binaryFontButton    = nullptr;
  QFont        m_binarySelectedFont;
};

} // namespace Farman

#pragma once

#include <QColor>
#include <QFont>
#include <QWidget>

class QCheckBox;
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

  // Text viewer widgets
  QPushButton* m_textFontButton          = nullptr;
  QFont        m_textSelectedFont;
  QComboBox*   m_textEncodingCombo       = nullptr;
  QCheckBox*   m_textShowLineNumbersCheck = nullptr;
  QCheckBox*   m_textWordWrapCheck       = nullptr;
  // 色ボタン (Normal Fg/Bg, Selected Fg/Bg, Line Number Fg/Bg)
  QPushButton* m_textNormalFgButton     = nullptr;
  QPushButton* m_textNormalBgButton     = nullptr;
  QPushButton* m_textSelectedFgButton   = nullptr;
  QPushButton* m_textSelectedBgButton   = nullptr;
  QPushButton* m_textLineNumberFgButton = nullptr;
  QPushButton* m_textLineNumberBgButton = nullptr;
  QColor       m_textNormalFgValue;
  QColor       m_textNormalBgValue;
  QColor       m_textSelectedFgValue;
  QColor       m_textSelectedBgValue;
  QColor       m_textLineNumberFgValue;
  QColor       m_textLineNumberBgValue;

  // Binary viewer widgets
  QComboBox*   m_binaryUnitCombo     = nullptr;
  QComboBox*   m_binaryEndianCombo   = nullptr;
  QComboBox*   m_binaryEncodingCombo = nullptr;
  QPushButton* m_binaryFontButton    = nullptr;
  QFont        m_binarySelectedFont;
};

} // namespace Farman

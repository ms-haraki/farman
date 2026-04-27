#pragma once

#include <QColor>
#include <QFont>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QRadioButton;

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

  // Text viewer widgets
  QLineEdit*   m_textExtensionsEdit       = nullptr;
  QLineEdit*   m_textMimePatternsEdit     = nullptr;
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

  // Image viewer widgets
  QLineEdit*    m_imageExtensionsEdit          = nullptr;
  QLineEdit*    m_imageMimePatternsEdit        = nullptr;
  QComboBox*    m_imageZoomCombo               = nullptr;
  QCheckBox*    m_imageFitToWindowCheck        = nullptr;
  QCheckBox*    m_imageAnimationCheck          = nullptr;
  // 透明部分の表示モード (デフォルト) は radio で選択
  QRadioButton* m_imageTransparencyCheckerRadio = nullptr;
  QRadioButton* m_imageTransparencySolidRadio   = nullptr;
  // 各モードの色設定
  QPushButton*  m_imageCheckerColor1Button     = nullptr;
  QPushButton*  m_imageCheckerColor2Button     = nullptr;
  QPushButton*  m_imageSolidColorButton        = nullptr;
  QColor        m_imageCheckerColor1Value;
  QColor        m_imageCheckerColor2Value;
  QColor        m_imageSolidColorValue;

  // Binary viewer widgets
  QComboBox*   m_binaryUnitCombo     = nullptr;
  QComboBox*   m_binaryEndianCombo   = nullptr;
  QComboBox*   m_binaryEncodingCombo = nullptr;
  QPushButton* m_binaryFontButton    = nullptr;
  QFont        m_binarySelectedFont;
  // 色ボタン: Normal / Selected / Address × Foreground / Background
  QPushButton* m_binaryNormalFgButton   = nullptr;
  QPushButton* m_binaryNormalBgButton   = nullptr;
  QPushButton* m_binarySelectedFgButton = nullptr;
  QPushButton* m_binarySelectedBgButton = nullptr;
  QPushButton* m_binaryAddressFgButton  = nullptr;
  QPushButton* m_binaryAddressBgButton  = nullptr;
  QColor       m_binaryNormalFgValue;
  QColor       m_binaryNormalBgValue;
  QColor       m_binarySelectedFgValue;
  QColor       m_binarySelectedBgValue;
  QColor       m_binaryAddressFgValue;
  QColor       m_binaryAddressBgValue;
};

} // namespace Farman

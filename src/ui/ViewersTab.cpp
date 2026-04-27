#include "ViewersTab.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace Farman {

ViewersTab::ViewersTab(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
  loadSettings();
}

void ViewersTab::setupUi() {
  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);

  // 3 ビュアー分の設定を縦に並べる。長くなりがちなのでスクロール可能に。
  // 他タブのウィンドウ背景に揃えるため、ビューポートを透過にする。
  QScrollArea* scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setStyleSheet("QScrollArea { background: transparent; } "
                        "QScrollArea > QWidget > QWidget { background: transparent; }");
  QWidget* container = new QWidget(scroll);
  QVBoxLayout* col = new QVBoxLayout(container);
  col->setContentsMargins(0, 0, 0, 0);

  auto wrap = [container](const QString& title, QWidget* page) -> QGroupBox* {
    QGroupBox* box = new QGroupBox(title, container);
    QVBoxLayout* l = new QVBoxLayout(box);
    l->setContentsMargins(8, 4, 8, 4);
    l->addWidget(page);
    return box;
  };

  col->addWidget(wrap(tr("Text Viewer"),   buildTextViewerPage()));
  col->addWidget(wrap(tr("Image Viewer"),  buildImageViewerPage()));
  col->addWidget(wrap(tr("Binary Viewer"), buildBinaryViewerPage()));
  col->addStretch();

  scroll->setWidget(container);
  outer->addWidget(scroll);
}

QWidget* ViewersTab::buildTextViewerPage() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* outer = new QVBoxLayout(page);

  // 色ボタン生成のヘルパー (TextView の色設定用)
  auto makeColorButton = [this, page](QColor& storedValue, const QString& dialogTitle,
                                      bool allowInvalid = false) -> QPushButton* {
    QPushButton* btn = new QPushButton(page);
    btn->setFixedWidth(100);
    QObject::connect(btn, &QPushButton::clicked, this,
        [this, &storedValue, btn, dialogTitle, allowInvalid]() {
      QColor initial = storedValue.isValid() ? storedValue : QColor(Qt::black);
      QColorDialog::ColorDialogOptions opts = QColorDialog::ShowAlphaChannel;
      if (allowInvalid) {
        // 不正値 (パレット既定を使う) を選びたいケース用に "リセット" 動作は今回は割愛し、
        // 通常の色選択のみ提供する。
      }
      QColor picked = QColorDialog::getColor(initial, this, dialogTitle, opts);
      if (picked.isValid()) {
        storedValue = picked;
        const QString style = QString("background-color: %1; color: %2;")
          .arg(picked.name(),
               picked.lightness() > 128 ? "black" : "white");
        btn->setStyleSheet(style);
        btn->setText(picked.name());
      }
    });
    return btn;
  };

  // ── 上段: Font / Encoding / Line Numbers / Word Wrap を横並び (左寄せ) ─
  QHBoxLayout* row = new QHBoxLayout();
  row->setSpacing(12);
  auto addPair = [page, row](const QString& labelText, QWidget* w) {
    row->addWidget(new QLabel(labelText, page));
    row->addWidget(w);
  };

  m_textFontButton = new QPushButton(tr("Select Font..."), page);
  m_textFontButton->setToolTip(tr("Choose the font for the text viewer"));
  QObject::connect(m_textFontButton, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    const QFont chosen = QFontDialog::getFont(&ok, m_textSelectedFont, this,
                                              tr("Text Viewer Font"));
    if (ok) {
      m_textSelectedFont = chosen;
      m_textFontButton->setText(QString("%1, %2pt")
        .arg(m_textSelectedFont.family())
        .arg(m_textSelectedFont.pointSize()));
    }
  });
  addPair(tr("Font:"), m_textFontButton);

  m_textEncodingCombo = new QComboBox(page);
  m_textEncodingCombo->setEditable(true);
  m_textEncodingCombo->addItem(QStringLiteral("UTF-8"));
  m_textEncodingCombo->addItem(QStringLiteral("UTF-16LE"));
  m_textEncodingCombo->addItem(QStringLiteral("UTF-16BE"));
  m_textEncodingCombo->addItem(QStringLiteral("Shift_JIS"));
  m_textEncodingCombo->addItem(QStringLiteral("EUC-JP"));
  m_textEncodingCombo->addItem(QStringLiteral("ISO-8859-1"));
  m_textEncodingCombo->setToolTip(tr("Default encoding for opening text files"));
  addPair(tr("Encoding:"), m_textEncodingCombo);

  m_textShowLineNumbersCheck = new QCheckBox(tr("Show line numbers"), page);
  row->addWidget(m_textShowLineNumbersCheck);

  m_textWordWrapCheck = new QCheckBox(tr("Word wrap"), page);
  row->addWidget(m_textWordWrapCheck);

  row->addStretch();
  outer->addLayout(row);

  // ── 下段: カラー (Normal / Selected / Line Number) ───────
  QGridLayout* colors = new QGridLayout();
  // ラベルとボタン列を左に詰めるため、末尾列で残り幅を吸収する
  colors->setColumnStretch(3, 1);
  int r = 0;
  colors->addWidget(new QLabel(tr("Foreground"), page), r, 1, Qt::AlignCenter);
  colors->addWidget(new QLabel(tr("Background"), page), r, 2, Qt::AlignCenter);
  ++r;

  m_textNormalFgButton = makeColorButton(m_textNormalFgValue,   tr("Normal Foreground"));
  m_textNormalBgButton = makeColorButton(m_textNormalBgValue,   tr("Normal Background"));
  colors->addWidget(new QLabel(tr("Normal:"), page), r, 0);
  colors->addWidget(m_textNormalFgButton, r, 1);
  colors->addWidget(m_textNormalBgButton, r, 2);
  ++r;

  m_textSelectedFgButton = makeColorButton(m_textSelectedFgValue, tr("Selected Foreground"));
  m_textSelectedBgButton = makeColorButton(m_textSelectedBgValue, tr("Selected Background"));
  colors->addWidget(new QLabel(tr("Selected:"), page), r, 0);
  colors->addWidget(m_textSelectedFgButton, r, 1);
  colors->addWidget(m_textSelectedBgButton, r, 2);
  ++r;

  m_textLineNumberFgButton = makeColorButton(m_textLineNumberFgValue, tr("Line Number Foreground"));
  m_textLineNumberBgButton = makeColorButton(m_textLineNumberBgValue, tr("Line Number Background"));
  colors->addWidget(new QLabel(tr("Line Number:"), page), r, 0);
  colors->addWidget(m_textLineNumberFgButton, r, 1);
  colors->addWidget(m_textLineNumberBgButton, r, 2);
  ++r;

  outer->addLayout(colors);
  outer->addStretch();
  return page;
}

QWidget* ViewersTab::buildImageViewerPage() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* outer = new QVBoxLayout(page);

  // 上段: Zoom / Fit / Animation を横並び (左寄せ)
  QHBoxLayout* topRow = new QHBoxLayout();
  topRow->setSpacing(12);

  topRow->addWidget(new QLabel(tr("Zoom:"), page));
  m_imageZoomCombo = new QComboBox(page);
  m_imageZoomCombo->setEditable(true);
  for (int p : { 25, 50, 75, 100, 200 }) {
    m_imageZoomCombo->addItem(QString::number(p) + QLatin1Char('%'), p);
  }
  m_imageZoomCombo->setToolTip(tr("Default zoom factor (used when 'Fit to window' is off)"));
  topRow->addWidget(m_imageZoomCombo);

  m_imageFitToWindowCheck = new QCheckBox(tr("Fit image to window"), page);
  m_imageFitToWindowCheck->setToolTip(
    tr("Scale the image to fit within the viewer; zoom factor is ignored while this is on."));
  topRow->addWidget(m_imageFitToWindowCheck);

  m_imageAnimationCheck = new QCheckBox(tr("Play animation (GIF / WebP)"), page);
  topRow->addWidget(m_imageAnimationCheck);

  topRow->addStretch();
  outer->addLayout(topRow);

  // 色ボタン生成のヘルパー
  auto makeColorButton = [this, page](QColor& storedValue, const QString& dialogTitle) -> QPushButton* {
    QPushButton* btn = new QPushButton(page);
    btn->setFixedWidth(120);
    connect(btn, &QPushButton::clicked, this, [this, &storedValue, btn, dialogTitle]() {
      QColor picked = QColorDialog::getColor(
        storedValue.isValid() ? storedValue : QColor(Qt::white),
        this, dialogTitle, QColorDialog::ShowAlphaChannel);
      if (picked.isValid()) {
        storedValue = picked;
        btn->setStyleSheet(QString("background-color: %1; color: %2;")
          .arg(picked.name(),
               picked.lightness() > 128 ? "black" : "white"));
        btn->setText(picked.name());
      }
    });
    return btn;
  };

  // ── Transparency セクション ────────────────
  QGroupBox* transparencyGroup = new QGroupBox(tr("Transparency"), page);
  QVBoxLayout* transparencyLayout = new QVBoxLayout(transparencyGroup);

  // Checker (色 2 つ)
  QGroupBox* checkerGroup = new QGroupBox(transparencyGroup);
  QHBoxLayout* checkerRow = new QHBoxLayout(checkerGroup);
  m_imageTransparencyCheckerRadio = new QRadioButton(tr("Checker"), checkerGroup);
  checkerRow->addWidget(m_imageTransparencyCheckerRadio);
  m_imageCheckerColor1Button = makeColorButton(m_imageCheckerColor1Value, tr("Checker Color 1"));
  m_imageCheckerColor2Button = makeColorButton(m_imageCheckerColor2Value, tr("Checker Color 2"));
  checkerRow->addWidget(new QLabel(tr("Color 1:"), checkerGroup));
  checkerRow->addWidget(m_imageCheckerColor1Button);
  checkerRow->addWidget(new QLabel(tr("Color 2:"), checkerGroup));
  checkerRow->addWidget(m_imageCheckerColor2Button);
  checkerRow->addStretch();
  transparencyLayout->addWidget(checkerGroup);

  // Solid Color (色 1 つ)
  QGroupBox* solidGroup = new QGroupBox(transparencyGroup);
  QHBoxLayout* solidRow = new QHBoxLayout(solidGroup);
  m_imageTransparencySolidRadio = new QRadioButton(tr("Solid Color"), solidGroup);
  solidRow->addWidget(m_imageTransparencySolidRadio);
  m_imageSolidColorButton = makeColorButton(m_imageSolidColorValue, tr("Solid Color"));
  solidRow->addWidget(new QLabel(tr("Color:"), solidGroup));
  solidRow->addWidget(m_imageSolidColorButton);
  solidRow->addStretch();
  transparencyLayout->addWidget(solidGroup);

  outer->addWidget(transparencyGroup);
  outer->addStretch();
  return page;
}

QWidget* ViewersTab::buildBinaryViewerPage() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* outer = new QVBoxLayout(page);

  // 色ボタン生成のヘルパー
  auto makeColorButton = [this, page](QColor& storedValue, const QString& dialogTitle) -> QPushButton* {
    QPushButton* btn = new QPushButton(page);
    btn->setFixedWidth(100);
    QObject::connect(btn, &QPushButton::clicked, this,
        [this, &storedValue, btn, dialogTitle]() {
      QColor picked = QColorDialog::getColor(
        storedValue.isValid() ? storedValue : QColor(Qt::black),
        this, dialogTitle, QColorDialog::ShowAlphaChannel);
      if (picked.isValid()) {
        storedValue = picked;
        btn->setStyleSheet(QString("background-color: %1; color: %2;")
          .arg(picked.name(),
               picked.lightness() > 128 ? "black" : "white"));
        btn->setText(picked.name());
      }
    });
    return btn;
  };

  // 上段: Font / Unit / Endian / String Encoding を 1 行に横並び (左寄せ)
  QHBoxLayout* topRow = new QHBoxLayout();
  topRow->setSpacing(12);
  auto addPair = [page](QHBoxLayout* row, const QString& labelText, QWidget* w) {
    row->addWidget(new QLabel(labelText, page));
    row->addWidget(w);
  };

  m_binaryFontButton = new QPushButton(tr("Select Font..."), page);
  m_binaryFontButton->setToolTip(tr("Choose the font for the binary viewer hex dump"));
  connect(m_binaryFontButton, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    const QFont chosen = QFontDialog::getFont(&ok, m_binarySelectedFont, this,
                                              tr("Binary Viewer Font"));
    if (ok) {
      m_binarySelectedFont = chosen;
      m_binaryFontButton->setText(QString("%1, %2pt")
        .arg(m_binarySelectedFont.family())
        .arg(m_binarySelectedFont.pointSize()));
    }
  });
  addPair(topRow, tr("Font:"), m_binaryFontButton);

  m_binaryUnitCombo = new QComboBox(page);
  m_binaryUnitCombo->addItem(tr("1 Byte"), 1);
  m_binaryUnitCombo->addItem(tr("2 Byte"), 2);
  m_binaryUnitCombo->addItem(tr("4 Byte"), 4);
  m_binaryUnitCombo->addItem(tr("8 Byte"), 8);
  m_binaryUnitCombo->setToolTip(
    tr("Number of bytes per hex column (each line still shows 16 bytes)"));
  addPair(topRow, tr("Unit:"), m_binaryUnitCombo);

  m_binaryEndianCombo = new QComboBox(page);
  m_binaryEndianCombo->addItem(tr("Little Endian"),
    static_cast<int>(BinaryViewerEndian::Little));
  m_binaryEndianCombo->addItem(tr("Big Endian"),
    static_cast<int>(BinaryViewerEndian::Big));
  m_binaryEndianCombo->setToolTip(
    tr("Byte order applied when the unit is larger than 1 byte"));
  addPair(topRow, tr("Endian:"), m_binaryEndianCombo);

  m_binaryEncodingCombo = new QComboBox(page);
  m_binaryEncodingCombo->setEditable(true);
  m_binaryEncodingCombo->addItem(QStringLiteral("UTF-8"));
  m_binaryEncodingCombo->addItem(QStringLiteral("UTF-16LE"));
  m_binaryEncodingCombo->addItem(QStringLiteral("UTF-16BE"));
  m_binaryEncodingCombo->addItem(QStringLiteral("Shift_JIS"));
  m_binaryEncodingCombo->addItem(QStringLiteral("EUC-JP"));
  m_binaryEncodingCombo->addItem(QStringLiteral("ISO-8859-1"));
  m_binaryEncodingCombo->setToolTip(
    tr("Text encoding for the string column on the right"));
  addPair(topRow, tr("String Encoding:"), m_binaryEncodingCombo);
  topRow->addStretch();
  outer->addLayout(topRow);

  // 下段: カラー (Normal / Selected / Address)
  QGridLayout* colors = new QGridLayout();
  colors->setColumnStretch(3, 1);
  int r = 0;
  colors->addWidget(new QLabel(tr("Foreground"), page), r, 1, Qt::AlignCenter);
  colors->addWidget(new QLabel(tr("Background"), page), r, 2, Qt::AlignCenter);
  ++r;

  m_binaryNormalFgButton = makeColorButton(m_binaryNormalFgValue, tr("Normal Foreground"));
  m_binaryNormalBgButton = makeColorButton(m_binaryNormalBgValue, tr("Normal Background"));
  colors->addWidget(new QLabel(tr("Normal:"), page), r, 0);
  colors->addWidget(m_binaryNormalFgButton, r, 1);
  colors->addWidget(m_binaryNormalBgButton, r, 2);
  ++r;

  m_binarySelectedFgButton = makeColorButton(m_binarySelectedFgValue, tr("Selected Foreground"));
  m_binarySelectedBgButton = makeColorButton(m_binarySelectedBgValue, tr("Selected Background"));
  colors->addWidget(new QLabel(tr("Selected:"), page), r, 0);
  colors->addWidget(m_binarySelectedFgButton, r, 1);
  colors->addWidget(m_binarySelectedBgButton, r, 2);
  ++r;

  m_binaryAddressFgButton = makeColorButton(m_binaryAddressFgValue, tr("Address Foreground"));
  m_binaryAddressBgButton = makeColorButton(m_binaryAddressBgValue, tr("Address Background"));
  colors->addWidget(new QLabel(tr("Address:"), page), r, 0);
  colors->addWidget(m_binaryAddressFgButton, r, 1);
  colors->addWidget(m_binaryAddressBgButton, r, 2);
  ++r;

  outer->addLayout(colors);
  outer->addStretch();
  return page;
}

void ViewersTab::loadSettings() {
  const Settings& settings = Settings::instance();

  // Text viewer
  m_textSelectedFont = settings.textViewerFont();
  m_textFontButton->setText(QString("%1, %2pt")
    .arg(m_textSelectedFont.family())
    .arg(m_textSelectedFont.pointSize()));
  m_textEncodingCombo->setCurrentText(settings.textViewerEncoding());
  m_textShowLineNumbersCheck->setChecked(settings.textViewerShowLineNumbers());
  m_textWordWrapCheck->setChecked(settings.textViewerWordWrap());

  auto applyButton = [](QPushButton* btn, const QColor& c) {
    if (!c.isValid()) {
      btn->setStyleSheet(QString());
      btn->setText(QObject::tr("(none)"));
      return;
    }
    btn->setStyleSheet(QString("background-color: %1; color: %2;")
      .arg(c.name(), c.lightness() > 128 ? "black" : "white"));
    btn->setText(c.name());
  };
  m_textNormalFgValue     = settings.textViewerNormalForeground();
  m_textNormalBgValue     = settings.textViewerNormalBackground();
  m_textSelectedFgValue   = settings.textViewerSelectedForeground();
  m_textSelectedBgValue   = settings.textViewerSelectedBackground();
  m_textLineNumberFgValue = settings.textViewerLineNumberForeground();
  m_textLineNumberBgValue = settings.textViewerLineNumberBackground();
  applyButton(m_textNormalFgButton,     m_textNormalFgValue);
  applyButton(m_textNormalBgButton,     m_textNormalBgValue);
  applyButton(m_textSelectedFgButton,   m_textSelectedFgValue);
  applyButton(m_textSelectedBgButton,   m_textSelectedBgValue);
  applyButton(m_textLineNumberFgButton, m_textLineNumberFgValue);
  applyButton(m_textLineNumberBgButton, m_textLineNumberBgValue);

  // Image viewer
  m_imageZoomCombo->setCurrentText(QString::number(settings.imageViewerZoomPercent()) + QLatin1Char('%'));
  m_imageFitToWindowCheck->setChecked(settings.imageViewerFitToWindow());
  m_imageAnimationCheck->setChecked(settings.imageViewerAnimation());

  if (settings.imageViewerTransparencyMode() == ImageTransparencyMode::SolidColor) {
    m_imageTransparencySolidRadio->setChecked(true);
  } else {
    m_imageTransparencyCheckerRadio->setChecked(true);
  }
  m_imageCheckerColor1Value = settings.imageViewerCheckerColor1();
  m_imageCheckerColor2Value = settings.imageViewerCheckerColor2();
  m_imageSolidColorValue    = settings.imageViewerSolidColor();
  applyButton(m_imageCheckerColor1Button, m_imageCheckerColor1Value);
  applyButton(m_imageCheckerColor2Button, m_imageCheckerColor2Value);
  applyButton(m_imageSolidColorButton,    m_imageSolidColorValue);

  const int unitBytes = binaryViewerUnitToBytes(settings.binaryViewerUnit());
  for (int i = 0; i < m_binaryUnitCombo->count(); ++i) {
    if (m_binaryUnitCombo->itemData(i).toInt() == unitBytes) {
      m_binaryUnitCombo->setCurrentIndex(i);
      break;
    }
  }
  for (int i = 0; i < m_binaryEndianCombo->count(); ++i) {
    if (m_binaryEndianCombo->itemData(i).toInt()
        == static_cast<int>(settings.binaryViewerEndian())) {
      m_binaryEndianCombo->setCurrentIndex(i);
      break;
    }
  }
  m_binaryEncodingCombo->setCurrentText(settings.binaryViewerEncoding());

  m_binarySelectedFont = settings.binaryViewerFont();
  m_binaryFontButton->setText(QString("%1, %2pt")
    .arg(m_binarySelectedFont.family())
    .arg(m_binarySelectedFont.pointSize()));

  m_binaryNormalFgValue   = settings.binaryViewerNormalForeground();
  m_binaryNormalBgValue   = settings.binaryViewerNormalBackground();
  m_binarySelectedFgValue = settings.binaryViewerSelectedForeground();
  m_binarySelectedBgValue = settings.binaryViewerSelectedBackground();
  m_binaryAddressFgValue  = settings.binaryViewerAddressForeground();
  m_binaryAddressBgValue  = settings.binaryViewerAddressBackground();
  applyButton(m_binaryNormalFgButton,   m_binaryNormalFgValue);
  applyButton(m_binaryNormalBgButton,   m_binaryNormalBgValue);
  applyButton(m_binarySelectedFgButton, m_binarySelectedFgValue);
  applyButton(m_binarySelectedBgButton, m_binarySelectedBgValue);
  applyButton(m_binaryAddressFgButton,  m_binaryAddressFgValue);
  applyButton(m_binaryAddressBgButton,  m_binaryAddressBgValue);
}

void ViewersTab::save() {
  Settings& settings = Settings::instance();

  // Text viewer
  settings.setTextViewerFont(m_textSelectedFont);
  settings.setTextViewerEncoding(m_textEncodingCombo->currentText().trimmed());
  settings.setTextViewerShowLineNumbers(m_textShowLineNumbersCheck->isChecked());
  settings.setTextViewerWordWrap(m_textWordWrapCheck->isChecked());
  settings.setTextViewerNormalForeground(m_textNormalFgValue);
  settings.setTextViewerNormalBackground(m_textNormalBgValue);
  settings.setTextViewerSelectedForeground(m_textSelectedFgValue);
  settings.setTextViewerSelectedBackground(m_textSelectedBgValue);
  settings.setTextViewerLineNumberForeground(m_textLineNumberFgValue);
  settings.setTextViewerLineNumberBackground(m_textLineNumberBgValue);

  // Image viewer
  {
    QString s = m_imageZoomCombo->currentText().trimmed();
    if (s.endsWith(QLatin1Char('%'))) s.chop(1);
    bool ok = false;
    int v = s.toInt(&ok);
    if (ok) settings.setImageViewerZoomPercent(v);
  }
  settings.setImageViewerFitToWindow(m_imageFitToWindowCheck->isChecked());
  settings.setImageViewerAnimation(m_imageAnimationCheck->isChecked());
  settings.setImageViewerTransparencyMode(
    m_imageTransparencySolidRadio->isChecked()
      ? ImageTransparencyMode::SolidColor
      : ImageTransparencyMode::Checker);
  settings.setImageViewerCheckerColor1(m_imageCheckerColor1Value);
  settings.setImageViewerCheckerColor2(m_imageCheckerColor2Value);
  settings.setImageViewerSolidColor(m_imageSolidColorValue);

  settings.setBinaryViewerUnit(
    bytesToBinaryViewerUnit(m_binaryUnitCombo->currentData().toInt()));
  settings.setBinaryViewerEndian(
    static_cast<BinaryViewerEndian>(m_binaryEndianCombo->currentData().toInt()));
  settings.setBinaryViewerEncoding(m_binaryEncodingCombo->currentText().trimmed());
  settings.setBinaryViewerFont(m_binarySelectedFont);
  settings.setBinaryViewerNormalForeground(m_binaryNormalFgValue);
  settings.setBinaryViewerNormalBackground(m_binaryNormalBgValue);
  settings.setBinaryViewerSelectedForeground(m_binarySelectedFgValue);
  settings.setBinaryViewerSelectedBackground(m_binarySelectedBgValue);
  settings.setBinaryViewerAddressForeground(m_binaryAddressFgValue);
  settings.setBinaryViewerAddressBackground(m_binaryAddressBgValue);
}

} // namespace Farman

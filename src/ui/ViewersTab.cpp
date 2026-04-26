#include "ViewersTab.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Farman {

ViewersTab::ViewersTab(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
  loadSettings();
}

void ViewersTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_subTabs = new QTabWidget(this);
  m_subTabs->addTab(buildTextViewerPage(),   tr("Text Viewer"));
  m_subTabs->addTab(buildImageViewerPage(),  tr("Image Viewer"));
  m_subTabs->addTab(buildBinaryViewerPage(), tr("Binary Viewer"));
  mainLayout->addWidget(m_subTabs);
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

  // ── 上段: Font / Encoding / Line Numbers / Word Wrap ─────
  QFormLayout* form = new QFormLayout();

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
  form->addRow(tr("Font:"), m_textFontButton);

  m_textEncodingCombo = new QComboBox(page);
  m_textEncodingCombo->setEditable(true);
  m_textEncodingCombo->addItem(QStringLiteral("UTF-8"));
  m_textEncodingCombo->addItem(QStringLiteral("UTF-16LE"));
  m_textEncodingCombo->addItem(QStringLiteral("UTF-16BE"));
  m_textEncodingCombo->addItem(QStringLiteral("Shift_JIS"));
  m_textEncodingCombo->addItem(QStringLiteral("EUC-JP"));
  m_textEncodingCombo->addItem(QStringLiteral("ISO-8859-1"));
  m_textEncodingCombo->setToolTip(tr("Default encoding for opening text files"));
  form->addRow(tr("Encoding:"), m_textEncodingCombo);

  m_textShowLineNumbersCheck = new QCheckBox(tr("Show line numbers"), page);
  form->addRow(QString(), m_textShowLineNumbersCheck);

  m_textWordWrapCheck = new QCheckBox(tr("Word wrap"), page);
  form->addRow(QString(), m_textWordWrapCheck);

  outer->addLayout(form);

  // ── 下段: カラー (Normal / Selected / Line Number) ───────
  QGridLayout* colors = new QGridLayout();
  int row = 0;
  colors->addWidget(new QLabel(tr("Foreground"), page), row, 1, Qt::AlignCenter);
  colors->addWidget(new QLabel(tr("Background"), page), row, 2, Qt::AlignCenter);
  ++row;

  m_textNormalFgButton = makeColorButton(m_textNormalFgValue,   tr("Normal Foreground"));
  m_textNormalBgButton = makeColorButton(m_textNormalBgValue,   tr("Normal Background"));
  colors->addWidget(new QLabel(tr("Normal:"), page), row, 0);
  colors->addWidget(m_textNormalFgButton, row, 1);
  colors->addWidget(m_textNormalBgButton, row, 2);
  ++row;

  m_textSelectedFgButton = makeColorButton(m_textSelectedFgValue, tr("Selected Foreground"));
  m_textSelectedBgButton = makeColorButton(m_textSelectedBgValue, tr("Selected Background"));
  colors->addWidget(new QLabel(tr("Selected:"), page), row, 0);
  colors->addWidget(m_textSelectedFgButton, row, 1);
  colors->addWidget(m_textSelectedBgButton, row, 2);
  ++row;

  m_textLineNumberFgButton = makeColorButton(m_textLineNumberFgValue, tr("Line Number Foreground"));
  m_textLineNumberBgButton = makeColorButton(m_textLineNumberBgValue, tr("Line Number Background"));
  colors->addWidget(new QLabel(tr("Line Number:"), page), row, 0);
  colors->addWidget(m_textLineNumberFgButton, row, 1);
  colors->addWidget(m_textLineNumberBgButton, row, 2);
  ++row;

  outer->addLayout(colors);
  outer->addStretch();
  return page;
}

QWidget* ViewersTab::buildImageViewerPage() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(page);
  QLabel* note = new QLabel(tr("Image viewer settings are not yet implemented."), page);
  note->setStyleSheet("color: palette(mid); padding: 8px;");
  layout->addWidget(note);
  layout->addStretch();
  return page;
}

QWidget* ViewersTab::buildBinaryViewerPage() {
  QWidget* page = new QWidget(this);
  QFormLayout* form = new QFormLayout(page);

  m_binaryUnitCombo = new QComboBox(page);
  m_binaryUnitCombo->addItem(tr("1 Byte"), 1);
  m_binaryUnitCombo->addItem(tr("2 Byte"), 2);
  m_binaryUnitCombo->addItem(tr("4 Byte"), 4);
  m_binaryUnitCombo->addItem(tr("8 Byte"), 8);
  m_binaryUnitCombo->setToolTip(
    tr("Number of bytes per hex column (each line still shows 16 bytes)"));
  form->addRow(tr("Unit:"), m_binaryUnitCombo);

  m_binaryEndianCombo = new QComboBox(page);
  m_binaryEndianCombo->addItem(tr("Little Endian"),
    static_cast<int>(BinaryViewerEndian::Little));
  m_binaryEndianCombo->addItem(tr("Big Endian"),
    static_cast<int>(BinaryViewerEndian::Big));
  m_binaryEndianCombo->setToolTip(
    tr("Byte order applied when the unit is larger than 1 byte"));
  form->addRow(tr("Endian:"), m_binaryEndianCombo);

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
  form->addRow(tr("String Encoding:"), m_binaryEncodingCombo);

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
  form->addRow(tr("Font:"), m_binaryFontButton);

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

  settings.setBinaryViewerUnit(
    bytesToBinaryViewerUnit(m_binaryUnitCombo->currentData().toInt()));
  settings.setBinaryViewerEndian(
    static_cast<BinaryViewerEndian>(m_binaryEndianCombo->currentData().toInt()));
  settings.setBinaryViewerEncoding(m_binaryEncodingCombo->currentText().trimmed());
  settings.setBinaryViewerFont(m_binarySelectedFont);
}

} // namespace Farman

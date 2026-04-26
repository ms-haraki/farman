#include "ViewersTab.h"
#include "settings/Settings.h"

#include <QComboBox>
#include <QFontDialog>
#include <QFormLayout>
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
  // Phase 2 以降で実装予定。今は空のプレースホルダー。
  QWidget* page = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(page);
  QLabel* note = new QLabel(tr("Text viewer settings are not yet implemented."), page);
  note->setStyleSheet("color: palette(mid); padding: 8px;");
  layout->addWidget(note);
  layout->addStretch();
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

  settings.setBinaryViewerUnit(
    bytesToBinaryViewerUnit(m_binaryUnitCombo->currentData().toInt()));
  settings.setBinaryViewerEndian(
    static_cast<BinaryViewerEndian>(m_binaryEndianCombo->currentData().toInt()));
  settings.setBinaryViewerEncoding(m_binaryEncodingCombo->currentText().trimmed());
  settings.setBinaryViewerFont(m_binarySelectedFont);
}

} // namespace Farman

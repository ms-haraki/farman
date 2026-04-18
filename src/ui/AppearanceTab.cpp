#include "AppearanceTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QFontDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>

namespace Farman {

AppearanceTab::AppearanceTab(QWidget* parent)
  : QWidget(parent)
  , m_fontButton(nullptr)
  , m_fileSizeFormatCombo(nullptr)
  , m_dateTimeFormatCombo(nullptr)
  , m_colorRulesList(nullptr)
  , m_addRuleButton(nullptr)
  , m_editRuleButton(nullptr)
  , m_removeRuleButton(nullptr)
  , m_previewText(nullptr) {
  setupUi();
  loadSettings();
}

void AppearanceTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Font settings
  QGroupBox* fontGroup = new QGroupBox(tr("Font"), this);
  QFormLayout* fontLayout = new QFormLayout(fontGroup);

  m_fontButton = new QPushButton(tr("Select Font..."), this);
  m_fontButton->setToolTip(tr("Choose the font for the file list"));
  connect(m_fontButton, &QPushButton::clicked, this, &AppearanceTab::onSelectFont);
  fontLayout->addRow(tr("Font:"), m_fontButton);

  mainLayout->addWidget(fontGroup);

  // Format settings
  QGroupBox* formatGroup = new QGroupBox(tr("Display Formats"), this);
  QFormLayout* formatLayout = new QFormLayout(formatGroup);

  m_fileSizeFormatCombo = new QComboBox(this);
  m_fileSizeFormatCombo->addItem(tr("Bytes"), static_cast<int>(FileSizeFormat::Bytes));
  m_fileSizeFormatCombo->addItem(tr("KB/MB/GB (1000)"), static_cast<int>(FileSizeFormat::SI));
  m_fileSizeFormatCombo->addItem(tr("KiB/MiB/GiB (1024)"), static_cast<int>(FileSizeFormat::IEC));
  m_fileSizeFormatCombo->addItem(tr("Auto"), static_cast<int>(FileSizeFormat::Auto));
  m_fileSizeFormatCombo->setToolTip(tr("Choose how file sizes are displayed"));
  connect(m_fileSizeFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &AppearanceTab::updatePreview);
  formatLayout->addRow(tr("File Size:"), m_fileSizeFormatCombo);

  m_dateTimeFormatCombo = new QComboBox(this);
  m_dateTimeFormatCombo->setEditable(true);
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy-MM-dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("dd/MM/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("MM/dd/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd");
  m_dateTimeFormatCombo->setToolTip(tr("Choose the date/time format (Qt format string)"));
  connect(m_dateTimeFormatCombo, &QComboBox::currentTextChanged,
          this, &AppearanceTab::updatePreview);
  formatLayout->addRow(tr("Date/Time:"), m_dateTimeFormatCombo);

  mainLayout->addWidget(formatGroup);

  // Color rules
  QGroupBox* colorGroup = new QGroupBox(tr("File Type Colors"), this);
  QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

  QLabel* colorLabel = new QLabel(tr("Define color rules for different file types using glob patterns (e.g., *.cpp, *.txt)"), this);
  colorLabel->setWordWrap(true);
  colorLayout->addWidget(colorLabel);

  m_colorRulesList = new QListWidget(this);
  connect(m_colorRulesList, &QListWidget::itemSelectionChanged,
          this, &AppearanceTab::onColorRuleSelectionChanged);
  connect(m_colorRulesList, &QListWidget::itemDoubleClicked,
          this, [this](QListWidgetItem*) { onEditColorRule(); });
  colorLayout->addWidget(m_colorRulesList);

  QHBoxLayout* colorButtonLayout = new QHBoxLayout();
  m_addRuleButton = new QPushButton(tr("Add Rule"), this);
  connect(m_addRuleButton, &QPushButton::clicked, this, &AppearanceTab::onAddColorRule);
  colorButtonLayout->addWidget(m_addRuleButton);

  m_editRuleButton = new QPushButton(tr("Edit Rule"), this);
  m_editRuleButton->setEnabled(false);
  connect(m_editRuleButton, &QPushButton::clicked, this, &AppearanceTab::onEditColorRule);
  colorButtonLayout->addWidget(m_editRuleButton);

  m_removeRuleButton = new QPushButton(tr("Remove Rule"), this);
  m_removeRuleButton->setEnabled(false);
  connect(m_removeRuleButton, &QPushButton::clicked, this, &AppearanceTab::onRemoveColorRule);
  colorButtonLayout->addWidget(m_removeRuleButton);

  colorButtonLayout->addStretch();
  colorLayout->addLayout(colorButtonLayout);

  mainLayout->addWidget(colorGroup);

  // Preview
  QGroupBox* previewGroup = new QGroupBox(tr("Preview"), this);
  QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

  m_previewText = new QTextEdit(this);
  m_previewText->setReadOnly(true);
  m_previewText->setMaximumHeight(100);
  previewLayout->addWidget(m_previewText);

  mainLayout->addWidget(previewGroup);

  mainLayout->addStretch();
}

void AppearanceTab::loadSettings() {
  auto& settings = Settings::instance();

  // Load font
  m_selectedFont = settings.font();
  m_fontButton->setText(QString("%1, %2pt").arg(m_selectedFont.family()).arg(m_selectedFont.pointSize()));

  // Load file size format
  FileSizeFormat fmt = settings.fileSizeFormat();
  for (int i = 0; i < m_fileSizeFormatCombo->count(); ++i) {
    if (m_fileSizeFormatCombo->itemData(i).toInt() == static_cast<int>(fmt)) {
      m_fileSizeFormatCombo->setCurrentIndex(i);
      break;
    }
  }

  // Load date/time format
  m_pendingDateTimeFormat = settings.dateTimeFormat();
  m_dateTimeFormatCombo->setCurrentText(m_pendingDateTimeFormat);

  // Load color rules
  m_pendingColorRules = settings.colorRules();
  m_colorRulesList->clear();
  for (const ColorRule& rule : m_pendingColorRules) {
    QListWidgetItem* item = new QListWidgetItem(formatColorRule(rule), m_colorRulesList);
    item->setForeground(rule.foreground);
    item->setBackground(rule.background);
    if (rule.bold) {
      QFont font = item->font();
      font.setBold(true);
      item->setFont(font);
    }
  }

  updatePreview();
}

QString AppearanceTab::formatColorRule(const ColorRule& rule) const {
  QString text = rule.pattern;
  if (rule.bold) {
    text += tr(" (bold)");
  }
  return text;
}

void AppearanceTab::onSelectFont() {
  bool ok;
  QFont font = QFontDialog::getFont(&ok, m_selectedFont, this, tr("Select Font"));
  if (ok) {
    m_selectedFont = font;
    m_fontButton->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    updatePreview();
  }
}

void AppearanceTab::onAddColorRule() {
  // Get pattern
  bool ok;
  QString pattern = QInputDialog::getText(this, tr("Add Color Rule"),
                                         tr("Enter glob pattern (e.g., *.cpp, *.txt):"),
                                         QLineEdit::Normal, QString(), &ok);
  if (!ok || pattern.isEmpty()) {
    return;
  }

  // Get foreground color
  QColor fg = QColorDialog::getColor(Qt::black, this, tr("Select Foreground Color"));
  if (!fg.isValid()) {
    return;
  }

  // Get background color
  QColor bg = QColorDialog::getColor(Qt::white, this, tr("Select Background Color"));
  if (!bg.isValid()) {
    return;
  }

  // Ask if bold
  QMessageBox::StandardButton bold = QMessageBox::question(
    this, tr("Bold Font"), tr("Use bold font for this rule?"),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No
  );

  // Create rule
  ColorRule rule;
  rule.pattern = pattern;
  rule.foreground = fg;
  rule.background = bg;
  rule.bold = (bold == QMessageBox::Yes);

  m_pendingColorRules.append(rule);

  // Add to list
  QListWidgetItem* item = new QListWidgetItem(formatColorRule(rule), m_colorRulesList);
  item->setForeground(fg);
  item->setBackground(bg);
  if (rule.bold) {
    QFont font = item->font();
    font.setBold(true);
    item->setFont(font);
  }

  updatePreview();
}

void AppearanceTab::onEditColorRule() {
  QListWidgetItem* item = m_colorRulesList->currentItem();
  if (!item) {
    return;
  }

  int index = m_colorRulesList->currentRow();
  if (index < 0 || index >= m_pendingColorRules.size()) {
    return;
  }

  ColorRule& rule = m_pendingColorRules[index];

  // Get pattern
  bool ok;
  QString pattern = QInputDialog::getText(this, tr("Edit Color Rule"),
                                         tr("Enter glob pattern (e.g., *.cpp, *.txt):"),
                                         QLineEdit::Normal, rule.pattern, &ok);
  if (!ok || pattern.isEmpty()) {
    return;
  }

  // Get foreground color
  QColor fg = QColorDialog::getColor(rule.foreground, this, tr("Select Foreground Color"));
  if (!fg.isValid()) {
    return;
  }

  // Get background color
  QColor bg = QColorDialog::getColor(rule.background, this, tr("Select Background Color"));
  if (!bg.isValid()) {
    return;
  }

  // Ask if bold
  QMessageBox::StandardButton bold = QMessageBox::question(
    this, tr("Bold Font"), tr("Use bold font for this rule?"),
    QMessageBox::Yes | QMessageBox::No,
    rule.bold ? QMessageBox::Yes : QMessageBox::No
  );

  // Update rule
  rule.pattern = pattern;
  rule.foreground = fg;
  rule.background = bg;
  rule.bold = (bold == QMessageBox::Yes);

  // Update list item
  item->setText(formatColorRule(rule));
  item->setForeground(fg);
  item->setBackground(bg);
  QFont font = item->font();
  font.setBold(rule.bold);
  item->setFont(font);

  updatePreview();
}

void AppearanceTab::onRemoveColorRule() {
  int index = m_colorRulesList->currentRow();
  if (index < 0 || index >= m_pendingColorRules.size()) {
    return;
  }

  m_pendingColorRules.removeAt(index);
  delete m_colorRulesList->takeItem(index);

  updatePreview();
}

void AppearanceTab::onColorRuleSelectionChanged() {
  bool hasSelection = m_colorRulesList->currentItem() != nullptr;
  m_editRuleButton->setEnabled(hasSelection);
  m_removeRuleButton->setEnabled(hasSelection);
}

void AppearanceTab::updatePreview() {
  QString html = "<p><b>Preview:</b></p>";

  html += "<p style='font-family: \"" + m_selectedFont.family() + "\"; font-size: "
          + QString::number(m_selectedFont.pointSize()) + "pt;'>";

  // Show sample files with colors
  QStringList samples = {"document.txt", "script.cpp", "image.png", "archive.zip"};
  for (const QString& sample : samples) {
    QString style = "font-family: \"" + m_selectedFont.family() + "\"; ";
    style += "font-size: " + QString::number(m_selectedFont.pointSize()) + "pt; ";

    // Find matching color rule
    for (const ColorRule& rule : m_pendingColorRules) {
      QRegularExpression re(QRegularExpression::wildcardToRegularExpression(rule.pattern));
      if (re.match(sample).hasMatch()) {
        style += "color: " + rule.foreground.name() + "; ";
        style += "background-color: " + rule.background.name() + "; ";
        if (rule.bold) {
          style += "font-weight: bold; ";
        }
        break;
      }
    }

    html += "<span style='" + style + "'>" + sample + "</span><br>";
  }

  html += "</p>";

  // Show format examples
  html += "<p><b>Formats:</b><br>";
  html += "File size (1234567 bytes): ";

  FileSizeFormat fmt = static_cast<FileSizeFormat>(
    m_fileSizeFormatCombo->currentData().toInt()
  );

  qint64 size = 1234567;
  switch (fmt) {
    case FileSizeFormat::Bytes:
      html += QString::number(size) + " bytes";
      break;
    case FileSizeFormat::SI:
      html += QString::number(size / 1000.0, 'f', 2) + " KB";
      break;
    case FileSizeFormat::IEC:
      html += QString::number(size / 1024.0, 'f', 2) + " KiB";
      break;
    case FileSizeFormat::Auto:
      html += QString::number(size / 1024.0 / 1024.0, 'f', 2) + " MiB";
      break;
  }

  html += "<br>";
  html += "Date/time: " + QDateTime::currentDateTime().toString(
    m_dateTimeFormatCombo->currentText()
  );
  html += "</p>";

  m_previewText->setHtml(html);
}

void AppearanceTab::save() {
  auto& settings = Settings::instance();

  // Save font
  settings.setFont(m_selectedFont);

  // Save file size format
  FileSizeFormat fmt = static_cast<FileSizeFormat>(
    m_fileSizeFormatCombo->currentData().toInt()
  );
  settings.setFileSizeFormat(fmt);

  // Save date/time format
  settings.setDateTimeFormat(m_dateTimeFormatCombo->currentText());

  // Save color rules
  settings.setColorRules(m_pendingColorRules);
}

} // namespace Farman

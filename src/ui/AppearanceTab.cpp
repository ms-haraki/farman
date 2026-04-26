#include "AppearanceTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QFontDialog>
#include <QColorDialog>

namespace Farman {

AppearanceTab::AppearanceTab(QWidget* parent)
  : QWidget(parent)
  , m_fontButton(nullptr)
  , m_fileSizeFormatCombo(nullptr)
  , m_dateTimeFormatCombo(nullptr) {
  setupUi();
  loadSettings();
}

void AppearanceTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // 色ボタン生成のヘルパー
  auto makeColorButton = [this](QColor& storedValue, const QString& dialogTitle) -> QPushButton* {
    QPushButton* btn = new QPushButton(this);
    btn->setFixedWidth(100);
    connect(btn, &QPushButton::clicked, this, [this, &storedValue, btn, dialogTitle]() {
      QColor picked = QColorDialog::getColor(
        storedValue.isValid() ? storedValue : QColor(Qt::black),
        this, dialogTitle, QColorDialog::ShowAlphaChannel);
      if (picked.isValid()) {
        storedValue = picked;
        updateColorButton(btn, picked);
      }
    });
    return btn;
  };

  // ─── Path グループ: フォント + 色 ─────────────────────
  QGroupBox* pathGroup = new QGroupBox(tr("Path"), this);
  QFormLayout* pathForm = new QFormLayout(pathGroup);

  m_pathFontButton = new QPushButton(tr("Select Font..."), this);
  m_pathFontButton->setToolTip(tr("Choose the font for the path label above each pane"));
  connect(m_pathFontButton, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    const QFont chosen = QFontDialog::getFont(&ok, m_pathFontValue, this, tr("Path Font"));
    if (ok) {
      m_pathFontValue = chosen;
      m_pathFontButton->setText(QString("%1, %2pt")
        .arg(m_pathFontValue.family())
        .arg(m_pathFontValue.pointSize()));
    }
  });
  pathForm->addRow(tr("Font:"), m_pathFontButton);

  m_pathFgButton = makeColorButton(m_pathFgValue, tr("Path Foreground Color"));
  m_pathBgButton = makeColorButton(m_pathBgValue, tr("Path Background Color"));
  pathForm->addRow(tr("Foreground:"), m_pathFgButton);
  pathForm->addRow(tr("Background:"), m_pathBgButton);

  mainLayout->addWidget(pathGroup);

  // ─── File List グループ: フォント + 表示形式 + カーソル + 色 ──
  QGroupBox* fileListGroup = new QGroupBox(tr("File List"), this);
  QVBoxLayout* fileListOuter = new QVBoxLayout(fileListGroup);

  // Font + 表示フォーマット
  QFormLayout* fileListForm = new QFormLayout();

  m_fontButton = new QPushButton(tr("Select Font..."), this);
  m_fontButton->setToolTip(tr("Choose the font for the file list"));
  connect(m_fontButton, &QPushButton::clicked, this, &AppearanceTab::onSelectFont);
  fileListForm->addRow(tr("Font:"), m_fontButton);

  m_fileSizeFormatCombo = new QComboBox(this);
  m_fileSizeFormatCombo->addItem(tr("Bytes"), static_cast<int>(FileSizeFormat::Bytes));
  m_fileSizeFormatCombo->addItem(tr("KB/MB/GB (1000)"), static_cast<int>(FileSizeFormat::SI));
  m_fileSizeFormatCombo->addItem(tr("KiB/MiB/GiB (1024)"), static_cast<int>(FileSizeFormat::IEC));
  m_fileSizeFormatCombo->addItem(tr("Auto"), static_cast<int>(FileSizeFormat::Auto));
  m_fileSizeFormatCombo->setToolTip(tr("Choose how file sizes are displayed"));
  fileListForm->addRow(tr("File Size:"), m_fileSizeFormatCombo);

  m_dateTimeFormatCombo = new QComboBox(this);
  m_dateTimeFormatCombo->setEditable(true);
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy-MM-dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("dd/MM/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("MM/dd/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd");
  m_dateTimeFormatCombo->setToolTip(tr("Choose the date/time format (Qt format string)"));
  fileListForm->addRow(tr("Date/Time:"), m_dateTimeFormatCombo);

  fileListOuter->addLayout(fileListForm);

  // Cursor (行カーソルの色)
  QGroupBox* cursorGroup = new QGroupBox(tr("Cursor"), this);
  QFormLayout* cursorForm = new QFormLayout(cursorGroup);
  m_cursorActiveButton   = makeColorButton(m_cursorActiveValue,   tr("Active Cursor Color"));
  m_cursorInactiveButton = makeColorButton(m_cursorInactiveValue, tr("Inactive Cursor Color"));
  cursorForm->addRow(tr("Active:"),   m_cursorActiveButton);
  cursorForm->addRow(tr("Inactive:"), m_cursorInactiveButton);
  fileListOuter->addWidget(cursorGroup);

  // ファイル種別ごとのカラーリング — アクティブ／非アクティブ × 通常／選択の 4 グリッド
  QGroupBox* categoryGroup = new QGroupBox(tr("Colors"), this);
  QVBoxLayout* categoryOuter = new QVBoxLayout(categoryGroup);

  auto buildStateGrid = [this](const QString& title, bool selected, bool inactive) -> QGroupBox* {
    QGroupBox* box = new QGroupBox(title, this);
    QGridLayout* grid = new QGridLayout(box);
    grid->addWidget(new QLabel(tr("Category"),   this), 0, 0);
    grid->addWidget(new QLabel(tr("Foreground"), this), 0, 1);
    grid->addWidget(new QLabel(tr("Background"), this), 0, 2);
    grid->addWidget(new QLabel(tr("Bold"),       this), 0, 3);
    buildCategoryRow(grid, 1, FileCategory::Normal,    tr("Normal"),    selected, inactive);
    buildCategoryRow(grid, 2, FileCategory::Hidden,    tr("Hidden"),    selected, inactive);
    buildCategoryRow(grid, 3, FileCategory::Directory, tr("Directory"), selected, inactive);
    return box;
  };

  QGroupBox* activeGroup = new QGroupBox(tr("Active Pane"), this);
  QHBoxLayout* activeRow = new QHBoxLayout(activeGroup);
  activeRow->addWidget(buildStateGrid(tr("Normal State"),   false, false));
  activeRow->addWidget(buildStateGrid(tr("Selected State"), true,  false));
  categoryOuter->addWidget(activeGroup);

  m_useInactivePaneColorsCheck = new QCheckBox(
    tr("Use custom colors for inactive pane"), this);
  m_useInactivePaneColorsCheck->setToolTip(
    tr("Enable separate colors for the non-active pane. "
       "When off, the active colors are used for both panes."));
  categoryOuter->addWidget(m_useInactivePaneColorsCheck);

  m_inactivePaneGroup = new QGroupBox(tr("Inactive Pane"), this);
  QHBoxLayout* inactiveRow = new QHBoxLayout(m_inactivePaneGroup);
  inactiveRow->addWidget(buildStateGrid(tr("Normal State"),   false, true));
  inactiveRow->addWidget(buildStateGrid(tr("Selected State"), true,  true));
  categoryOuter->addWidget(m_inactivePaneGroup);

  connect(m_useInactivePaneColorsCheck, &QCheckBox::toggled,
          this, [this](bool checked) {
    m_inactivePaneGroup->setEnabled(checked);
  });

  fileListOuter->addWidget(categoryGroup);

  mainLayout->addWidget(fileListGroup, /*stretch*/ 1);
}

void AppearanceTab::loadSettings() {
  auto& settings = Settings::instance();

  // Load fonts
  m_selectedFont = settings.font();
  m_fontButton->setText(QString("%1, %2pt").arg(m_selectedFont.family()).arg(m_selectedFont.pointSize()));
  m_pathFontValue = settings.pathFont();
  m_pathFontButton->setText(QString("%1, %2pt").arg(m_pathFontValue.family()).arg(m_pathFontValue.pointSize()));

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


  // Load category colors: 4 states (active/inactive × normal/selected)
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    auto loadState = [&](CategoryStateRow& r, bool selected, bool inactive) {
      r.value = settings.categoryColor(static_cast<FileCategory>(i), selected, inactive);
      updateColorButton(r.fgButton, r.value.foreground);
      updateColorButton(r.bgButton, r.value.background);
      r.boldCheck->setChecked(r.value.bold);
    };
    loadState(m_categoryRows[i].normal,           false, false);
    loadState(m_categoryRows[i].selected,         true,  false);
    loadState(m_categoryRows[i].inactiveNormal,   false, true);
    loadState(m_categoryRows[i].inactiveSelected, true,  true);
  }

  // 非アクティブペイン設定
  m_useInactivePaneColorsCheck->setChecked(settings.useInactivePaneColors());
  m_inactivePaneGroup->setEnabled(settings.useInactivePaneColors());

  // Path & Cursor
  m_pathFgValue = settings.pathForeground();
  m_pathBgValue = settings.pathBackground();
  updateColorButton(m_pathFgButton, m_pathFgValue);
  updateColorButton(m_pathBgButton, m_pathBgValue);

  m_cursorActiveValue   = settings.cursorColor(true);
  m_cursorInactiveValue = settings.cursorColor(false);
  updateColorButton(m_cursorActiveButton,   m_cursorActiveValue);
  updateColorButton(m_cursorInactiveButton, m_cursorInactiveValue);
}

void AppearanceTab::onSelectFont() {
  bool ok;
  QFont font = QFontDialog::getFont(&ok, m_selectedFont, this, tr("Select Font"));
  if (ok) {
    m_selectedFont = font;
    m_fontButton->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
  }
}

void AppearanceTab::buildCategoryRow(QGridLayout* grid, int row,
                                     FileCategory cat, const QString& label,
                                     bool selected, bool inactive) {
  const int idx = static_cast<int>(cat);
  auto& row_ref = m_categoryRows[idx];
  auto pickSlot = [&row_ref](bool sel, bool inact) -> CategoryStateRow& {
    if (inact)  return sel ? row_ref.inactiveSelected : row_ref.inactiveNormal;
    return           sel ? row_ref.selected         : row_ref.normal;
  };
  CategoryStateRow& r = pickSlot(selected, inactive);

  grid->addWidget(new QLabel(label, this), row, 0);

  r.fgButton = new QPushButton(this);
  r.fgButton->setFixedWidth(80);
  grid->addWidget(r.fgButton, row, 1);
  connect(r.fgButton, &QPushButton::clicked, this, [this, idx, selected, inactive]() {
    auto& rr = m_categoryRows[idx];
    CategoryStateRow& slot = inactive
      ? (selected ? rr.inactiveSelected : rr.inactiveNormal)
      : (selected ? rr.selected         : rr.normal);
    QColor picked = QColorDialog::getColor(
      slot.value.foreground.isValid() ? slot.value.foreground : QColor(Qt::black),
      this, tr("Select Foreground Color"),
      QColorDialog::ShowAlphaChannel);
    if (picked.isValid()) {
      slot.value.foreground = picked;
      updateColorButton(slot.fgButton, picked);
    }
  });

  r.bgButton = new QPushButton(this);
  r.bgButton->setFixedWidth(80);
  grid->addWidget(r.bgButton, row, 2);
  connect(r.bgButton, &QPushButton::clicked, this, [this, idx, selected, inactive]() {
    auto& rr = m_categoryRows[idx];
    CategoryStateRow& slot = inactive
      ? (selected ? rr.inactiveSelected : rr.inactiveNormal)
      : (selected ? rr.selected         : rr.normal);
    QColor picked = QColorDialog::getColor(
      slot.value.background.isValid() ? slot.value.background : QColor(Qt::white),
      this, tr("Select Background Color"),
      QColorDialog::ShowAlphaChannel);
    if (picked.isValid()) {
      slot.value.background = picked;
      updateColorButton(slot.bgButton, picked);
    }
  });

  r.boldCheck = new QCheckBox(this);
  grid->addWidget(r.boldCheck, row, 3);
  connect(r.boldCheck, &QCheckBox::toggled, this,
          [this, idx, selected, inactive](bool checked) {
    auto& rr = m_categoryRows[idx];
    CategoryStateRow& slot = inactive
      ? (selected ? rr.inactiveSelected : rr.inactiveNormal)
      : (selected ? rr.selected         : rr.normal);
    slot.value.bold = checked;
  });
}

void AppearanceTab::updateColorButton(QPushButton* btn, const QColor& color) {
  if (!btn) return;
  if (!color.isValid()) {
    btn->setText(tr("(none)"));
    btn->setStyleSheet(QString());
    return;
  }
  btn->setText(color.name());
  // ボタンにプレビュー色を反映。文字色は輝度から自動選択。
  const int luminance = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
  const QString textColor = (luminance > 160) ? "black" : "white";
  btn->setStyleSheet(QString("background-color: %1; color: %2;")
                       .arg(color.name(), textColor));
}

void AppearanceTab::save() {
  auto& settings = Settings::instance();

  // Save fonts
  settings.setFont(m_selectedFont);
  settings.setPathFont(m_pathFontValue);

  // Save file size format
  FileSizeFormat fmt = static_cast<FileSizeFormat>(
    m_fileSizeFormatCombo->currentData().toInt()
  );
  settings.setFileSizeFormat(fmt);

  // Save date/time format
  settings.setDateTimeFormat(m_dateTimeFormatCombo->currentText());


  // Save category colors: 4 states
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    const FileCategory cat = static_cast<FileCategory>(i);
    settings.setCategoryColor(cat, false, false, m_categoryRows[i].normal.value);
    settings.setCategoryColor(cat, true,  false, m_categoryRows[i].selected.value);
    settings.setCategoryColor(cat, false, true,  m_categoryRows[i].inactiveNormal.value);
    settings.setCategoryColor(cat, true,  true,  m_categoryRows[i].inactiveSelected.value);
  }
  settings.setUseInactivePaneColors(m_useInactivePaneColorsCheck->isChecked());

  // Save path & cursor colors
  settings.setPathForeground(m_pathFgValue);
  settings.setPathBackground(m_pathBgValue);
  settings.setCursorColor(true,  m_cursorActiveValue);
  settings.setCursorColor(false, m_cursorInactiveValue);
}

} // namespace Farman

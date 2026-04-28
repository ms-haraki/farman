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
#include <QSpinBox>
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

  // ラベル + ウィジェットを横並びの 1 ペアにまとめるヘルパー
  auto addPair = [this](QHBoxLayout* row, const QString& labelText, QWidget* w) {
    QLabel* label = new QLabel(labelText, this);
    row->addWidget(label);
    row->addWidget(w);
    row->addSpacing(12);
  };

  // ─── Address グループ: Font / Foreground / Background を横並び ───
  QGroupBox* addressGroup = new QGroupBox(tr("Address"), this);
  QHBoxLayout* addressRow = new QHBoxLayout(addressGroup);

  m_addressFontButton = new QPushButton(tr("Select Font..."), this);
  m_addressFontButton->setToolTip(tr("Choose the font for the address bar above each pane"));
  connect(m_addressFontButton, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    const QFont chosen = QFontDialog::getFont(&ok, m_addressFontValue, this, tr("Address Font"));
    if (ok) {
      m_addressFontValue = chosen;
      m_addressFontButton->setText(QString("%1, %2pt")
        .arg(m_addressFontValue.family())
        .arg(m_addressFontValue.pointSize()));
    }
  });
  addPair(addressRow, tr("Font:"), m_addressFontButton);

  m_addressFgButton = makeColorButton(m_addressFgValue, tr("Address Foreground Color"));
  m_addressBgButton = makeColorButton(m_addressBgValue, tr("Address Background Color"));
  addPair(addressRow, tr("Foreground:"), m_addressFgButton);
  addPair(addressRow, tr("Background:"), m_addressBgButton);
  addressRow->addStretch();
  mainLayout->addWidget(addressGroup);

  // ─── Cursor グループ: Shape / Thickness / Active / Inactive を横並び ─
  QGroupBox* cursorGroup = new QGroupBox(tr("Cursor"), this);
  QHBoxLayout* cursorRow = new QHBoxLayout(cursorGroup);

  m_cursorShapeCombo = new QComboBox(this);
  m_cursorShapeCombo->addItem(tr("Underline"),      static_cast<int>(CursorShape::Underline));
  m_cursorShapeCombo->addItem(tr("Row Background"), static_cast<int>(CursorShape::RowBackground));
  m_cursorShapeCombo->setToolTip(
    tr("Underline: thin line at the bottom of the row.\n"
       "Row Background: fill the entire row with the cursor color."));
  addPair(cursorRow, tr("Shape:"), m_cursorShapeCombo);

  m_cursorThicknessSpin = new QSpinBox(this);
  m_cursorThicknessSpin->setRange(1, 32);
  m_cursorThicknessSpin->setSuffix(tr(" px"));
  m_cursorThicknessSpin->setToolTip(tr("Underline thickness in pixels (used only when shape is Underline)"));
  addPair(cursorRow, tr("Thickness:"), m_cursorThicknessSpin);

  // Underline 以外のときは Thickness を無効化
  connect(m_cursorShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    const auto shape = static_cast<CursorShape>(m_cursorShapeCombo->currentData().toInt());
    m_cursorThicknessSpin->setEnabled(shape == CursorShape::Underline);
  });

  m_cursorActiveButton   = makeColorButton(m_cursorActiveValue,   tr("Active Cursor Color"));
  m_cursorInactiveButton = makeColorButton(m_cursorInactiveValue, tr("Inactive Cursor Color"));
  addPair(cursorRow, tr("Active:"),   m_cursorActiveButton);
  addPair(cursorRow, tr("Inactive:"), m_cursorInactiveButton);
  cursorRow->addStretch();
  mainLayout->addWidget(cursorGroup);

  // ─── File List グループ: フォント/表示形式 + ファイル種別カラー ──
  QGroupBox* fileListGroup = new QGroupBox(tr("File List"), this);
  QVBoxLayout* fileListOuter = new QVBoxLayout(fileListGroup);

  // Font / File Size / Date/Time を横並び
  QHBoxLayout* fileListRow = new QHBoxLayout();

  m_fontButton = new QPushButton(tr("Select Font..."), this);
  m_fontButton->setToolTip(tr("Choose the font for the file list"));
  connect(m_fontButton, &QPushButton::clicked, this, &AppearanceTab::onSelectFont);
  addPair(fileListRow, tr("Font:"), m_fontButton);

  m_fileSizeFormatCombo = new QComboBox(this);
  m_fileSizeFormatCombo->addItem(tr("Bytes"), static_cast<int>(FileSizeFormat::Bytes));
  m_fileSizeFormatCombo->addItem(tr("KB/MB/GB (1000)"), static_cast<int>(FileSizeFormat::SI));
  m_fileSizeFormatCombo->addItem(tr("KiB/MiB/GiB (1024)"), static_cast<int>(FileSizeFormat::IEC));
  m_fileSizeFormatCombo->addItem(tr("Auto"), static_cast<int>(FileSizeFormat::Auto));
  m_fileSizeFormatCombo->setToolTip(tr("Choose how file sizes are displayed"));
  addPair(fileListRow, tr("File Size:"), m_fileSizeFormatCombo);

  m_dateTimeFormatCombo = new QComboBox(this);
  m_dateTimeFormatCombo->setEditable(true);
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy-MM-dd HH:mm:ss");
  m_dateTimeFormatCombo->addItem("dd/MM/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("MM/dd/yyyy HH:mm:ss");
  m_dateTimeFormatCombo->addItem("yyyy/MM/dd");
  m_dateTimeFormatCombo->setToolTip(
    tr("Choose the date/time format. Use yyyy=year, MM=month, dd=day, "
       "HH=hour, mm=minute, ss=second."));
  addPair(fileListRow, tr("Date/Time:"), m_dateTimeFormatCombo);
  fileListRow->addStretch();
  fileListOuter->addLayout(fileListRow);

  // 1 行目に詰めると横に溢れるので、Row Height は 2 行目に分離する。
  // 0 を「Auto」(Qt 既定) として扱うため specialValueText を使う。
  m_rowHeightSpin = new QSpinBox(this);
  m_rowHeightSpin->setRange(0, 200);
  m_rowHeightSpin->setSpecialValueText(tr("Auto"));
  m_rowHeightSpin->setSuffix(tr(" px"));
  m_rowHeightSpin->setToolTip(
    tr("Custom row height for the file list, in pixels. 'Auto' uses the default height."));

  QHBoxLayout* fileListRow2 = new QHBoxLayout();
  addPair(fileListRow2, tr("Row Height:"), m_rowHeightSpin);
  fileListRow2->addStretch();
  fileListOuter->addLayout(fileListRow2);
  // 下のアクティブパネルグループとの間に余白を入れる
  fileListOuter->addSpacing(8);

  // ファイル種別ごとのカラーリング (4 グリッド)
  QVBoxLayout* categoryOuter = fileListOuter;

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
    // 余り行で縦方向のストレッチを吸収して、ヘッダー＋3 行を上端に詰める
    grid->setRowStretch(4, 1);
    return box;
  };

  QGroupBox* activeGroup = new QGroupBox(tr("Active Pane"), this);
  QHBoxLayout* activeRow = new QHBoxLayout(activeGroup);
  activeRow->addWidget(buildStateGrid(tr("Normal State"),   false, false));
  activeRow->addWidget(buildStateGrid(tr("Selected State"), true,  false));
  categoryOuter->addWidget(activeGroup);

  // QGroupBox 自体を checkable にして「非アクティブパネル」のタイトル左に
  // チェックボックスを置く。チェック OFF で中身がグレーアウト + 値を使わなく
  // なる。Settings 上は m_useInactivePaneColors と等価に扱う。
  m_inactivePaneGroup = new QGroupBox(tr("Inactive Pane"), this);
  m_inactivePaneGroup->setCheckable(true);
  m_inactivePaneGroup->setToolTip(
    tr("Enable separate colors for the non-active pane. "
       "When off, the active colors are used for both panes."));
  QHBoxLayout* inactiveRow = new QHBoxLayout(m_inactivePaneGroup);
  inactiveRow->addWidget(buildStateGrid(tr("Normal State"),   false, true));
  inactiveRow->addWidget(buildStateGrid(tr("Selected State"), true,  true));
  categoryOuter->addWidget(m_inactivePaneGroup);

  // 旧 m_useInactivePaneColorsCheck は廃止。グループのチェック状態を
  // そのまま load/save で参照する。

  mainLayout->addWidget(fileListGroup, /*stretch*/ 1);
}

void AppearanceTab::loadSettings() {
  auto& settings = Settings::instance();

  // Load fonts
  m_selectedFont = settings.font();
  m_fontButton->setText(QString("%1, %2pt").arg(m_selectedFont.family()).arg(m_selectedFont.pointSize()));
  m_addressFontValue = settings.addressFont();
  m_addressFontButton->setText(QString("%1, %2pt").arg(m_addressFontValue.family()).arg(m_addressFontValue.pointSize()));

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

  // Load file list row height (0 = Auto = SpinBox の specialValueText が出る)
  m_rowHeightSpin->setValue(settings.fileListRowHeight());


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
  m_inactivePaneGroup->setChecked(settings.useInactivePaneColors());

  // Path & Cursor
  m_addressFgValue = settings.addressForeground();
  m_addressBgValue = settings.addressBackground();
  updateColorButton(m_addressFgButton, m_addressFgValue);
  updateColorButton(m_addressBgButton, m_addressBgValue);

  m_cursorActiveValue   = settings.cursorColor(true);
  m_cursorInactiveValue = settings.cursorColor(false);
  updateColorButton(m_cursorActiveButton,   m_cursorActiveValue);
  updateColorButton(m_cursorInactiveButton, m_cursorInactiveValue);

  for (int i = 0; i < m_cursorShapeCombo->count(); ++i) {
    if (m_cursorShapeCombo->itemData(i).toInt() == static_cast<int>(settings.cursorShape())) {
      m_cursorShapeCombo->setCurrentIndex(i);
      break;
    }
  }
  m_cursorThicknessSpin->setValue(settings.cursorThickness());
  m_cursorThicknessSpin->setEnabled(settings.cursorShape() == CursorShape::Underline);
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
  // setStyleSheet を当てるとネイティブのフォーカスリングが消えるので、
  // :focus でハイライトカラーの枠を出してキー操作でも視認できるようにする。
  const int luminance = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
  const QString textColor = (luminance > 160) ? "black" : "white";
  btn->setStyleSheet(QString(
      "QPushButton { background-color: %1; color: %2; border: 1px solid #888; "
                    "border-radius: 3px; padding: 2px 6px; }"
      "QPushButton:focus { border: 2px solid palette(highlight); padding: 1px 5px; }")
                       .arg(color.name(), textColor));
}

void AppearanceTab::save() {
  auto& settings = Settings::instance();

  // Save fonts
  settings.setFont(m_selectedFont);
  settings.setAddressFont(m_addressFontValue);

  // Save file size format
  FileSizeFormat fmt = static_cast<FileSizeFormat>(
    m_fileSizeFormatCombo->currentData().toInt()
  );
  settings.setFileSizeFormat(fmt);

  // Save date/time format
  settings.setDateTimeFormat(m_dateTimeFormatCombo->currentText());
  settings.setFileListRowHeight(m_rowHeightSpin->value());


  // Save category colors: 4 states
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    const FileCategory cat = static_cast<FileCategory>(i);
    settings.setCategoryColor(cat, false, false, m_categoryRows[i].normal.value);
    settings.setCategoryColor(cat, true,  false, m_categoryRows[i].selected.value);
    settings.setCategoryColor(cat, false, true,  m_categoryRows[i].inactiveNormal.value);
    settings.setCategoryColor(cat, true,  true,  m_categoryRows[i].inactiveSelected.value);
  }
  settings.setUseInactivePaneColors(m_inactivePaneGroup->isChecked());

  // Save path & cursor colors
  settings.setAddressForeground(m_addressFgValue);
  settings.setAddressBackground(m_addressBgValue);
  settings.setCursorColor(true,  m_cursorActiveValue);
  settings.setCursorColor(false, m_cursorInactiveValue);
  settings.setCursorShape(
    static_cast<CursorShape>(m_cursorShapeCombo->currentData().toInt()));
  settings.setCursorThickness(m_cursorThicknessSpin->value());
}

} // namespace Farman

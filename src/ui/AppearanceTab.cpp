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
#include <QRadioButton>
#include <QButtonGroup>

namespace Farman {

AppearanceTab::AppearanceTab(QWidget* parent)
  : QWidget(parent)
  , m_fontButton(nullptr) {
  setupUi();
  loadSettings();
}

void AppearanceTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // ─── Theme グループ: Light / Dark / Auto モード切替 ───
  // モードを切替えると、その瞬間のウィジェット値が「直前まで編集していた側」
  // のシャドースキームへ書き戻され、反対側の値が読み込まれる。
  // OK ボタン押下で両側のスキームと themeMode が一括コミットされる。
  QGroupBox* themeGroup = new QGroupBox(tr("Theme"), this);
  QHBoxLayout* themeRow = new QHBoxLayout(themeGroup);
  themeRow->addWidget(new QLabel(tr("Mode:"), this));
  m_themeAutoRadio  = new QRadioButton(tr("Auto (follow OS)"), this);
  m_themeLightRadio = new QRadioButton(tr("Light"), this);
  m_themeDarkRadio  = new QRadioButton(tr("Dark"),  this);
  m_themeAutoRadio->setToolTip(
    tr("Follow the operating system's appearance setting. "
       "Editing automatically targets the currently active side."));
  // QButtonGroup で排他的に管理 (RadioButton は同一 parent でも自動グルーピング
  // されるが、明示しておくと意図が読みやすい)。
  auto* group = new QButtonGroup(this);
  group->addButton(m_themeAutoRadio);
  group->addButton(m_themeLightRadio);
  group->addButton(m_themeDarkRadio);
  themeRow->addWidget(m_themeAutoRadio);
  themeRow->addWidget(m_themeLightRadio);
  themeRow->addWidget(m_themeDarkRadio);
  themeRow->addSpacing(16);

  m_editingTargetLabel = new QLabel(this);
  m_editingTargetLabel->setStyleSheet("QLabel { color: palette(mid); font-style: italic; }");
  themeRow->addWidget(m_editingTargetLabel);
  themeRow->addStretch();
  mainLayout->addWidget(themeGroup);

  auto onModeChanged = [this](bool checked) {
    if (!checked) return;  // off side のシグナルは無視
    applyThemeModeFromRadios();
  };
  connect(m_themeAutoRadio,  &QRadioButton::toggled, this, onModeChanged);
  connect(m_themeLightRadio, &QRadioButton::toggled, this, onModeChanged);
  connect(m_themeDarkRadio,  &QRadioButton::toggled, this, onModeChanged);

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

  // Font と Row Height を横並びの 1 行に。
  // ファイルサイズ・日時の表示形式は Behavior タブの「List Display」へ移動済み。
  // ここは装飾 (フォント / 色 / Row Height / カテゴリカラー) だけ。
  QHBoxLayout* fileListRow = new QHBoxLayout();

  m_fontButton = new QPushButton(tr("Select Font..."), this);
  m_fontButton->setToolTip(tr("Choose the font for the file list"));
  connect(m_fontButton, &QPushButton::clicked, this, &AppearanceTab::onSelectFont);
  addPair(fileListRow, tr("Font:"), m_fontButton);

  // Row Height — 0 を "Auto" として扱うため specialValueText を使う。
  m_rowHeightSpin = new QSpinBox(this);
  m_rowHeightSpin->setRange(0, 200);
  m_rowHeightSpin->setSpecialValueText(tr("Auto"));
  m_rowHeightSpin->setSuffix(tr(" px"));
  m_rowHeightSpin->setToolTip(
    tr("Custom row height for the file list, in pixels. 'Auto' uses the default height."));
  fileListRow->addSpacing(16);
  addPair(fileListRow, tr("Row Height:"), m_rowHeightSpin);

  fileListRow->addStretch();
  fileListOuter->addLayout(fileListRow);

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

  // ── テーマスキーム: Light/Dark 双方をシャドーバッファに取り込む ──
  m_dialogLight = settings.scheme(ThemeMode::Light);
  m_dialogDark  = settings.scheme(ThemeMode::Dark);
  m_dialogMode  = settings.themeMode();
  // 編集対象 (= 現在 effective なテーマ)
  m_dialogEditingSide = (m_dialogMode == ThemeMode::Auto)
                          ? settings.effectiveTheme()
                          : m_dialogMode;

  // モードラジオを反映 (ハンドラを抑止して再ロードを防ぐ)
  {
    QSignalBlocker b1(m_themeAutoRadio);
    QSignalBlocker b2(m_themeLightRadio);
    QSignalBlocker b3(m_themeDarkRadio);
    switch (m_dialogMode) {
      case ThemeMode::Auto:  m_themeAutoRadio->setChecked(true);  break;
      case ThemeMode::Light: m_themeLightRadio->setChecked(true); break;
      case ThemeMode::Dark:  m_themeDarkRadio->setChecked(true);  break;
    }
  }

  // 編集対象側のスキームをウィジェットに流し込む
  loadFromScheme(currentScheme());
  updateEditingTargetLabel();

  // 非アクティブペイン設定 (テーマ非依存; グローバル設定)
  m_inactivePaneGroup->setChecked(settings.useInactivePaneColors());

  // カーソル形状 / 太さ (テーマ非依存)
  for (int i = 0; i < m_cursorShapeCombo->count(); ++i) {
    if (m_cursorShapeCombo->itemData(i).toInt() == static_cast<int>(settings.cursorShape())) {
      m_cursorShapeCombo->setCurrentIndex(i);
      break;
    }
  }
  m_cursorThicknessSpin->setValue(settings.cursorThickness());
  m_cursorThicknessSpin->setEnabled(settings.cursorShape() == CursorShape::Underline);
}

ColorScheme& AppearanceTab::currentScheme() {
  return m_dialogEditingSide == ThemeMode::Dark ? m_dialogDark : m_dialogLight;
}

void AppearanceTab::loadFromScheme(const ColorScheme& s) {
  // フォント
  m_selectedFont = s.listFont;
  m_fontButton->setText(QString("%1, %2pt").arg(m_selectedFont.family()).arg(m_selectedFont.pointSize()));
  m_addressFontValue = s.addressFont;
  m_addressFontButton->setText(QString("%1, %2pt").arg(m_addressFontValue.family()).arg(m_addressFontValue.pointSize()));

  // 行高
  m_rowHeightSpin->setValue(s.fileListRowHeight);

  // ファイル種別カラー
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    auto setState = [](CategoryStateRow& r, const CategoryColor& v, AppearanceTab* self) {
      r.value = v;
      self->updateColorButton(r.fgButton, v.foreground);
      self->updateColorButton(r.bgButton, v.background);
      r.boldCheck->setChecked(v.bold);
    };
    setState(m_categoryRows[i].normal,           s.categoryColors[i],                 this);
    setState(m_categoryRows[i].selected,         s.selectedCategoryColors[i],         this);
    setState(m_categoryRows[i].inactiveNormal,   s.inactiveCategoryColors[i],         this);
    setState(m_categoryRows[i].inactiveSelected, s.inactiveSelectedCategoryColors[i], this);
  }

  // アドレス / カーソル色
  m_addressFgValue = s.addressForeground;
  m_addressBgValue = s.addressBackground;
  updateColorButton(m_addressFgButton, m_addressFgValue);
  updateColorButton(m_addressBgButton, m_addressBgValue);

  m_cursorActiveValue   = s.cursorActiveColor;
  m_cursorInactiveValue = s.cursorInactiveColor;
  updateColorButton(m_cursorActiveButton,   m_cursorActiveValue);
  updateColorButton(m_cursorInactiveButton, m_cursorInactiveValue);
}

void AppearanceTab::saveToScheme(ColorScheme& s) const {
  s.listFont           = m_selectedFont;
  s.addressFont        = m_addressFontValue;
  s.fileListRowHeight  = m_rowHeightSpin->value();

  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    s.categoryColors[i]                  = m_categoryRows[i].normal.value;
    s.selectedCategoryColors[i]          = m_categoryRows[i].selected.value;
    s.inactiveCategoryColors[i]          = m_categoryRows[i].inactiveNormal.value;
    s.inactiveSelectedCategoryColors[i]  = m_categoryRows[i].inactiveSelected.value;
  }

  s.addressForeground  = m_addressFgValue;
  s.addressBackground  = m_addressBgValue;
  s.cursorActiveColor  = m_cursorActiveValue;
  s.cursorInactiveColor= m_cursorInactiveValue;

  // 注意: colorRules / textViewer / imageViewer / binaryViewer 系のフィールドは
  // この AppearanceTab には UI が無いので触らない (s に元から入っている値が
  // そのまま温存される)。ViewersTab 側で編集する viewer フォントや色は別系統。
}

void AppearanceTab::applyThemeModeFromRadios() {
  // 1. いまウィジェットに表示されている値を、現在編集中の側へ書き戻す
  saveToScheme(currentScheme());

  // 2. ラジオの新しい状態から m_dialogMode を更新
  if      (m_themeLightRadio->isChecked()) m_dialogMode = ThemeMode::Light;
  else if (m_themeDarkRadio->isChecked())  m_dialogMode = ThemeMode::Dark;
  else                                     m_dialogMode = ThemeMode::Auto;

  // 3. 編集対象側を再決定
  m_dialogEditingSide = (m_dialogMode == ThemeMode::Auto)
                          ? Settings::instance().effectiveTheme()
                          : m_dialogMode;

  // 4. 新しい編集対象のスキームをウィジェットへロード
  loadFromScheme(currentScheme());
  updateEditingTargetLabel();
}

void AppearanceTab::updateEditingTargetLabel() {
  if (!m_editingTargetLabel) return;
  m_editingTargetLabel->setText(
    m_dialogEditingSide == ThemeMode::Dark
      ? tr("Editing: Dark scheme")
      : tr("Editing: Light scheme"));
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

  // 1. 現在ウィジェットに乗っている値を編集中側のシャドースキームへ最終反映
  saveToScheme(currentScheme());

  // 2. 両方のスキームを Settings へ流し込む。setScheme は内部で
  //    「アクティブ側ならそのまま m_ にも反映」する。setThemeMode で最終
  //    アクティブ側を確定するため、ここでの順序は light → dark で OK。
  //    (両者の setScheme で save が走るが影響範囲は内部 m_ のみ)
  settings.setScheme(ThemeMode::Light, m_dialogLight);
  settings.setScheme(ThemeMode::Dark,  m_dialogDark);

  // 3. テーマモードを反映 (変更があれば m_ を新側へスワップ + save)
  settings.setThemeMode(m_dialogMode);

  // 4. テーマ非依存のグローバル設定
  settings.setUseInactivePaneColors(m_inactivePaneGroup->isChecked());
  settings.setCursorShape(
    static_cast<CursorShape>(m_cursorShapeCombo->currentData().toInt()));
  settings.setCursorThickness(m_cursorThicknessSpin->value());
}

} // namespace Farman

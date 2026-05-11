#include "ExternalAppsTab.h"
#include "settings/Settings.h"
#include "settings/PresetIO.h"
#include "core/UserCommand.h"
#include "core/UserCommandManager.h"
#include "core/AppPresets.h"
#include "utils/Dialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QStyle>

namespace Farman {

ExternalAppsTab::ExternalAppsTab(QWidget* parent)
  : QWidget(parent) {
  setupUi();
  loadSettings();
}

void ExternalAppsTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // 「Terminal」「Text Editor」を 2 ペインの GroupBox で並べる。
  // 各グループの先頭に「Preset」コンボを置き、選ぶと Program / Arguments が
  // 自動で埋まる。フィールドを手で書き換えると "(Custom)" に戻る。
  // 作業ディレクトリは常にアクティブペインの cwd に固定 (UI から削除)。
  auto buildExtAppBox = [this](
      const QString& title,
      QComboBox*&    presetCombo,
      QLineEdit*&    programEdit,
      QToolButton*&  programBrowse,
      QLineEdit*&    argsEdit,
      QPushButton*&  testButton,
      const QString& argsPlaceholder) -> QGroupBox* {
    QGroupBox* box = new QGroupBox(title, this);
    QFormLayout* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    // Preset コンボ (一番上)
    presetCombo = new QComboBox(this);
    presetCombo->setToolTip(tr(
      "Pick a detected installed application to fill Program / Arguments "
      "automatically. Editing any field below switches back to (Custom)."));
    form->addRow(tr("Preset:"), presetCombo);

    // Program (LineEdit + Browse)
    QWidget* programRow = new QWidget(this);
    QHBoxLayout* programRowLayout = new QHBoxLayout(programRow);
    programRowLayout->setContentsMargins(0, 0, 0, 0);
    programEdit = new QLineEdit(this);
    programEdit->setPlaceholderText(tr("/path/to/executable"));
    programBrowse = new QToolButton(this);
    programBrowse->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    programBrowse->setToolTip(tr("Browse for executable..."));
    programRowLayout->addWidget(programEdit, 1);
    programRowLayout->addWidget(programBrowse);
    form->addRow(tr("Program:"), programRow);

    // Arguments
    argsEdit = new QLineEdit(this);
    argsEdit->setPlaceholderText(argsPlaceholder);
    argsEdit->setToolTip(tr(
      "Space-separated arguments. Use double quotes for arguments with spaces. "
      "Placeholders: {dir} {otherDir} {file} {files} {name} {ext}"));
    form->addRow(tr("Arguments:"), argsEdit);

    // Test launch
    testButton = new QPushButton(tr("Test launch"), this);
    QHBoxLayout* testRow = new QHBoxLayout();
    testRow->setContentsMargins(0, 0, 0, 0);
    testRow->addStretch(1);
    testRow->addWidget(testButton);
    form->addRow(QString(), testRow);

    return box;
  };

  mainLayout->addWidget(buildExtAppBox(
    tr("Terminal"),
    m_terminalPresetCombo,
    m_terminalProgramEdit, m_terminalProgramBrowse,
    m_terminalArgsEdit, m_terminalTestButton,
    tr("e.g. --working-directory={dir}")));

  mainLayout->addWidget(buildExtAppBox(
    tr("Text Editor"),
    m_editorPresetCombo,
    m_editorProgramEdit, m_editorProgramBrowse,
    m_editorArgsEdit, m_editorTestButton,
    tr("e.g. {file}")));

  // 検出 + コンボ初期化。検出は同期で軽量 (10〜20 個のパス確認のみ) なので
  // 構築フローに組み込んで問題ない。
  m_terminalPresets = detectInstalledPresets(QStringLiteral("terminal"));
  m_editorPresets   = detectInstalledPresets(QStringLiteral("editor"));
  populatePresetCombo(m_terminalPresetCombo, m_terminalPresets);
  populatePresetCombo(m_editorPresetCombo,   m_editorPresets);

  connect(m_terminalProgramBrowse, &QToolButton::clicked,
          this, &ExternalAppsTab::onBrowseTerminalProgram);
  connect(m_editorProgramBrowse, &QToolButton::clicked,
          this, &ExternalAppsTab::onBrowseEditorProgram);
  connect(m_terminalTestButton, &QPushButton::clicked,
          this, &ExternalAppsTab::onTestLaunchTerminal);
  connect(m_editorTestButton, &QPushButton::clicked,
          this, &ExternalAppsTab::onTestLaunchEditor);

  // Preset 選択 → フィールドを書き換える
  connect(m_terminalPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ExternalAppsTab::onTerminalPresetChanged);
  connect(m_editorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ExternalAppsTab::onEditorPresetChanged);

  // フィールドを手で書き換え → Preset コンボを "(Custom)" に戻す。
  // textEdited はユーザーの入力でのみ発火し、setText() による反映では発火しない
  // ため、プリセット選択時の自動書き換えと衝突しない。
  connect(m_terminalProgramEdit, &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchTerminalToCustom);
  connect(m_terminalArgsEdit,    &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchTerminalToCustom);
  connect(m_editorProgramEdit,   &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchEditorToCustom);
  connect(m_editorArgsEdit,      &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchEditorToCustom);

  // ─── ユーザー定義コマンドの Import / Export ───
  // 現状この UI からは「コマンドを追加 / 編集 / 削除」する手段がないが、
  // (SPEC.md L763 に「フェーズ 2 以降」と記載) Import / Export なら
  // settings.json を直接編集することなく取り回しできる。組み込み 2 件
  // (terminal / editor) は OS 依存なので対象外: Export では除外され、
  // Import の Replace モードでも上書きされない。
  QGroupBox* ioGroup = new QGroupBox(tr("Custom Commands"), this);
  QHBoxLayout* ioRow = new QHBoxLayout(ioGroup);
  QLabel* ioInfoLabel = new QLabel(
    tr("Custom user commands beyond Terminal / Editor live in settings.json. "
       "Use Import / Export to share them between machines."),
    this);
  ioInfoLabel->setWordWrap(true);
  ioRow->addWidget(ioInfoLabel, /*stretch*/ 1);

  QPushButton* exportButton = new QPushButton(tr("Export..."), this);
  exportButton->setToolTip(tr("Save all custom (non-builtin) user commands to a JSON file"));
  connect(exportButton, &QPushButton::clicked, this, &ExternalAppsTab::onExportCommands);
  ioRow->addWidget(exportButton);

  QPushButton* importButton = new QPushButton(tr("Import..."), this);
  importButton->setToolTip(tr("Load custom user commands from a JSON file"));
  connect(importButton, &QPushButton::clicked, this, &ExternalAppsTab::onImportCommands);
  ioRow->addWidget(importButton);

  mainLayout->addWidget(ioGroup);

  mainLayout->addStretch();
}

void ExternalAppsTab::populatePresetCombo(QComboBox* combo,
                                          const QList<AppPreset>& presets) {
  combo->clear();
  // 1 行目は常に "(Custom)"。userData は空文字。
  combo->addItem(tr("(Custom)"), QString());
  for (const AppPreset& p : presets) {
    combo->addItem(p.displayName, p.id);
  }
}

void ExternalAppsTab::loadSettings() {
  auto& settings = Settings::instance();

  m_nonBuiltinUserCommands.clear();
  const QList<UserCommand> all = settings.userCommands();

  auto fillExtApp = [](const UserCommand& c,
                       QLineEdit* program, QLineEdit* args) {
    program->setText(c.program);
    args->setText(joinArgsTemplate(c.argsTemplate));
    // workingDirTemplate は UI に持たない (常にアクティブペイン)
  };

  bool gotTerm = false, gotEdit = false;
  for (const UserCommand& c : all) {
    if (c.builtin && c.builtinKind == QLatin1String("terminal") && !gotTerm) {
      fillExtApp(c, m_terminalProgramEdit, m_terminalArgsEdit);
      gotTerm = true;
    } else if (c.builtin && c.builtinKind == QLatin1String("editor") && !gotEdit) {
      fillExtApp(c, m_editorProgramEdit, m_editorArgsEdit);
      gotEdit = true;
    } else {
      m_nonBuiltinUserCommands.append(c);
    }
  }

  // 何らかの理由で builtin 2 件のいずれかが欠けている場合 (ユーザーが
  // settings.json を手で削った等) はプラットフォーム既定で UI を埋め直す。
  // これにより save 時にも 2 件が必ず復元される。
  if (!gotTerm || !gotEdit) {
    const QList<UserCommand> defaults = defaultBuiltinUserCommands();
    for (const UserCommand& d : defaults) {
      if (!gotTerm && d.builtinKind == QLatin1String("terminal")) {
        fillExtApp(d, m_terminalProgramEdit, m_terminalArgsEdit);
        gotTerm = true;
      } else if (!gotEdit && d.builtinKind == QLatin1String("editor")) {
        fillExtApp(d, m_editorProgramEdit, m_editorArgsEdit);
        gotEdit = true;
      }
    }
  }

  // フィールドが揃ったところで Preset コンボの初期選択を決める。
  // 現在値が検出済プリセットのいずれかと完全一致すればそれを、しなければ
  // "(Custom)" を選ぶ。setCurrentIndex() の発火で onTerminalPresetChanged が
  // 呼ばれるが、(a) 値は既に同じなので setText が走っても変化はなく、
  // (b) "(Custom)" 選択ロジックは何もしないので副作用はない。
  auto selectMatchedPreset = [](QComboBox* combo, const QList<AppPreset>& presets,
                                QLineEdit* program, QLineEdit* args) {
    const QString matchedId = matchPresetId(presets,
                                            program->text().trimmed(),
                                            splitArgsTemplate(args->text()));
    int index = 0; // "(Custom)"
    for (int i = 1; i < combo->count(); ++i) {
      if (combo->itemData(i).toString() == matchedId) { index = i; break; }
    }
    QSignalBlocker blocker(combo);
    combo->setCurrentIndex(index);
  };
  selectMatchedPreset(m_terminalPresetCombo, m_terminalPresets,
                      m_terminalProgramEdit, m_terminalArgsEdit);
  selectMatchedPreset(m_editorPresetCombo, m_editorPresets,
                      m_editorProgramEdit, m_editorArgsEdit);
}

void ExternalAppsTab::save() {
  auto& settings = Settings::instance();

  auto buildBuiltin = [](const QString& kind, const QString& name,
                         const QLineEdit* program, const QLineEdit* args) {
    UserCommand c;
    c.id                  = QStringLiteral("user.cmd.") + kind;
    c.name                = name;
    c.builtin             = true;
    c.builtinKind         = kind;
    c.showInToolsMenu     = true;
    c.program             = program->text().trimmed();
    c.argsTemplate        = splitArgsTemplate(args->text());
    // 作業ディレクトリは常にアクティブペイン (= empty)。
    c.workingDirTemplate  = QString();
    return c;
  };

  QList<UserCommand> updated;
  updated.append(buildBuiltin(QStringLiteral("terminal"), tr("Terminal"),
                              m_terminalProgramEdit, m_terminalArgsEdit));
  updated.append(buildBuiltin(QStringLiteral("editor"), tr("Text Editor"),
                              m_editorProgramEdit, m_editorArgsEdit));
  // ロード時に避けておいた非 builtin エントリを末尾に戻す。
  for (const UserCommand& c : m_nonBuiltinUserCommands) updated.append(c);
  settings.setUserCommands(updated);
}

namespace {

void browseProgramInto(QWidget* parent, QLineEdit* target, const QString& title) {
  QString start = target->text().trimmed();
  if (start.isEmpty() || !QFileInfo(start).exists()) {
    start = QDir::homePath();
  }
  const QString selected = QFileDialog::getOpenFileName(parent, title, start);
  if (!selected.isEmpty()) {
    target->setText(selected);
  }
}

UserCommand buildTransient(const QString& kind, const QString& name,
                           QLineEdit* programEdit, QLineEdit* argsEdit) {
  UserCommand c;
  c.id                 = QStringLiteral("user.cmd.") + kind;
  c.name               = name;
  c.builtin            = true;
  c.builtinKind        = kind;
  c.program            = programEdit->text().trimmed();
  c.argsTemplate       = splitArgsTemplate(argsEdit->text());
  // 作業ディレクトリはアクティブペインに固定 (= empty)
  c.workingDirTemplate = QString();
  return c;
}

} // anonymous namespace

void ExternalAppsTab::onBrowseTerminalProgram() {
  browseProgramInto(this, m_terminalProgramEdit, tr("Choose terminal executable"));
}

void ExternalAppsTab::onBrowseEditorProgram() {
  browseProgramInto(this, m_editorProgramEdit, tr("Choose editor executable"));
}

void ExternalAppsTab::onTestLaunchTerminal() {
  const UserCommand c = buildTransient(
    QStringLiteral("terminal"), tr("Terminal"),
    m_terminalProgramEdit, m_terminalArgsEdit);
  QString err;
  if (!UserCommandManager::instance().runTransient(c, &err)) {
    QMessageBox::warning(this, tr("Test launch failed"), err);
  }
}

void ExternalAppsTab::onTestLaunchEditor() {
  const UserCommand c = buildTransient(
    QStringLiteral("editor"), tr("Text Editor"),
    m_editorProgramEdit, m_editorArgsEdit);
  QString err;
  if (!UserCommandManager::instance().runTransient(c, &err)) {
    QMessageBox::warning(this, tr("Test launch failed"), err);
  }
}

namespace {

// 共通: 選択された Preset のフィールドを LineEdit 群に流し込む。
// "(Custom)" (= userData 空) なら何もしない。setText() は textEdited を発火
// しないので、switch*ToCustom() に取られて Custom に戻る心配はない。
// workingDirTemplate は UI から削除したためここでも反映しない (実行時は
// アクティブペインの cwd が使われる)。
void applyPresetTo(QComboBox* combo,
                   const QList<AppPreset>& presets,
                   QLineEdit* program, QLineEdit* args) {
  const QString id = combo->currentData().toString();
  if (id.isEmpty()) return;  // (Custom)
  for (const AppPreset& p : presets) {
    if (p.id == id) {
      program->setText(p.program);
      args->setText(joinArgsTemplate(p.argsTemplate));
      return;
    }
  }
}

} // anonymous namespace

void ExternalAppsTab::onTerminalPresetChanged(int /*index*/) {
  applyPresetTo(m_terminalPresetCombo, m_terminalPresets,
                m_terminalProgramEdit, m_terminalArgsEdit);
}

void ExternalAppsTab::onEditorPresetChanged(int /*index*/) {
  applyPresetTo(m_editorPresetCombo, m_editorPresets,
                m_editorProgramEdit, m_editorArgsEdit);
}

void ExternalAppsTab::switchTerminalToCustom() {
  // 既に "(Custom)" (index 0) なら何もしない。setCurrentIndex(0) を毎回呼ぶと
  // currentIndexChanged が連発するので、既に 0 のときはスキップ。
  if (m_terminalPresetCombo->currentIndex() == 0) return;
  QSignalBlocker blocker(m_terminalPresetCombo);
  m_terminalPresetCombo->setCurrentIndex(0);
}

void ExternalAppsTab::switchEditorToCustom() {
  if (m_editorPresetCombo->currentIndex() == 0) return;
  QSignalBlocker blocker(m_editorPresetCombo);
  m_editorPresetCombo->setCurrentIndex(0);
}

void ExternalAppsTab::onExportCommands() {
  if (m_nonBuiltinUserCommands.isEmpty()) {
    QMessageBox::information(this, tr("Export Commands"),
                             tr("No custom commands to export. "
                                "(The built-in Terminal and Editor entries are not included.)"));
    return;
  }

  const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  const QString path = QFileDialog::getSaveFileName(
    this, tr("Export Commands"),
    defaultDir + QStringLiteral("/farman-commands.json"),
    tr("JSON Files (*.json)"));
  if (path.isEmpty()) return;

  if (auto r = PresetIO::exportUserCommandsToFile(path, m_nonBuiltinUserCommands); !r.ok) {
    QMessageBox::warning(this, tr("Export Commands"), r.error);
  }
}

void ExternalAppsTab::onImportCommands() {
  const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  const QString path = QFileDialog::getOpenFileName(
    this, tr("Import Commands"), defaultDir,
    tr("JSON Files (*.json)"));
  if (path.isEmpty()) return;

  // ファイルを読み出す
  QList<UserCommand> imported;
  if (auto r = PresetIO::importUserCommandsFromFile(path, imported); !r.ok) {
    QMessageBox::warning(this, tr("Import Commands"), r.error);
    return;
  }

  // builtin が紛れていたら除外 (組み込み terminal/editor は別 UI 管理)。
  QList<UserCommand> filtered;
  for (const UserCommand& c : imported) {
    if (!c.builtin) filtered.append(c);
  }
  if (filtered.isEmpty()) {
    QMessageBox::information(this, tr("Import Commands"),
                             tr("The file contains no custom (non-builtin) commands."));
    return;
  }

  // Replace / Append を選択させる
  QMessageBox box(this);
  box.setIcon(QMessageBox::Question);
  box.setWindowTitle(tr("Import Commands"));
  box.setText(tr("Imported %1 command(s). How should they be applied?").arg(filtered.size()));
  QPushButton* replaceBtn = box.addButton(tr("Replace"), QMessageBox::AcceptRole);
  QPushButton* appendBtn  = box.addButton(tr("Append"),  QMessageBox::AcceptRole);
  box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(appendBtn);
  box.exec();
  QAbstractButton* clicked = box.clickedButton();
  if (clicked != replaceBtn && clicked != appendBtn) return;

  if (clicked == replaceBtn) {
    m_nonBuiltinUserCommands = filtered;
  } else {
    // Append: 既存の id と衝突したら数字接尾辞でリネーム (id 衝突防止)
    QSet<QString> usedIds;
    for (const UserCommand& c : m_nonBuiltinUserCommands) usedIds.insert(c.id);
    for (UserCommand c : filtered) {
      QString base = c.id;
      QString id   = base;
      int    n     = 2;
      while (usedIds.contains(id)) {
        id = QStringLiteral("%1-%2").arg(base).arg(n++);
      }
      c.id = id;
      usedIds.insert(id);
      m_nonBuiltinUserCommands.append(c);
    }
  }

  QMessageBox::information(this, tr("Import Commands"),
                           tr("Imported %1 custom command(s). "
                              "They will be saved when you close Settings with OK.")
                             .arg(filtered.size()));
}

} // namespace Farman

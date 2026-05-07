#include "ExternalAppsTab.h"
#include "settings/Settings.h"
#include "core/UserCommand.h"
#include "core/UserCommandManager.h"
#include "core/AppPresets.h"
#include "utils/Dialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
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
  // 各グループの先頭に「Preset」コンボを置き、選ぶと Program / Arguments /
  // Working Dir が自動で埋まる。フィールドを手で書き換えると "(Custom)" に戻る。
  // フェーズ 2 でユーザー定義コマンドの一覧 UI をこの下に追加する想定。
  auto buildExtAppBox = [this](
      const QString& title,
      QComboBox*&    presetCombo,
      QLineEdit*&    programEdit,
      QToolButton*&  programBrowse,
      QLineEdit*&    argsEdit,
      QLineEdit*&    workingDirEdit,
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

    // Working dir
    workingDirEdit = new QLineEdit(this);
    workingDirEdit->setPlaceholderText(tr("(active pane)"));
    workingDirEdit->setToolTip(tr(
      "Working directory of the launched process. "
      "Empty = the active pane's current directory."));
    form->addRow(tr("Working Dir:"), workingDirEdit);

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
    m_terminalArgsEdit, m_terminalWorkingDirEdit, m_terminalTestButton,
    tr("e.g. --working-directory={dir}")));

  mainLayout->addWidget(buildExtAppBox(
    tr("Text Editor"),
    m_editorPresetCombo,
    m_editorProgramEdit, m_editorProgramBrowse,
    m_editorArgsEdit, m_editorWorkingDirEdit, m_editorTestButton,
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
  connect(m_terminalProgramEdit,    &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchTerminalToCustom);
  connect(m_terminalArgsEdit,       &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchTerminalToCustom);
  connect(m_terminalWorkingDirEdit, &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchTerminalToCustom);
  connect(m_editorProgramEdit,      &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchEditorToCustom);
  connect(m_editorArgsEdit,         &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchEditorToCustom);
  connect(m_editorWorkingDirEdit,   &QLineEdit::textEdited,
          this, &ExternalAppsTab::switchEditorToCustom);

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
                       QLineEdit* program, QLineEdit* args, QLineEdit* wd) {
    program->setText(c.program);
    args->setText(joinArgsTemplate(c.argsTemplate));
    wd->setText(c.workingDirTemplate);
  };

  bool gotTerm = false, gotEdit = false;
  for (const UserCommand& c : all) {
    if (c.builtin && c.builtinKind == QLatin1String("terminal") && !gotTerm) {
      fillExtApp(c, m_terminalProgramEdit, m_terminalArgsEdit, m_terminalWorkingDirEdit);
      gotTerm = true;
    } else if (c.builtin && c.builtinKind == QLatin1String("editor") && !gotEdit) {
      fillExtApp(c, m_editorProgramEdit, m_editorArgsEdit, m_editorWorkingDirEdit);
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
        fillExtApp(d, m_terminalProgramEdit, m_terminalArgsEdit, m_terminalWorkingDirEdit);
        gotTerm = true;
      } else if (!gotEdit && d.builtinKind == QLatin1String("editor")) {
        fillExtApp(d, m_editorProgramEdit, m_editorArgsEdit, m_editorWorkingDirEdit);
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
                                QLineEdit* program, QLineEdit* args,
                                QLineEdit* wd) {
    const QString matchedId = matchPresetId(presets,
                                            program->text().trimmed(),
                                            splitArgsTemplate(args->text()),
                                            wd->text().trimmed());
    int index = 0; // "(Custom)"
    for (int i = 1; i < combo->count(); ++i) {
      if (combo->itemData(i).toString() == matchedId) { index = i; break; }
    }
    QSignalBlocker blocker(combo);
    combo->setCurrentIndex(index);
  };
  selectMatchedPreset(m_terminalPresetCombo, m_terminalPresets,
                      m_terminalProgramEdit, m_terminalArgsEdit,
                      m_terminalWorkingDirEdit);
  selectMatchedPreset(m_editorPresetCombo, m_editorPresets,
                      m_editorProgramEdit, m_editorArgsEdit,
                      m_editorWorkingDirEdit);
}

void ExternalAppsTab::save() {
  auto& settings = Settings::instance();

  auto buildBuiltin = [](const QString& kind, const QString& name,
                         const QLineEdit* program, const QLineEdit* args,
                         const QLineEdit* wd) {
    UserCommand c;
    c.id                  = QStringLiteral("user.cmd.") + kind;
    c.name                = name;
    c.builtin             = true;
    c.builtinKind         = kind;
    c.showInToolsMenu     = true;
    c.program             = program->text().trimmed();
    c.argsTemplate        = splitArgsTemplate(args->text());
    c.workingDirTemplate  = wd->text().trimmed();
    return c;
  };

  QList<UserCommand> updated;
  updated.append(buildBuiltin(QStringLiteral("terminal"), tr("Terminal"),
                              m_terminalProgramEdit, m_terminalArgsEdit,
                              m_terminalWorkingDirEdit));
  updated.append(buildBuiltin(QStringLiteral("editor"), tr("Text Editor"),
                              m_editorProgramEdit, m_editorArgsEdit,
                              m_editorWorkingDirEdit));
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
                           QLineEdit* programEdit, QLineEdit* argsEdit,
                           QLineEdit* wdEdit) {
  UserCommand c;
  c.id                 = QStringLiteral("user.cmd.") + kind;
  c.name               = name;
  c.builtin            = true;
  c.builtinKind        = kind;
  c.program            = programEdit->text().trimmed();
  c.argsTemplate       = splitArgsTemplate(argsEdit->text());
  c.workingDirTemplate = wdEdit->text().trimmed();
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
    m_terminalProgramEdit, m_terminalArgsEdit, m_terminalWorkingDirEdit);
  QString err;
  if (!UserCommandManager::instance().runTransient(c, &err)) {
    QMessageBox::warning(this, tr("Test launch failed"), err);
  }
}

void ExternalAppsTab::onTestLaunchEditor() {
  const UserCommand c = buildTransient(
    QStringLiteral("editor"), tr("Text Editor"),
    m_editorProgramEdit, m_editorArgsEdit, m_editorWorkingDirEdit);
  QString err;
  if (!UserCommandManager::instance().runTransient(c, &err)) {
    QMessageBox::warning(this, tr("Test launch failed"), err);
  }
}

namespace {

// 共通: 選択された Preset のフィールドを LineEdit 群に流し込む。
// "(Custom)" (= userData 空) なら何もしない。setText() は textEdited を発火
// しないので、switch*ToCustom() に取られて Custom に戻る心配はない。
void applyPresetTo(QComboBox* combo,
                   const QList<AppPreset>& presets,
                   QLineEdit* program, QLineEdit* args, QLineEdit* wd) {
  const QString id = combo->currentData().toString();
  if (id.isEmpty()) return;  // (Custom)
  for (const AppPreset& p : presets) {
    if (p.id == id) {
      program->setText(p.program);
      args->setText(joinArgsTemplate(p.argsTemplate));
      wd->setText(p.workingDirTemplate);
      return;
    }
  }
}

} // anonymous namespace

void ExternalAppsTab::onTerminalPresetChanged(int /*index*/) {
  applyPresetTo(m_terminalPresetCombo, m_terminalPresets,
                m_terminalProgramEdit, m_terminalArgsEdit,
                m_terminalWorkingDirEdit);
}

void ExternalAppsTab::onEditorPresetChanged(int /*index*/) {
  applyPresetTo(m_editorPresetCombo, m_editorPresets,
                m_editorProgramEdit, m_editorArgsEdit,
                m_editorWorkingDirEdit);
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

} // namespace Farman

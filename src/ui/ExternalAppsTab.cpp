#include "ExternalAppsTab.h"
#include "settings/Settings.h"
#include "core/UserCommand.h"
#include "core/UserCommandManager.h"
#include "utils/Dialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
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
  // フェーズ 2 でユーザー定義コマンドの一覧 UI をこの下に追加する想定。
  auto buildExtAppBox = [this](
      const QString& title,
      QLineEdit*&    programEdit,
      QToolButton*&  programBrowse,
      QLineEdit*&    argsEdit,
      QLineEdit*&    workingDirEdit,
      QPushButton*&  testButton,
      const QString& argsPlaceholder) -> QGroupBox* {
    QGroupBox* box = new QGroupBox(title, this);
    QFormLayout* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
    m_terminalProgramEdit, m_terminalProgramBrowse,
    m_terminalArgsEdit, m_terminalWorkingDirEdit, m_terminalTestButton,
    tr("e.g. --working-directory={dir}")));

  mainLayout->addWidget(buildExtAppBox(
    tr("Text Editor"),
    m_editorProgramEdit, m_editorProgramBrowse,
    m_editorArgsEdit, m_editorWorkingDirEdit, m_editorTestButton,
    tr("e.g. {file}")));

  connect(m_terminalProgramBrowse, &QToolButton::clicked,
          this, &ExternalAppsTab::onBrowseTerminalProgram);
  connect(m_editorProgramBrowse, &QToolButton::clicked,
          this, &ExternalAppsTab::onBrowseEditorProgram);
  connect(m_terminalTestButton, &QPushButton::clicked,
          this, &ExternalAppsTab::onTestLaunchTerminal);
  connect(m_editorTestButton, &QPushButton::clicked,
          this, &ExternalAppsTab::onTestLaunchEditor);

  mainLayout->addStretch();
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

} // namespace Farman

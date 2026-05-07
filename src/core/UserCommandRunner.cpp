#include "UserCommandRunner.h"
#include "Logger.h"

#include <QProcess>
#include <QObject>

namespace Farman {

namespace {

// ログとユーザー向けメッセージで使う「コマンド + 引数」の表示。
// QStringList の各要素に空白が含まれる場合は引用符で囲んで読みやすくする。
QString prettyCommandLine(const QString& program, const QStringList& args) {
  QStringList parts;
  parts << program;
  for (const QString& a : args) {
    if (a.contains(QLatin1Char(' ')) || a.contains(QLatin1Char('\t')) || a.isEmpty()) {
      parts << (QLatin1Char('"') + a + QLatin1Char('"'));
    } else {
      parts << a;
    }
  }
  return parts.join(QLatin1Char(' '));
}

} // anonymous namespace

bool runUserCommand(const UserCommand& cmd,
                    const PaneContext& ctx,
                    QString*           errorOut) {
  // 1. 事前チェック
  if (cmd.program.trimmed().isEmpty()) {
    const QString msg = QObject::tr("External command \"%1\" has no program set.")
                         .arg(cmd.name);
    Logger::instance().warn(msg);
    if (errorOut) *errorOut = msg;
    return false;
  }

  // 2. 展開
  const QStringList args   = expandArgs(cmd.argsTemplate, ctx);
  const QString     cwd    = expandWorkingDir(cmd.workingDirTemplate, ctx);

  // 3. 起動 (デタッチ)。farman を閉じても外部アプリは生存する。
  const QString line = prettyCommandLine(cmd.program, args);
  qint64 pid = 0;
  const bool ok = QProcess::startDetached(cmd.program, args, cwd, &pid);

  if (ok) {
    Logger::instance().info(
      QObject::tr("Launched: %1 [%2] (pid=%3)").arg(cmd.name, line).arg(pid));
    return true;
  }

  // 4. 失敗
  const QString msg = QObject::tr("Failed to launch \"%1\". Command: %2")
                       .arg(cmd.name, line);
  Logger::instance().error(msg);
  if (errorOut) *errorOut = msg;
  return false;
}

} // namespace Farman

#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QTextStream>

namespace Farman {

namespace {
// 日次ログファイルの命名パターン: farman-YYYY-MM-DD.log
constexpr const char* kFilePrefix = "farman-";
constexpr const char* kFileSuffix = ".log";
}

Logger& Logger::instance() {
  static Logger inst;
  return inst;
}

Logger::Logger(QObject* parent)
  : QObject(parent)
{
}

Logger::~Logger() {
  QMutexLocker lock(&m_mutex);
  closeFileLocked();
}

void Logger::log(Level level, const QString& message) {
  const QDate today = QDate::currentDate();
  const QString formatted = format(level, message);
  {
    QMutexLocker lock(&m_mutex);
    m_buffer.append(formatted);
    while (m_buffer.size() > kMaxBuffer) m_buffer.removeFirst();

    if (m_fileEnabled) {
      // 日付が変わっていたらローテーション
      if (m_stream && m_currentDate != today) {
        closeFileLocked();
        purgeOldFilesLocked(today);
        openFileLocked(today);
      }
      if (m_stream) {
        *m_stream << formatted << '\n';
        m_stream->flush();
      }
    }
  }
  emit entryAppended(formatted);
}

QStringList Logger::recent() const {
  QMutexLocker lock(&m_mutex);
  return m_buffer;
}

void Logger::setFileOutput(bool enabled, const QString& directory, int retentionDays) {
  if (retentionDays < 0) retentionDays = 0;
  QMutexLocker lock(&m_mutex);
  // 状態が変わらなければ何もしない
  const bool sameAll = (enabled == m_fileEnabled)
                    && (directory == m_directory)
                    && (retentionDays == m_retentionDays)
                    && (!enabled || m_file);
  if (sameAll) return;

  closeFileLocked();
  m_fileEnabled   = enabled;
  m_directory     = directory;
  m_retentionDays = retentionDays;
  if (m_fileEnabled) {
    const QDate today = QDate::currentDate();
    purgeOldFilesLocked(today);
    openFileLocked(today);
  }
}

QString Logger::format(Level level, const QString& message) const {
  const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
  const char* lvl = (level == Warn) ? "WARN " : (level == Error) ? "ERROR" : "INFO ";
  return QStringLiteral("[%1] [%2] %3").arg(ts, QString::fromLatin1(lvl), message);
}

QString Logger::datedFilePath(const QDate& date) const {
  if (m_directory.isEmpty()) return QString();
  const QString stamp = date.toString(QStringLiteral("yyyy-MM-dd"));
  return QStringLiteral("%1/%2%3%4")
    .arg(m_directory, QString::fromLatin1(kFilePrefix), stamp, QString::fromLatin1(kFileSuffix));
}

void Logger::openFileLocked(const QDate& date) {
  if (m_directory.isEmpty()) {
    m_fileEnabled = false;
    return;
  }
  QDir().mkpath(m_directory);
  const QString path = datedFilePath(date);
  m_file = new QFile(path);
  if (!m_file->open(QIODevice::Append | QIODevice::Text)) {
    delete m_file;
    m_file = nullptr;
    m_fileEnabled = false;
    return;
  }
  m_stream = new QTextStream(m_file);
  m_stream->setEncoding(QStringConverter::Utf8);
  m_currentDate = date;
}

void Logger::closeFileLocked() {
  if (m_stream) { delete m_stream; m_stream = nullptr; }
  if (m_file)   { m_file->close(); delete m_file; m_file = nullptr; }
}

void Logger::purgeOldFilesLocked(const QDate& referenceDate) {
  if (m_retentionDays <= 0) return;  // 0 は永久保持
  if (m_directory.isEmpty()) return;

  // パターン: farman-YYYY-MM-DD.log
  const QRegularExpression re(
    QStringLiteral("^") + QRegularExpression::escape(QString::fromLatin1(kFilePrefix))
    + QStringLiteral("(\\d{4}-\\d{2}-\\d{2})")
    + QRegularExpression::escape(QString::fromLatin1(kFileSuffix))
    + QStringLiteral("$"));

  QDir d(m_directory);
  const QStringList entries = d.entryList(QDir::Files | QDir::NoSymLinks);
  for (const QString& name : entries) {
    auto m = re.match(name);
    if (!m.hasMatch()) continue;
    const QDate fileDate = QDate::fromString(m.captured(1), QStringLiteral("yyyy-MM-dd"));
    if (!fileDate.isValid()) continue;
    if (fileDate.daysTo(referenceDate) >= m_retentionDays) {
      QFile::remove(d.absoluteFilePath(name));
    }
  }
}

} // namespace Farman

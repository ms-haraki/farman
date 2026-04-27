#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QTextStream>

namespace Farman {

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

void Logger::setFileOutput(bool enabled, const QString& basePath, int retentionDays) {
  if (retentionDays < 0) retentionDays = 0;
  QMutexLocker lock(&m_mutex);
  // 状態が変わらなければ何もしない
  const bool sameAll = (enabled == m_fileEnabled)
                    && (basePath == m_basePath)
                    && (retentionDays == m_retentionDays)
                    && (!enabled || m_file);
  if (sameAll) return;

  closeFileLocked();
  m_fileEnabled   = enabled;
  m_basePath      = basePath;
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
  if (m_basePath.isEmpty()) return QString();
  QFileInfo fi(m_basePath);
  const QString dir    = fi.absolutePath();
  const QString base   = fi.completeBaseName();   // 拡張子を除く
  const QString suffix = fi.suffix();              // 拡張子 (なし可)
  const QString stamp  = date.toString(QStringLiteral("yyyy-MM-dd"));
  if (suffix.isEmpty()) {
    return QStringLiteral("%1/%2-%3").arg(dir, base, stamp);
  }
  return QStringLiteral("%1/%2-%3.%4").arg(dir, base, stamp, suffix);
}

void Logger::openFileLocked(const QDate& date) {
  if (m_basePath.isEmpty()) {
    m_fileEnabled = false;
    return;
  }
  const QString path = datedFilePath(date);
  QFileInfo fi(path);
  QDir().mkpath(fi.absolutePath());
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
  if (m_basePath.isEmpty()) return;

  QFileInfo fi(m_basePath);
  const QString dir    = fi.absolutePath();
  const QString base   = fi.completeBaseName();
  const QString suffix = fi.suffix();

  // パターン: {base}-YYYY-MM-DD(.{suffix})?
  const QString stampPattern = QStringLiteral("(\\d{4}-\\d{2}-\\d{2})");
  const QString suffixPart = suffix.isEmpty() ? QString() : QStringLiteral("\\.") + QRegularExpression::escape(suffix);
  const QRegularExpression re(QStringLiteral("^") + QRegularExpression::escape(base)
                              + QStringLiteral("-") + stampPattern + suffixPart + QStringLiteral("$"));

  QDir d(dir);
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

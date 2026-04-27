#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
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
  const QString formatted = format(level, message);
  {
    QMutexLocker lock(&m_mutex);
    m_buffer.append(formatted);
    while (m_buffer.size() > kMaxBuffer) m_buffer.removeFirst();
    if (m_fileEnabled && m_stream) {
      *m_stream << formatted << '\n';
      m_stream->flush();
    }
  }
  emit entryAppended(formatted);
}

QStringList Logger::recent() const {
  QMutexLocker lock(&m_mutex);
  return m_buffer;
}

void Logger::setFileOutput(bool enabled, const QString& filePath) {
  QMutexLocker lock(&m_mutex);
  // 状態が変わらなければ何もしない
  if (enabled == m_fileEnabled && filePath == m_filePath && (!enabled || m_file)) {
    return;
  }
  closeFileLocked();
  m_fileEnabled = enabled;
  m_filePath    = filePath;
  if (m_fileEnabled) {
    openFileLocked();
  }
}

QString Logger::format(Level level, const QString& message) const {
  const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
  const char* lvl = (level == Warn) ? "WARN " : (level == Error) ? "ERROR" : "INFO ";
  return QStringLiteral("[%1] [%2] %3").arg(ts, QString::fromLatin1(lvl), message);
}

void Logger::openFileLocked() {
  if (m_filePath.isEmpty()) {
    m_fileEnabled = false;
    return;
  }
  QFileInfo fi(m_filePath);
  QDir().mkpath(fi.absolutePath());
  m_file = new QFile(m_filePath);
  if (!m_file->open(QIODevice::Append | QIODevice::Text)) {
    delete m_file;
    m_file = nullptr;
    m_fileEnabled = false;
    return;
  }
  m_stream = new QTextStream(m_file);
  m_stream->setEncoding(QStringConverter::Utf8);
}

void Logger::closeFileLocked() {
  if (m_stream) { delete m_stream; m_stream = nullptr; }
  if (m_file)   { m_file->close(); delete m_file; m_file = nullptr; }
}

} // namespace Farman

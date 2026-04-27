#pragma once

#include <QMutex>
#include <QObject>
#include <QStringList>

class QFile;
class QTextStream;

namespace Farman {

// アプリケーション全体の操作ログを 1 か所に集約するシングルトン。
//   - メモリ上のリングバッファに直近 N 行を保持 (LogPane の初期表示用)
//   - ファイル出力 ON のときは追記モードで `m_filePath` に書き込む
//   - 新規エントリは `entryAppended` シグナルで購読側に通知される
class Logger : public QObject {
  Q_OBJECT

public:
  static Logger& instance();

  enum Level { Info, Warn, Error };

  void log(Level level, const QString& message);
  void info (const QString& m) { log(Info,  m); }
  void warn (const QString& m) { log(Warn,  m); }
  void error(const QString& m) { log(Error, m); }

  // 直近のログ行 (整形済み文字列)。LogPane 構築時に過去履歴を流し込む用。
  QStringList recent() const;

  // ファイル出力 ON/OFF とパスを設定。Settings 反映時に呼ぶ。
  void setFileOutput(bool enabled, const QString& filePath);

signals:
  void entryAppended(const QString& formatted);

private:
  explicit Logger(QObject* parent = nullptr);
  ~Logger() override;
  Q_DISABLE_COPY(Logger)

  QString format(Level level, const QString& message) const;
  void openFileLocked();   // m_mutex を保持した状態で呼ぶこと
  void closeFileLocked();

  static constexpr int kMaxBuffer = 1000;

  mutable QMutex m_mutex;
  QStringList    m_buffer;
  bool           m_fileEnabled = false;
  QString        m_filePath;
  QFile*         m_file   = nullptr;
  QTextStream*   m_stream = nullptr;
};

} // namespace Farman

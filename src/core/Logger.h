#pragma once

#include <QDate>
#include <QMutex>
#include <QObject>
#include <QStringList>

class QFile;
class QTextStream;

namespace Farman {

// アプリケーション全体の操作ログを 1 か所に集約するシングルトン。
//   - メモリ上のリングバッファに直近 N 行を保持 (LogPane の初期表示用)
//   - ファイル出力 ON のときは日付付きファイル (basePath を base にして
//     `{base}-YYYY-MM-DD.{suffix}`) に追記する。日が変わると自動で次のファイルへ。
//   - retentionDays > 0 なら、その日数より古い日付のファイルを起動時 / ローテーション時に削除。
//   - retentionDays == 0 は永久保持。
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

  // ファイル出力 ON/OFF とベースパス、保持日数を設定。Settings 反映時に呼ぶ。
  // retentionDays は 0 で永久保持、それ以外は日数で古いファイルを削除。
  void setFileOutput(bool enabled, const QString& basePath, int retentionDays);

signals:
  void entryAppended(const QString& formatted);

private:
  explicit Logger(QObject* parent = nullptr);
  ~Logger() override;
  Q_DISABLE_COPY(Logger)

  QString format(Level level, const QString& message) const;
  // basePath とその日付から実ファイルパスを組み立てる。例:
  //   "/.../farman.log" + 2026-04-27 -> "/.../farman-2026-04-27.log"
  QString datedFilePath(const QDate& date) const;
  void openFileLocked(const QDate& date);  // m_mutex を保持した状態で呼ぶこと
  void closeFileLocked();
  // 古いログファイルを削除。m_mutex を保持した状態で呼ぶこと。
  void purgeOldFilesLocked(const QDate& referenceDate);

  static constexpr int kMaxBuffer = 1000;

  mutable QMutex m_mutex;
  QStringList    m_buffer;
  bool           m_fileEnabled  = false;
  QString        m_basePath;
  int            m_retentionDays = 0;  // 0 = 永久
  QDate          m_currentDate;
  QFile*         m_file   = nullptr;
  QTextStream*   m_stream = nullptr;
};

} // namespace Farman

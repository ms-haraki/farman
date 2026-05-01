#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QStandardPaths>
#include <QTranslator>
#include "ui/MainWindow.h"
#include "viewer/ViewerDispatcher.h"
#include "settings/Settings.h"

namespace {
// 二重起動検出用のサーバー名。HOME や AppConfigLocation のハッシュを混ぜて
// 同一ユーザーの異なるセッション (例: SSH 経由の別ホスト) で衝突しないように
// する。マルチユーザー環境でも同じ socket 名を取り合わないように、
// ローカル限定の AppConfigLocation を使う。
QString singleInstanceServerName() {
  QString id = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (id.isEmpty()) id = QDir::homePath();
  return QStringLiteral("farman-instance-%1")
    .arg(QString::number(qHash(id), 16));
}
}  // namespace

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setOrganizationName("Farman");
  app.setApplicationName("farman");

  // ランタイム用アイコン (タスクバー / ウィンドウタイトル / Linux WM など)。
  // macOS の Dock とバンドル本体のアイコンは Info.plist + Contents/Resources の
  // icon.icns 側 (CMake で MACOSX_BUNDLE_ICON_FILE 指定) が使われる。
  app.setWindowIcon(QIcon(QStringLiteral(":/icons/farman.png")));

  // Settings をロードして UI 言語を決定する。
  // QTranslator は QApplication と同じ寿命にしたいので static にしておく。
  Farman::Settings::instance().load();

  // 二重起動チェック (Settings の singleInstance ON のとき)。
  // - 既存インスタンスがあれば QLocalSocket 経由で activate を投げて即終了。
  // - 無ければ QLocalServer を立ち上げて、後続の起動からの activate を受ける。
  static QLocalServer* singleInstanceServer = nullptr;
  if (Farman::Settings::instance().singleInstance()) {
    const QString serverName = singleInstanceServerName();
    {
      QLocalSocket probe;
      probe.connectToServer(serverName);
      if (probe.waitForConnected(300)) {
        // 既存インスタンス検出。activate 要求を送って自分は終了する。
        probe.write("activate\n");
        probe.flush();
        probe.waitForBytesWritten(300);
        probe.disconnectFromServer();
        return 0;
      }
    }
    // 自分が最初のインスタンス。残骸 socket を消してから listen 開始。
    QLocalServer::removeServer(serverName);
    singleInstanceServer = new QLocalServer(&app);
    if (!singleInstanceServer->listen(serverName)) {
      // listen 失敗してもアプリ自体は続行 (single-instance 機能を諦める)。
      delete singleInstanceServer;
      singleInstanceServer = nullptr;
    }
  }
  static QTranslator appTranslator;
  static QTranslator qtTranslator;

  QString lang;
  switch (Farman::Settings::instance().language()) {
    case Farman::LanguageMode::English:  lang = "en"; break;
    case Farman::LanguageMode::Japanese: lang = "ja"; break;
    case Farman::LanguageMode::Auto:     lang = QLocale::system().name(); break;
  }
  // ja_JP なども ja として扱う
  const QString shortLang = lang.section('_', 0, 0);

  // Qt 標準ダイアログ (OK/Cancel など) の翻訳
  if (qtTranslator.load("qt_" + shortLang,
                        QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
    app.installTranslator(&qtTranslator);
  }

  // farman 自身の翻訳。リソース埋め込み済み (qt_add_translations のデフォルト命名)
  if (appTranslator.load(QStringLiteral(":/translations/farman_") + shortLang)) {
    app.installTranslator(&appTranslator);
  }

  // Initialize built-in viewers
  Farman::ViewerDispatcher::instance().registerBuiltins();

  Farman::MainWindow window;
  window.show();

  // 後続インスタンスからの activate 要求でこのウィンドウを前面に出す。
  if (singleInstanceServer) {
    QObject::connect(singleInstanceServer, &QLocalServer::newConnection,
      [&window]() {
        QLocalSocket* client = singleInstanceServer->nextPendingConnection();
        if (!client) return;
        QObject::connect(client, &QLocalSocket::readyRead, &window,
          [client, &window]() {
            const QByteArray msg = client->readAll();
            if (msg.contains("activate")) {
              if (window.isMinimized()) window.showNormal();
              window.show();
              window.raise();
              window.activateWindow();
            }
          });
        QObject::connect(client, &QLocalSocket::disconnected,
                         client, &QObject::deleteLater);
      });
  }

  return app.exec();
}

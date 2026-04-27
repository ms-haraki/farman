#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include "ui/MainWindow.h"
#include "viewer/ViewerDispatcher.h"
#include "settings/Settings.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setOrganizationName("Farman");
  app.setApplicationName("farman");

  // Settings をロードして UI 言語を決定する。
  // QTranslator は QApplication と同じ寿命にしたいので static にしておく。
  Farman::Settings::instance().load();
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

  return app.exec();
}

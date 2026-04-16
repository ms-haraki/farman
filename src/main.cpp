#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QLabel label("farman 起動確認");
  label.show();
  return app.exec();
}

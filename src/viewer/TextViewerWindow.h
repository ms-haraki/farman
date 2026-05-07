#pragma once

#include <QMainWindow>

namespace Farman {

class TextView;

class TextViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit TextViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~TextViewerWindow() override = default;

protected:
  // External モード時に Esc で閉じてメインに戻れるよう、QMainWindow の
  // keyPressEvent を上書きする。
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUi();
  void loadFile();

  QString    m_filePath;
  TextView*  m_textView;
};

} // namespace Farman

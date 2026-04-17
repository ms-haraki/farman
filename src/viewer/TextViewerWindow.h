#pragma once

#include <QMainWindow>

class QTextEdit;

namespace Farman {

class TextViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit TextViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~TextViewerWindow() override = default;

private:
  void setupUi();
  void loadFile();

  QString m_filePath;
  QTextEdit* m_textEdit;
};

} // namespace Farman

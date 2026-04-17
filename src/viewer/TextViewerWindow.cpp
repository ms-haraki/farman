#include "TextViewerWindow.h"
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QFileInfo>

namespace Farman {

TextViewerWindow::TextViewerWindow(const QString& filePath, QWidget* parent)
  : QMainWindow(parent)
  , m_filePath(filePath)
  , m_textEdit(nullptr)
{
  setupUi();
  loadFile();
}

void TextViewerWindow::setupUi() {
  // Set window title with file name
  QFileInfo fileInfo(m_filePath);
  setWindowTitle(QString("Text Viewer - %1").arg(fileInfo.fileName()));

  // Create central widget with layout
  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  // Create text edit widget
  m_textEdit = new QTextEdit(this);
  m_textEdit->setReadOnly(true);
  m_textEdit->setLineWrapMode(QTextEdit::NoWrap);

  layout->addWidget(m_textEdit);
  setCentralWidget(centralWidget);

  // Set window size
  resize(800, 600);
}

void TextViewerWindow::loadFile() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error",
      QString("Failed to open file: %1").arg(m_filePath));
    return;
  }

  QTextStream in(&file);
  QString content = in.readAll();
  m_textEdit->setPlainText(content);
  file.close();

  // Move cursor to beginning
  QTextCursor cursor = m_textEdit->textCursor();
  cursor.movePosition(QTextCursor::Start);
  m_textEdit->setTextCursor(cursor);
}

} // namespace Farman

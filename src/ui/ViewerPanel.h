#pragma once

#include <QWidget>

class QTextEdit;
class QLabel;
class QScrollArea;
class QStackedWidget;

namespace Farman {

class BinaryView;

class ViewerPanel : public QWidget {
  Q_OBJECT

public:
  explicit ViewerPanel(QWidget* parent = nullptr);
  ~ViewerPanel() override;

  // ファイルを開く
  bool openFile(const QString& filePath);

  // 現在のファイルパス
  QString currentFilePath() const { return m_currentFilePath; }

  // ビューアをクリア
  void clear();

signals:
  void fileOpened(const QString& filePath);
  void fileClosed();

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void setupUi();
  bool openTextFile(const QString& filePath);
  bool openImageFile(const QString& filePath);
  bool openBinaryFile(const QString& filePath);
  void updateImageScale();

  QStackedWidget* m_stack;
  QTextEdit* m_textEdit;
  QScrollArea* m_imageScrollArea;
  QLabel* m_imageLabel;
  BinaryView* m_binaryView;

  QString m_currentFilePath;
  QPixmap m_originalPixmap;  // 元の画像サイズを保持
};

} // namespace Farman

#pragma once

#include <QString>
#include <QWidget>

class QStackedWidget;

namespace Farman {

class BinaryView;
class ImageView;
class TextView;

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

private:
  void setupUi();
  bool openTextFile(const QString& filePath);
  bool openImageFile(const QString& filePath);
  bool openBinaryFile(const QString& filePath);

  QStackedWidget* m_stack      = nullptr;
  TextView*       m_textView   = nullptr;
  ImageView*      m_imageView  = nullptr;
  BinaryView*     m_binaryView = nullptr;

  QString m_currentFilePath;
};

} // namespace Farman

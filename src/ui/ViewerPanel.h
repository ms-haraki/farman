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
  // どのビュアーで開くかを呼び出し側から強制したい場合に使う。
  // Auto は拡張子・MIME ルーティングに従う通常動作。
  enum class ViewerKind { Auto, Text, Image, Binary };

  explicit ViewerPanel(QWidget* parent = nullptr);
  ~ViewerPanel() override;

  // ファイルを開く（kind=Auto のとき Settings の対応表に従って自動判定）
  bool openFile(const QString& filePath, ViewerKind kind = ViewerKind::Auto);

  // 現在のファイルパス
  QString currentFilePath() const { return m_currentFilePath; }

  // ビューアをクリア
  void clear();

signals:
  void fileOpened(const QString& filePath);
  void fileClosed();
  // ステータスバー連携用: 表示中ファイルのパスと、ビュアー固有の要約。
  // 何も表示していなければ両方空文字列。
  void viewerStatusChanged(const QString& path, const QString& summary);

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

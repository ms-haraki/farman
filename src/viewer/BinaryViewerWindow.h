#pragma once

#include <QMainWindow>

namespace Farman {

class BinaryView;

class BinaryViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit BinaryViewerWindow(const QString& filePath, QWidget* parent = nullptr);
  ~BinaryViewerWindow() override = default;

protected:
  // Esc でウィンドウを閉じる。
  void keyPressEvent(QKeyEvent* event) override;

private:
  QString     m_filePath;
  BinaryView* m_view;
};

} // namespace Farman

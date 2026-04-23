#pragma once

#include <QLabel>

namespace Farman {

// マウスクリックでシグナルを発行する軽量な QLabel 派生。
// QToolButton だと macOS のネイティブスタイルが効いて隣接 QLabel と高さが
// 揃わないため、見栄えを合わせたいクリック可能表示に使う。
class ClickableLabel : public QLabel {
  Q_OBJECT

public:
  using QLabel::QLabel;

signals:
  void clicked();

protected:
  void mousePressEvent(QMouseEvent* event) override;
};

} // namespace Farman

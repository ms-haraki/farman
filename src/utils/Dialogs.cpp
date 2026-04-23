#include "Dialogs.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>

namespace Farman {

namespace {

// QMessageBox は内部で独自のイベント処理を行っていて keyPressEvent の
// override や setShortcut が届かないことがあった。確実に Y/N キーを拾う
// ため、QDialog をそのまま継承して自前の UI/key handling で実装する。
class ConfirmDialog : public QDialog {
public:
  ConfirmDialog(QWidget* parent,
                const QString& title,
                const QString& text,
                bool defaultYes)
      : QDialog(parent) {
    setWindowTitle(title);
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(text, this);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* noBtn  = new QPushButton(tr("No"),  this);
    auto* yesBtn = new QPushButton(tr("Yes"), this);
    btnLayout->addWidget(noBtn);
    btnLayout->addWidget(yesBtn);
    layout->addLayout(btnLayout);

    connect(yesBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(noBtn,  &QPushButton::clicked, this, &QDialog::reject);

    (defaultYes ? yesBtn : noBtn)->setDefault(true);
  }

protected:
  void keyPressEvent(QKeyEvent* event) override {
    const auto mods = event->modifiers();
    if (mods == Qt::NoModifier || mods == Qt::KeypadModifier) {
      if (event->key() == Qt::Key_Y) { accept(); return; }
      if (event->key() == Qt::Key_N) { reject(); return; }
    }
    QDialog::keyPressEvent(event);
  }
};

} // anonymous namespace

bool confirm(QWidget* parent,
             const QString& title,
             const QString& text,
             bool defaultYes) {
  ConfirmDialog dlg(parent, title, text, defaultYes);
  return dlg.exec() == QDialog::Accepted;
}

} // namespace Farman

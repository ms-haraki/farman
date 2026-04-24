#include "Dialogs.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

namespace {

// QMessageBox では keyPressEvent override が届かないため、QDialog を直接
// 継承し、さらに子ボタンにフォーカスがあっても Y/N を拾えるよう eventFilter
// も仕込む。Tab によるボタン間フォーカス移動も明示的に有効化する。
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

    // Alt+Y / Alt+N のショートカットとラベル表記、Tab フォーカスを設定
    applyAltShortcut(yesBtn, Qt::Key_Y);
    applyAltShortcut(noBtn,  Qt::Key_N);
    setTabOrder(noBtn, yesBtn);

    (defaultYes ? yesBtn : noBtn)->setDefault(true);
    (defaultYes ? yesBtn : noBtn)->setFocus();

    // 子ボタンにフォーカスがある時でも単押しの Y/N を拾えるようフィルタ経由でも処理。
    yesBtn->installEventFilter(this);
    noBtn->installEventFilter(this);
  }

protected:
  bool eventFilter(QObject* obj, QEvent* event) override {
    if (event->type() == QEvent::KeyPress) {
      auto* ke = static_cast<QKeyEvent*>(event);
      if (handleShortcut(ke)) return true;
    }
    return QDialog::eventFilter(obj, event);
  }

  void keyPressEvent(QKeyEvent* event) override {
    if (handleShortcut(event)) return;
    QDialog::keyPressEvent(event);
  }

private:
  bool handleShortcut(QKeyEvent* event) {
    const auto mods = event->modifiers();
    if (mods != Qt::NoModifier && mods != Qt::KeypadModifier) return false;
    if (event->key() == Qt::Key_Y) { accept(); return true; }
    if (event->key() == Qt::Key_N) { reject(); return true; }
    return false;
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

void applyAltShortcut(QPushButton* btn, Qt::Key key) {
  if (!btn) return;
  const QKeySequence seq(Qt::ALT | key);
  const QString hint = QStringLiteral(" (%1)").arg(seq.toString(QKeySequence::NativeText));
  QString text = btn->text();
  if (!text.endsWith(hint)) {
    // & は mnemonic 用の予約文字。macOS では表示されない上、重複付加の誤検知の
    // 元になるため一旦除去する。
    text.remove(QLatin1Char('&'));
    btn->setText(text + hint);
  }
  btn->setShortcut(seq);
  btn->setFocusPolicy(Qt::StrongFocus);
}

} // namespace Farman

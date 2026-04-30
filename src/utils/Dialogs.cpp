#include "Dialogs.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

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

QString inputText(QWidget* parent,
                  const QString& title,
                  const QString& label,
                  const QString& defaultValue,
                  bool* ok,
                  TextInputCursor cursor) {
  QDialog dlg(parent);
  dlg.setWindowTitle(title);
  dlg.setModal(true);
  dlg.resize(480, 0);

  auto* layout = new QVBoxLayout(&dlg);

  auto* labelWidget = new QLabel(label, &dlg);
  labelWidget->setWordWrap(true);
  layout->addWidget(labelWidget);

  auto* edit = new QLineEdit(defaultValue, &dlg);
  edit->setFocusPolicy(Qt::StrongFocus);
  if (cursor == TextInputCursor::BeforeExtension) {
    // 拡張子の手前にカーソル。先頭 '.' (ドットファイル) はスキップ。
    // ダイアログ show() 直後に Qt が selectAll を再適用してくることが
    // あるので、QTimer で次のイベントループまで遅延してから上書きする。
    int extPos = -1;
    for (int i = 1; i < defaultValue.length(); ++i) {
      if (defaultValue.at(i) == QLatin1Char('.')) { extPos = i; break; }
    }
    const int pos = (extPos > 0) ? extPos : defaultValue.length();
    QTimer::singleShot(0, edit, [edit, pos]() {
      edit->setFocus();
      edit->deselect();
      edit->setCursorPosition(pos);
    });
  } else {
    edit->selectAll();
  }
  layout->addWidget(edit);

  // 他のダイアログ（ConfirmDialog など）と同じ見た目にするため、
  // QDialogButtonBox ではなく自前の QPushButton を右寄せで配置する。
  auto* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  auto* cancelBtn = new QPushButton(QObject::tr("Cancel"), &dlg);
  auto* okBtn     = new QPushButton(QObject::tr("OK"),     &dlg);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  applyAltShortcut(okBtn,     Qt::Key_O);
  okBtn->setDefault(true);
  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(okBtn);
  layout->addLayout(btnLayout);
  QObject::connect(okBtn,     &QPushButton::clicked, &dlg, &QDialog::accept);
  QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

  // Tab: 入力欄 → Cancel → OK（実行系が最後）
  QWidget::setTabOrder(edit,      cancelBtn);
  QWidget::setTabOrder(cancelBtn, okBtn);

  edit->setFocus();

  const bool accepted = dlg.exec() == QDialog::Accepted;
  if (ok) *ok = accepted;
  return accepted ? edit->text() : QString();
}

} // namespace Farman

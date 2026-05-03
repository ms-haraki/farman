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

QString withAltMnemonic(const QString& text, Qt::Key key) {
  // & は mnemonic 用の予約文字。再呼び出しに備えて一旦除去する。
  QString out = text;
  out.remove(QLatin1Char('&'));

  const QKeySequence seq(Qt::ALT | key);

#ifdef Q_OS_MAC
  // macOS は & mnemonic を表示しない (Apple HIG)。末尾に "(⌥X)" を添えて
  // ショートカットを視覚的に伝える。既に末尾に hint があれば追加しない。
  const QString hint = QStringLiteral(" (%1)").arg(seq.toString(QKeySequence::NativeText));
  if (out.endsWith(hint)) return out;
  return out + hint;
#else
  // Windows / Linux は & mnemonic がネイティブ流儀。Alt 押下時に該当文字が
  // アンダーラインで描画される。QPushButton / QRadioButton / QCheckBox は
  // この & 表記から Alt+key で自動的にアクティベートする。
  //   - 英語: "Copy" + Qt::Key_C → "&Copy"  (C にアンダーライン)
  //   - 日本語: "コピー" + Qt::Key_C → 該当字無いので "コピー (&C)" と末尾追加
  //     これは Windows の和文ボタン慣習 (例: "OK(&O)") に揃える形。
  const QChar target = QChar(static_cast<int>(key)).toLower();
  for (int i = 0; i < out.length(); ++i) {
    if (out[i].toLower() == target) {
      out.insert(i, QLatin1Char('&'));
      return out;
    }
  }
  // 該当字なし: "(&F)" 末尾追加
  const QChar upper = QChar(static_cast<int>(key));
  return out + QStringLiteral(" (&%1)").arg(upper);
#endif
}

void applyAltShortcut(QPushButton* btn, Qt::Key key) {
  if (!btn) return;
  btn->setText(withAltMnemonic(btn->text(), key));
  btn->setShortcut(QKeySequence(Qt::ALT | key));
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

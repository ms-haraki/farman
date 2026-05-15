#include "Dialogs.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
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

namespace {

// 情報 / 警告 / エラーの OK 単独ダイアログの実体。
// 標準 QMessageBox は macOS のキーボードナビゲーション設定が OFF だと
// Tab フォーカスや Alt+key が効かないので、ConfirmDialog と同じパターンで
// 自前実装する。
class MessageDialog : public QDialog {
public:
  enum Icon { Information, Warning, Critical };

  MessageDialog(QWidget* parent, const QString& title,
                const QString& text, Icon icon)
      : QDialog(parent) {
    setWindowTitle(title);
    setModal(true);
    resize(480, 0);

    auto* mainLayout = new QVBoxLayout(this);

    auto* row = new QHBoxLayout();
    row->setSpacing(12);

    QStyle::StandardPixmap sp = QStyle::SP_MessageBoxInformation;
    switch (icon) {
      case Information: sp = QStyle::SP_MessageBoxInformation; break;
      case Warning:     sp = QStyle::SP_MessageBoxWarning;     break;
      case Critical:    sp = QStyle::SP_MessageBoxCritical;    break;
    }
    auto* iconLabel = new QLabel(this);
    const int sz = style()->pixelMetric(QStyle::PM_LargeIconSize);
    iconLabel->setPixmap(style()->standardIcon(sp).pixmap(sz, sz));
    iconLabel->setAlignment(Qt::AlignTop);
    iconLabel->setFixedWidth(sz);
    row->addWidget(iconLabel, 0, Qt::AlignTop);

    auto* msgLabel = new QLabel(text, this);
    msgLabel->setWordWrap(true);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgLabel->setAlignment(Qt::AlignTop);
    row->addWidget(msgLabel, 1);
    mainLayout->addLayout(row);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* okBtn = new QPushButton(tr("OK"), this);
    applyAltShortcut(okBtn, Qt::Key_O);
    okBtn->setDefault(true);
    okBtn->setAutoDefault(true);
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

    okBtn->setFocus();
  }

protected:
  void keyPressEvent(QKeyEvent* event) override {
    // Esc / Enter / Return / Space すべて OK 扱いで閉じる。
    const int k = event->key();
    if (k == Qt::Key_Escape || k == Qt::Key_Return || k == Qt::Key_Enter) {
      accept();
      return;
    }
    QDialog::keyPressEvent(event);
  }
};

void showMessage(QWidget* parent, const QString& title,
                 const QString& text, MessageDialog::Icon icon) {
  MessageDialog dlg(parent, title, text, icon);
  dlg.exec();
}

} // anonymous namespace

void inform(QWidget* parent, const QString& title, const QString& text) {
  showMessage(parent, title, text, MessageDialog::Information);
}

void warn(QWidget* parent, const QString& title, const QString& text) {
  showMessage(parent, title, text, MessageDialog::Warning);
}

void critical(QWidget* parent, const QString& title, const QString& text) {
  showMessage(parent, title, text, MessageDialog::Critical);
}

namespace {

// 任意ボタンの選択ダイアログ。MessageDialog と同じレイアウトに、右下に
// 任意個のボタンを並べる。各ボタンに Alt+key (任意) を applyAltShortcut で
// 設定し、ボタンクリック / Enter (default) / Esc (cancelIndex) を index に
// 変換して accept() する。返り値は QDialog::exec() の戻りではなく、
// メンバ m_result に格納した「押されたボタンの index」を使う。
class ChoiceDialog : public QDialog {
public:
  ChoiceDialog(QWidget* parent,
               const QString& title,
               const QString& text,
               const QList<DialogButton>& buttons,
               int defaultIndex,
               int cancelIndex,
               DialogIcon icon)
      : QDialog(parent)
      , m_result(cancelIndex)
      , m_cancelIndex(cancelIndex) {
    setWindowTitle(title);
    setModal(true);
    resize(480, 0);

    auto* mainLayout = new QVBoxLayout(this);

    auto* row = new QHBoxLayout();
    row->setSpacing(12);

    if (icon != DialogIcon::None) {
      QStyle::StandardPixmap sp = QStyle::SP_MessageBoxInformation;
      switch (icon) {
        case DialogIcon::Information: sp = QStyle::SP_MessageBoxInformation; break;
        case DialogIcon::Question:    sp = QStyle::SP_MessageBoxQuestion;    break;
        case DialogIcon::Warning:     sp = QStyle::SP_MessageBoxWarning;     break;
        case DialogIcon::Critical:    sp = QStyle::SP_MessageBoxCritical;    break;
        case DialogIcon::None:        break;
      }
      auto* iconLabel = new QLabel(this);
      const int sz = style()->pixelMetric(QStyle::PM_LargeIconSize);
      iconLabel->setPixmap(style()->standardIcon(sp).pixmap(sz, sz));
      iconLabel->setAlignment(Qt::AlignTop);
      iconLabel->setFixedWidth(sz);
      row->addWidget(iconLabel, 0, Qt::AlignTop);
    }

    auto* msgLabel = new QLabel(text, this);
    msgLabel->setWordWrap(true);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgLabel->setAlignment(Qt::AlignTop);
    row->addWidget(msgLabel, 1);
    mainLayout->addLayout(row);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    QList<QPushButton*> btns;
    for (int i = 0; i < buttons.size(); ++i) {
      const auto& b = buttons.at(i);
      auto* btn = new QPushButton(b.label, this);
      btn->setFocusPolicy(Qt::StrongFocus);
      if (b.altKey != Qt::Key(0)) {
        applyAltShortcut(btn, b.altKey);
      }
      connect(btn, &QPushButton::clicked, this, [this, i]() {
        m_result = i;
        accept();
      });
      btnLayout->addWidget(btn);
      btns.append(btn);
    }
    mainLayout->addLayout(btnLayout);

    // Default ボタン (Enter で発火) + 初期フォーカス
    if (defaultIndex >= 0 && defaultIndex < btns.size()) {
      btns[defaultIndex]->setDefault(true);
      btns[defaultIndex]->setAutoDefault(true);
      btns[defaultIndex]->setFocus();
    }
    // Tab 順 (左 → 右)
    for (int i = 0; i + 1 < btns.size(); ++i) {
      setTabOrder(btns[i], btns[i + 1]);
    }
  }

  int result() const { return m_result; }

protected:
  void keyPressEvent(QKeyEvent* event) override {
    if (event->key() == Qt::Key_Escape) {
      m_result = m_cancelIndex;
      reject();
      return;
    }
    QDialog::keyPressEvent(event);
  }

private:
  int m_result;
  int m_cancelIndex;
};

} // anonymous namespace

int choose(QWidget* parent,
           const QString& title,
           const QString& text,
           const QList<DialogButton>& buttons,
           int defaultIndex,
           int cancelIndex,
           DialogIcon icon) {
  ChoiceDialog dlg(parent, title, text, buttons,
                   defaultIndex, cancelIndex, icon);
  dlg.exec();
  return dlg.result();
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

bool informWithSuppress(QWidget* parent,
                        const QString& title,
                        const QString& message,
                        bool initiallyShow) {
  QDialog dlg(parent);
  dlg.setWindowTitle(title);
  dlg.setModal(true);
  dlg.resize(480, 0);

  auto* layout = new QVBoxLayout(&dlg);

  auto* msg = new QLabel(message, &dlg);
  msg->setWordWrap(true);
  msg->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(msg);

  // 「次回以降表示しない」(チェック ON で「次回以降表示しない」)
  // initiallyShow=true (= 通常呼び出し: 設定上はまだ表示する) → チェック OFF
  // initiallyShow=false (= 既に「表示しない」設定だが手動で再表示している場合) →
  //   チェック ON で開いて、ユーザーが解除すれば「再び表示する」に戻せる。
  auto* suppressCheck = new QCheckBox(
    QObject::tr("Do not show this dialog next time"), &dlg);
  suppressCheck->setChecked(!initiallyShow);
  layout->addWidget(suppressCheck);

  auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
  auto* okBtn = btnBox->button(QDialogButtonBox::Ok);
  applyAltShortcut(okBtn, Qt::Key_O);
  okBtn->setDefault(true);
  layout->addWidget(btnBox);

  QObject::connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

  dlg.exec();

  // 戻り値は「次回以降も表示するか」: チェックが OFF なら true (= 表示し続ける)
  return !suppressCheck->isChecked();
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
    // ベース名を選択しつつカーソルを拡張子の手前に置く。
    // 先頭 '.' (ドットファイル) は拡張子としてではなくベース名の一部とみなす。
    //   "foo.txt"      → "foo" を選択、カーソルは '.' の直前
    //   "foo.tar.gz"   → "foo" を選択 (最初の '.' の直前)
    //   ".gitignore"   → 全選択 (拡張子とみなさない)
    //   "Makefile"     → 全選択 ('.' なし)
    // ダイアログ show() 直後に Qt が selectAll を再適用してくることが
    // あるので、QTimer で次のイベントループまで遅延してから上書きする。
    int extPos = -1;
    for (int i = 1; i < defaultValue.length(); ++i) {
      if (defaultValue.at(i) == QLatin1Char('.')) { extPos = i; break; }
    }
    const int pos = (extPos > 0) ? extPos : defaultValue.length();
    QTimer::singleShot(0, edit, [edit, pos]() {
      edit->setFocus();
      // setSelection(start, length) は選択範囲を設定すると同時に
      // カーソルを選択末尾 (= pos) に移動するので、別途 setCursorPosition
      // を呼ぶ必要はない。
      edit->setSelection(0, pos);
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

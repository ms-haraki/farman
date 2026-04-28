#include "SettingsDialog.h"
#include "KeybindingTab.h"
#include "AppearanceTab.h"
#include "BehaviorTab.h"
#include "GeneralTab.h"
#include "ViewersTab.h"
#include "settings/Settings.h"
#include "keybinding/KeyBindingManager.h"
#include "utils/Dialogs.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <cstdlib>

namespace Farman {

SettingsDialog::SettingsDialog(const QString& leftCurrentPath,
                               const QString& rightCurrentPath,
                               QWidget* parent)
  : QDialog(parent)
  , m_tabWidget(nullptr)
  , m_keybindingTab(nullptr)
  , m_appearanceTab(nullptr)
  , m_behaviorTab(nullptr)
  , m_viewersTab(nullptr)
  , m_buttonBox(nullptr)
  , m_leftCurrentPath(leftCurrentPath)
  , m_rightCurrentPath(rightCurrentPath) {
  setupUi();
}

void SettingsDialog::setupUi() {
  setWindowTitle(tr("Settings"));
  resize(800, 600);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Tab widget
  m_tabWidget = new QTabWidget(this);

  m_keybindingTab = new KeybindingTab(this);
  m_appearanceTab = new AppearanceTab(this);
  m_behaviorTab   = new BehaviorTab(this);
  m_generalTab    = new GeneralTab(m_leftCurrentPath, m_rightCurrentPath, this);
  m_viewersTab    = new ViewersTab(this);

  m_tabWidget->addTab(m_generalTab,    tr("1. General"));
  m_tabWidget->addTab(m_behaviorTab,   tr("2. Behavior"));
  m_tabWidget->addTab(m_appearanceTab, tr("3. Appearance"));
  m_tabWidget->addTab(m_viewersTab,    tr("4. Viewers"));
  m_tabWidget->addTab(m_keybindingTab, tr("5. Keybindings"));

  // macOS の System Settings → 「キーボード」→「キーボードナビゲーション」に
  // 依存させたくないので、各タブ内の操作対象ウィジェットに対して明示的に
  // StrongFocus を設定する。Tab キーで全項目を辿れるようにするのが目的。
  const QList<QWidget*> tabRoots = {
    m_generalTab, m_behaviorTab, m_appearanceTab, m_viewersTab, m_keybindingTab
  };
  for (QWidget* root : tabRoots) {
    const auto widgets = root->findChildren<QWidget*>();
    for (QWidget* w : widgets) {
      if (qobject_cast<QCheckBox*>(w)   || qobject_cast<QComboBox*>(w) ||
          qobject_cast<QSpinBox*>(w)    || qobject_cast<QLineEdit*>(w) ||
          qobject_cast<QPushButton*>(w) || qobject_cast<QToolButton*>(w)) {
        w->setFocusPolicy(Qt::StrongFocus);
      }
    }
  }

  // Info label for keyboard shortcuts
  int tabCount = m_tabWidget->count();
  QLabel* infoLabel = new QLabel(tr("Use Alt+number to switch tabs, or Ctrl+Tab/Ctrl+Shift+Tab"), this);
  QFont infoFont = infoLabel->font();
  infoFont.setPointSize(infoFont.pointSize() - 1);
  infoLabel->setFont(infoFont);
  infoLabel->setStyleSheet("color: palette(mid); padding: 4px;");
  mainLayout->addWidget(infoLabel);

  mainLayout->addWidget(m_tabWidget);

  // Setup keyboard shortcuts for tab switching
  // Alt+1 through Alt+9 for direct tab access (up to 9 tabs)
  const QList<Qt::Key> numberKeys = {
    Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5,
    Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9
  };

  for (int i = 0; i < qMin(tabCount, numberKeys.size()); ++i) {
    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::ALT | numberKeys[i]), this);
    // Capture i by value to avoid lambda capture issues
    connect(shortcut, &QShortcut::activated, [this, i]() {
      m_tabWidget->setCurrentIndex(i);
    });
  }

  // Ctrl+Tab for next tab
  QShortcut* shortcutNext = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
  connect(shortcutNext, &QShortcut::activated, [this]() {
    int currentIndex = m_tabWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % m_tabWidget->count();
    m_tabWidget->setCurrentIndex(nextIndex);
  });

  // Ctrl+Shift+Tab for previous tab
  QShortcut* shortcutPrev = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
  connect(shortcutPrev, &QShortcut::activated, [this]() {
    int currentIndex = m_tabWidget->currentIndex();
    int prevIndex = (currentIndex - 1 + m_tabWidget->count()) % m_tabWidget->count();
    m_tabWidget->setCurrentIndex(prevIndex);
  });

  // OK/Cancel/Apply buttons + 全設定リセット (キーバインドを除く)
  m_buttonBox = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply
      | QDialogButtonBox::RestoreDefaults,
    this
  );
  // RestoreDefaults を「全設定リセット」用に流用。Keybindings タブ専用の
  // Reset to Defaults は KeybindingTab 内のボタンで継続。
  if (auto* resetBtn = m_buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
    resetBtn->setText(tr("Reset All Settings..."));
    resetBtn->setToolTip(
      tr("Reset every setting (except keybindings) to its default value."));
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetAllSettings);
  }
  auto* okBtn     = m_buttonBox->button(QDialogButtonBox::Ok);
  auto* cancelBtn = m_buttonBox->button(QDialogButtonBox::Cancel);
  auto* applyBtn  = m_buttonBox->button(QDialogButtonBox::Apply);
  applyAltShortcut(okBtn,     Qt::Key_O);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  applyAltShortcut(applyBtn,  Qt::Key_A);
  // 誤操作防止のため Cancel → Apply → OK の順で Tab が回るようにする
  setTabOrder(cancelBtn, applyBtn);
  setTabOrder(applyBtn,  okBtn);

  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onOk);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);

  mainLayout->addWidget(m_buttonBox);

  // Keyboard shortcuts for Keybindings tab
  m_clearShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
  connect(m_clearShortcut, &QShortcut::activated, this, &SettingsDialog::onClearBinding);

  m_resetShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
  connect(m_resetShortcut, &QShortcut::activated, this, &SettingsDialog::onResetToDefaults);
}

void SettingsDialog::onOk() {
  onApply();
  accept();
}

void SettingsDialog::onApply() {
  // Save all tabs
  m_keybindingTab->save();
  m_appearanceTab->save();
  m_behaviorTab->save();
  m_generalTab->save();
  m_viewersTab->save();

  // Save settings to file
  Settings::instance().save();

  // Save keybindings to settings
  KeyBindingManager::instance().saveToSettings();

  // Notify that settings have changed
  emit settingsChanged();

  // 言語が変わっていたら、現プロセスでは適切に切り替えられないので
  // 再起動を促す。Yes なら新プロセスを起動してから旧プロセスを即終了。
  // Y/N の単押し対応のため独自の confirm() ヘルパを使う。
  if (m_generalTab->languageChangedOnSave()) {
    if (confirm(this,
                tr("Language Changed"),
                tr("Restart farman now to apply the new language?"),
                /*defaultYes=*/true)) {
#ifdef Q_OS_MACOS
      // applicationFilePath() は <bundle>.app/Contents/MacOS/<exe> を返す。
      // 裸の exe を直接起動すると LaunchServices が別アプリ扱いし、Dock に
      // も別エントリで現れるので .app の根を open(1) -n で起動する。
      QString bundlePath = QApplication::applicationFilePath();
      const int idx = bundlePath.indexOf(QStringLiteral("/Contents/MacOS/"));
      if (idx > 0) bundlePath.truncate(idx);
      QProcess::startDetached(QStringLiteral("/usr/bin/open"),
                              QStringList{QStringLiteral("-n"), bundlePath});
#else
      QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
#endif
      // 新プロセスを起動した後、旧プロセスを「確実に」終了させる。
      // QApplication::quit() はモーダルダイアログの内側から呼ぶと外側の
      // event loop が終了せず、旧プロセスが残ったまま新プロセスが立ち上がる
      // ことがある。Settings は既に save() 済みなので、Qt のクリーンアップを
      // スキップして即終了する。
      std::_Exit(0);
    }
  }
}

void SettingsDialog::onClearBinding() {
  qDebug() << "SettingsDialog::onClearBinding()";
  // Only work if Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    m_keybindingTab->clearCurrentBinding();
  }
}

void SettingsDialog::onResetAllSettings() {
  if (!confirm(this, tr("Reset All Settings"),
               tr("Reset all settings to their default values?\n"
                  "Keybindings are not affected."),
               /*defaultYes=*/false)) {
    return;
  }
  // メモリ上のフィールドをすべてデフォルトに戻し、ファイルへも保存。
  // 各タブを丸ごと再構築するのは大変なので、ダイアログを閉じて呼び出し側に
  // 「設定が変わった」を通知する。ユーザーが Settings をもう一度開けば
  // フレッシュなデフォルト値が反映される。
  Settings::instance().resetToDefaults();
  Settings::instance().save();
  emit settingsChanged();
  accept();
}

void SettingsDialog::onResetToDefaults() {
  qDebug() << "SettingsDialog::onResetToDefaults()";
  // Only work if Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    m_keybindingTab->resetToDefaults();
  }
}

void SettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Only process shortcuts when Keybindings tab is active
  if (m_tabWidget->currentWidget() == m_keybindingTab) {
    // Ctrl+D (Cmd+D on macOS) for Clear Binding
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D) {
      qDebug() << "Ctrl+D pressed via keyPressEvent";
      onClearBinding();
      event->accept();
      return;
    }
    // Ctrl+R (Cmd+R on macOS) for Reset to Defaults
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_R) {
      qDebug() << "Ctrl+R pressed via keyPressEvent";
      onResetToDefaults();
      event->accept();
      return;
    }
  }

  // Pass unhandled events to parent class
  QDialog::keyPressEvent(event);
}

} // namespace Farman

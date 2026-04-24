#include "TransferConfirmDialog.h"
#include "settings/Settings.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

TransferConfirmDialog::TransferConfirmDialog(Operation op,
                                             const QString&     sourceDir,
                                             const QStringList& itemPaths,
                                             const QString&     destDir,
                                             QWidget* parent)
  : QDialog(parent)
  , m_overwriteModeCombo(nullptr)
  , m_autoRenameEdit(nullptr)
  , m_buttonBox(nullptr) {
  setupUi(op, sourceDir, itemPaths, destDir);
}

void TransferConfirmDialog::setupUi(Operation op,
                                    const QString&     sourceDir,
                                    const QStringList& itemPaths,
                                    const QString&     destDir) {
  setWindowTitle(op == Copy ? tr("Confirm Copy") : tr("Confirm Move"));
  resize(560, 420);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Source / destination paths
  QFormLayout* pathForm = new QFormLayout();
  pathForm->addRow(tr("Source:"), new QLabel(sourceDir, this));
  pathForm->addRow(tr("Destination:"), new QLabel(destDir, this));
  mainLayout->addLayout(pathForm);

  // Items list
  QGroupBox* itemsGroup = new QGroupBox(
    tr("Items (%1)").arg(itemPaths.size()), this);
  QVBoxLayout* itemsLayout = new QVBoxLayout(itemsGroup);
  QListWidget* list = new QListWidget(this);
  for (const QString& path : itemPaths) {
    QFileInfo info(path);
    QString label = info.fileName();
    if (info.isDir()) label += "/";
    list->addItem(label);
  }
  itemsLayout->addWidget(list);
  mainLayout->addWidget(itemsGroup, 1);

  // Overwrite mode。ラベルに Alt+キーをネイティブ表記で明記する。
  // macOS では「⌥O」、Windows / Linux では「Alt+O」のような表示になる。
  const QString altO =
    QKeySequence(Qt::ALT | Qt::Key_O).toString(QKeySequence::NativeText);
  const QString altS =
    QKeySequence(Qt::ALT | Qt::Key_S).toString(QKeySequence::NativeText);

  QFormLayout* overwriteForm = new QFormLayout();
  m_overwriteModeCombo = new QComboBox(this);
  m_overwriteModeCombo->addItem(tr("Ask"),            static_cast<int>(OverwriteMode::Ask));
  m_overwriteModeCombo->addItem(tr("Auto-overwrite"), static_cast<int>(OverwriteMode::AutoOverwrite));
  m_overwriteModeCombo->addItem(tr("Auto-rename"),    static_cast<int>(OverwriteMode::AutoRename));
  m_overwriteModeCombo->setToolTip(
    tr("How to handle files that already exist at the destination."));
  overwriteForm->addRow(tr("On overwrite (%1):").arg(altO), m_overwriteModeCombo);

  // Auto-rename テンプレート: AutoRename モード時のみ有効
  m_autoRenameEdit = new QLineEdit(this);
  m_autoRenameEdit->setText(Settings::instance().autoRenameTemplate());
  m_autoRenameEdit->setToolTip(
    tr("Suffix appended to rename conflicting files. "
       "Use {n} as the counter placeholder (e.g., ' ({n})' → 'foo (1).txt')."));
  overwriteForm->addRow(tr("Rename suffix (%1):").arg(altS), m_autoRenameEdit);
  auto updateEditEnabled = [this]() {
    const auto mode = static_cast<OverwriteMode>(
      m_overwriteModeCombo->currentData().toInt());
    m_autoRenameEdit->setEnabled(mode == OverwriteMode::AutoRename);
  };
  connect(m_overwriteModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [updateEditEnabled](int) { updateEditEnabled(); });
  updateEditEnabled();

  mainLayout->addLayout(overwriteForm);

  // Buttons
  m_buttonBox = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  auto* okBtn     = m_buttonBox->button(QDialogButtonBox::Ok);
  auto* cancelBtn = m_buttonBox->button(QDialogButtonBox::Cancel);
  okBtn->setText(op == Copy ? tr("Copy") : tr("Move"));
  applyAltShortcut(okBtn,     op == Copy ? Qt::Key_C : Qt::Key_M);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  okBtn->setDefault(true);
  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(m_buttonBox);

  // Tab ナビゲーション: items → combobox → rename suffix → Cancel → Copy/Move。
  // 実行系ボタンを最後に置くことで Tab 連打による誤操作を防ぐ。
  list->setFocusPolicy(Qt::StrongFocus);
  m_overwriteModeCombo->setFocusPolicy(Qt::StrongFocus);
  m_autoRenameEdit->setFocusPolicy(Qt::StrongFocus);
  setTabOrder(list, m_overwriteModeCombo);
  setTabOrder(m_overwriteModeCombo, m_autoRenameEdit);
  setTabOrder(m_autoRenameEdit, cancelBtn);
  setTabOrder(cancelBtn, okBtn);
}

void TransferConfirmDialog::keyPressEvent(QKeyEvent* event) {
  // Alt+ラベル文字 で対応フィールドにフォーカス移動（Windows 風）。
  // QLabel::setBuddy だけだと macOS で動かないことがあるため、明示的に処理する。
  if (event->modifiers() & Qt::AltModifier) {
    switch (event->key()) {
      case Qt::Key_O:
        if (m_overwriteModeCombo) {
          m_overwriteModeCombo->setFocus();
          m_overwriteModeCombo->showPopup();
        }
        event->accept();
        return;
      case Qt::Key_S:
        if (m_autoRenameEdit && m_autoRenameEdit->isEnabled()) {
          m_autoRenameEdit->setFocus();
          m_autoRenameEdit->selectAll();
        }
        event->accept();
        return;
      default:
        break;
    }
  }

  QDialog::keyPressEvent(event);
}

OverwriteMode TransferConfirmDialog::overwriteMode() const {
  return static_cast<OverwriteMode>(m_overwriteModeCombo->currentData().toInt());
}

QString TransferConfirmDialog::autoRenameTemplate() const {
  return m_autoRenameEdit->text();
}

} // namespace Farman

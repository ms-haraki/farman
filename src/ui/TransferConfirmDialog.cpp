#include "TransferConfirmDialog.h"
#include "settings/Settings.h"
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

  // Overwrite mode
  QFormLayout* overwriteForm = new QFormLayout();
  m_overwriteModeCombo = new QComboBox(this);
  m_overwriteModeCombo->addItem(tr("Ask"),            static_cast<int>(OverwriteMode::Ask));
  m_overwriteModeCombo->addItem(tr("Auto-overwrite"), static_cast<int>(OverwriteMode::AutoOverwrite));
  m_overwriteModeCombo->addItem(tr("Auto-rename"),    static_cast<int>(OverwriteMode::AutoRename));
  m_overwriteModeCombo->setToolTip(
    tr("How to handle files that already exist at the destination"));
  overwriteForm->addRow(tr("On overwrite:"), m_overwriteModeCombo);

  // Auto-rename テンプレート: AutoRename モード時のみ有効
  m_autoRenameEdit = new QLineEdit(this);
  m_autoRenameEdit->setText(Settings::instance().autoRenameTemplate());
  m_autoRenameEdit->setToolTip(
    tr("Suffix appended to rename conflicting files. "
       "Use {n} as the counter placeholder (e.g., ' ({n})' → 'foo (1).txt')."));
  overwriteForm->addRow(tr("Rename suffix:"), m_autoRenameEdit);
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
  m_buttonBox->button(QDialogButtonBox::Ok)->setText(
    op == Copy ? tr("Copy") : tr("Move"));
  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(m_buttonBox);
}

OverwriteMode TransferConfirmDialog::overwriteMode() const {
  return static_cast<OverwriteMode>(m_overwriteModeCombo->currentData().toInt());
}

QString TransferConfirmDialog::autoRenameTemplate() const {
  return m_autoRenameEdit->text();
}

} // namespace Farman

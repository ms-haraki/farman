#include "DirectoryCompareDialog.h"
#include "utils/Dialogs.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace Farman {

DirectoryCompareDialog::DirectoryCompareDialog(const QString& leftPath,
                                               const QString& rightPath,
                                               QWidget* parent)
  : QDialog(parent) {
  setupUi(leftPath, rightPath);
}

void DirectoryCompareDialog::setupUi(const QString& leftPath, const QString& rightPath) {
  setWindowTitle(tr("Compare Directories"));

  QVBoxLayout* main = new QVBoxLayout(this);

  // 左右パスの確認表示 (読取専用)
  auto* leftLabel  = new QLabel(tr("Left:  %1").arg(leftPath),  this);
  auto* rightLabel = new QLabel(tr("Right: %1").arg(rightPath), this);
  leftLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  rightLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  main->addWidget(leftLabel);
  main->addWidget(rightLabel);

  // 粒度
  auto* granGroup = new QGroupBox(tr("Compare by"), this);
  auto* granLayout = new QVBoxLayout(granGroup);
  m_radioNameOnly  = new QRadioButton(tr("File name only"), this);
  m_radioSizeMtime = new QRadioButton(tr("Size + modified time"), this);
  m_radioHash      = new QRadioButton(tr("SHA-256 hash (Phase B, not yet implemented)"), this);
  m_radioSizeMtime->setChecked(true);
  m_radioHash->setEnabled(false);
  granLayout->addWidget(m_radioNameOnly);
  granLayout->addWidget(m_radioSizeMtime);
  granLayout->addWidget(m_radioHash);
  main->addWidget(granGroup);

  // 再帰 (Phase B 以降、現状 disabled)
  m_recursiveCheck = new QCheckBox(tr("Recurse into subdirectories (Phase B, not yet implemented)"), this);
  m_recursiveCheck->setEnabled(false);
  main->addWidget(m_recursiveCheck);

  // OK / Cancel
  auto* buttons = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons->button(QDialogButtonBox::Ok)->setText(tr("Compare"));
  applyAltShortcut(buttons->button(QDialogButtonBox::Ok),     Qt::Key_O);
  applyAltShortcut(buttons->button(QDialogButtonBox::Cancel), Qt::Key_C);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  main->addWidget(buttons);

  setMinimumWidth(420);
}

CompareOptions DirectoryCompareDialog::options() const {
  CompareOptions o;
  if (m_radioNameOnly && m_radioNameOnly->isChecked()) {
    o.granularity = CompareGranularity::NameOnly;
  } else if (m_radioHash && m_radioHash->isChecked()) {
    o.granularity = CompareGranularity::Hash;
  } else {
    o.granularity = CompareGranularity::SizeMtime;
  }
  o.recursive = m_recursiveCheck && m_recursiveCheck->isChecked();
  return o;
}

} // namespace Farman

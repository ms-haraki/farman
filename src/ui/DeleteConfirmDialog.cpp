#include "DeleteConfirmDialog.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

DeleteConfirmDialog::DeleteConfirmDialog(const QString& message,
                                         bool defaultToTrash,
                                         QWidget* parent)
  : QDialog(parent)
  , m_trashRadio(nullptr)
  , m_permanentRadio(nullptr) {
  setupUi(message, defaultToTrash);
}

bool DeleteConfirmDialog::toTrash() const {
  return m_trashRadio ? m_trashRadio->isChecked() : true;
}

void DeleteConfirmDialog::setupUi(const QString& message, bool defaultToTrash) {
  setWindowTitle(tr("Confirm Delete"));
  setModal(true);
  resize(480, 0);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  auto* label = new QLabel(message, this);
  label->setWordWrap(true);
  mainLayout->addWidget(label);

  // ── Action 選択 ──
  // ラジオラベルに Alt+key の視覚ヒントを埋める (withAltMnemonic 経由)。
  QGroupBox* actionGroup = new QGroupBox(tr("Action"), this);
  QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
  m_trashRadio     = new QRadioButton(
    withAltMnemonic(tr("Move to Trash"), Qt::Key_T), this);
  m_permanentRadio = new QRadioButton(
    withAltMnemonic(tr("Delete permanently"), Qt::Key_P), this);
  m_trashRadio->setFocusPolicy(Qt::StrongFocus);
  m_permanentRadio->setFocusPolicy(Qt::StrongFocus);
  (defaultToTrash ? m_trashRadio : m_permanentRadio)->setChecked(true);
  actionLayout->addWidget(m_trashRadio);
  actionLayout->addWidget(m_permanentRadio);
  mainLayout->addWidget(actionGroup);

  QButtonGroup* group = new QButtonGroup(this);
  group->addButton(m_trashRadio);
  group->addButton(m_permanentRadio);

  // ── OK / Cancel ──
  QHBoxLayout* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  auto* cancelBtn = new QPushButton(tr("Cancel"), this);
  auto* okBtn     = new QPushButton(tr("OK"),     this);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  applyAltShortcut(okBtn,     Qt::Key_O);
  okBtn->setDefault(true);
  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(okBtn);
  mainLayout->addLayout(btnLayout);

  connect(okBtn,     &QPushButton::clicked, this, &QDialog::accept);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

  // Tab 順: Trash → Permanent → Cancel → OK
  setTabOrder(m_trashRadio,     m_permanentRadio);
  setTabOrder(m_permanentRadio, cancelBtn);
  setTabOrder(cancelBtn,        okBtn);
}

void DeleteConfirmDialog::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() & Qt::AltModifier) {
    switch (event->key()) {
      case Qt::Key_T:
        m_trashRadio->setChecked(true);
        event->accept();
        return;
      case Qt::Key_P:
        m_permanentRadio->setChecked(true);
        event->accept();
        return;
      default: break;
    }
  }
  QDialog::keyPressEvent(event);
}

} // namespace Farman

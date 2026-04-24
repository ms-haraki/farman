#include "ExtractArchiveDialog.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QStyle>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

ExtractArchiveDialog::ExtractArchiveDialog(const QString& archivePath,
                                           const QString& defaultOutputDir,
                                           QWidget*       parent)
  : QDialog(parent)
  , m_dirEdit(nullptr)
  , m_browseButton(nullptr) {
  setupUi(archivePath, defaultOutputDir);
}

QString ExtractArchiveDialog::outputDirectory() const {
  return m_dirEdit->text().trimmed();
}

void ExtractArchiveDialog::setupUi(const QString& archivePath,
                                   const QString& defaultOutputDir) {
  setWindowTitle(tr("Extract Archive"));
  setModal(true);
  resize(560, 0);

  const QString altD = QKeySequence(Qt::ALT | Qt::Key_D).toString(QKeySequence::NativeText);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  QFormLayout* form = new QFormLayout();
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  form->addRow(tr("Archive:"), new QLabel(archivePath, this));

  QWidget* dirRow = new QWidget(this);
  QHBoxLayout* dirRowLayout = new QHBoxLayout(dirRow);
  dirRowLayout->setContentsMargins(0, 0, 0, 0);
  m_dirEdit = new QLineEdit(defaultOutputDir, this);
  m_dirEdit->setFocusPolicy(Qt::StrongFocus);
  m_browseButton = new QPushButton(this);
  m_browseButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  m_browseButton->setToolTip(tr("Browse folder..."));
  m_browseButton->setFocusPolicy(Qt::StrongFocus);
  dirRowLayout->addWidget(m_dirEdit, 1);
  dirRowLayout->addWidget(m_browseButton);
  form->addRow(tr("Output directory (%1):").arg(altD), dirRow);

  mainLayout->addLayout(form);

  QHBoxLayout* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  auto* cancelBtn = new QPushButton(tr("Cancel"), this);
  auto* okBtn     = new QPushButton(tr("Extract"), this);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  applyAltShortcut(okBtn,     Qt::Key_E);
  okBtn->setDefault(true);
  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(okBtn);
  mainLayout->addLayout(btnLayout);

  connect(cancelBtn,      &QPushButton::clicked, this, &QDialog::reject);
  connect(okBtn,          &QPushButton::clicked, this, &QDialog::accept);
  connect(m_browseButton, &QPushButton::clicked, this, &ExtractArchiveDialog::onBrowseDir);

  setTabOrder(m_dirEdit,      m_browseButton);
  setTabOrder(m_browseButton, cancelBtn);
  setTabOrder(cancelBtn,      okBtn);

  m_dirEdit->setFocus();
  m_dirEdit->selectAll();
}

void ExtractArchiveDialog::onBrowseDir() {
  const QString start = m_dirEdit->text().trimmed().isEmpty()
                          ? QDir::homePath()
                          : m_dirEdit->text().trimmed();
  const QString selected = QFileDialog::getExistingDirectory(
    this, tr("Select Output Directory"), start,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!selected.isEmpty()) m_dirEdit->setText(selected);
}

void ExtractArchiveDialog::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() & Qt::AltModifier) {
    if (event->key() == Qt::Key_D) {
      m_dirEdit->setFocus();
      m_dirEdit->selectAll();
      event->accept();
      return;
    }
  }
  QDialog::keyPressEvent(event);
}

} // namespace Farman

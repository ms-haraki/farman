#include "ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>

namespace Farman {

ProgressDialog::ProgressDialog(
  const QString& operationName,
  QWidget*       parent)
  : QDialog(parent)
  , m_worker(nullptr) {

  setupUI(operationName);
  setModal(true);
  setMinimumWidth(400);
}

void ProgressDialog::setupUI(const QString& operationName) {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Operation label
  m_operationLabel = new QLabel(operationName, this);
  QFont boldFont = m_operationLabel->font();
  boldFont.setBold(true);
  m_operationLabel->setFont(boldFont);
  mainLayout->addWidget(m_operationLabel);

  // Current file label
  m_currentFileLabel = new QLabel("", this);
  m_currentFileLabel->setWordWrap(true);
  mainLayout->addWidget(m_currentFileLabel);

  // Progress bar
  m_progressBar = new QProgressBar(this);
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  mainLayout->addWidget(m_progressBar);

  // Button layout
  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();

  m_cancelButton = new QPushButton("Cancel", this);
  connect(m_cancelButton, &QPushButton::clicked, this, &ProgressDialog::onCancel);
  buttonLayout->addWidget(m_cancelButton);

  mainLayout->addLayout(buttonLayout);

  setLayout(mainLayout);
}

void ProgressDialog::setWorker(WorkerBase* worker) {
  m_worker = worker;

  if (m_worker) {
    connect(m_worker, &WorkerBase::progressUpdated,
            this, &ProgressDialog::onProgressUpdated);
    connect(m_worker, &WorkerBase::finished,
            this, &ProgressDialog::onFinished);
  }
}

void ProgressDialog::onProgressUpdated(const WorkerProgress& progress) {
  // Update current file label
  if (!progress.currentFile.isEmpty()) {
    QFileInfo info(progress.currentFile);
    m_currentFileLabel->setText(QString("Processing: %1").arg(info.fileName()));
  }

  // Update progress bar
  if (progress.filesTotal > 0) {
    int percentage = (progress.filesDone * 100) / progress.filesTotal;
    m_progressBar->setValue(percentage);
    m_progressBar->setFormat(QString("%1 / %2 files (%p%)")
                             .arg(progress.filesDone)
                             .arg(progress.filesTotal));
  } else if (progress.total > 0) {
    int percentage = (progress.processed * 100) / progress.total;
    m_progressBar->setValue(percentage);
    m_progressBar->setFormat(QString("%p%"));
  } else {
    // Indeterminate progress
    m_progressBar->setRange(0, 0);
    m_progressBar->setFormat(QString("%1 files processed").arg(progress.filesDone));
  }
}

void ProgressDialog::onFinished(bool success) {
  if (success) {
    accept();
  } else {
    reject();
  }
}

void ProgressDialog::onCancel() {
  if (m_worker) {
    m_worker->requestCancel();
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

} // namespace Farman

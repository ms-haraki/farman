#include "CreateArchiveDialog.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QStyle>
#include <QKeyEvent>
#include <QKeySequence>

namespace Farman {

CreateArchiveDialog::CreateArchiveDialog(const QStringList& inputPaths,
                                         const QString&     defaultOutputDir,
                                         QWidget*           parent)
  : QDialog(parent)
  , m_inputPaths(inputPaths)
  , m_formatCombo(nullptr)
  , m_dirEdit(nullptr)
  , m_browseButton(nullptr)
  , m_nameEdit(nullptr) {
  setupUi(defaultOutputDir);
}

QString CreateArchiveDialog::outputPath() const {
  return QDir(m_dirEdit->text().trimmed()).absoluteFilePath(m_nameEdit->text().trimmed());
}

ArchiveCreateWorker::Format CreateArchiveDialog::format() const {
  return static_cast<ArchiveCreateWorker::Format>(m_formatCombo->currentData().toInt());
}

QString CreateArchiveDialog::baseName() const {
  // 単一／複数選択にかかわらず、先頭エントリ名を起点にする。
  // ファイルなら最後の拡張子を取り除く（`foo.txt` → `foo`）、ディレクトリは
  // そのまま（`photos.2024` などドット付きでも保持する）。
  if (m_inputPaths.isEmpty()) return QStringLiteral("archive");
  const QFileInfo fi(m_inputPaths.first());
  const QString name = fi.isDir() ? fi.fileName() : fi.completeBaseName();
  return name.isEmpty() ? QStringLiteral("archive") : name;
}

QString CreateArchiveDialog::extensionForFormat(ArchiveCreateWorker::Format fmt) const {
  switch (fmt) {
    case ArchiveCreateWorker::Format::Zip:    return QStringLiteral(".zip");
    case ArchiveCreateWorker::Format::Tar:    return QStringLiteral(".tar");
    case ArchiveCreateWorker::Format::TarGz:  return QStringLiteral(".tar.gz");
    case ArchiveCreateWorker::Format::TarBz2: return QStringLiteral(".tar.bz2");
    case ArchiveCreateWorker::Format::TarXz:  return QStringLiteral(".tar.xz");
  }
  return QStringLiteral(".zip");
}

void CreateArchiveDialog::setupUi(const QString& defaultOutputDir) {
  setWindowTitle(tr("Create Archive"));
  setModal(true);
  resize(560, 0);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  QFormLayout* form = new QFormLayout();
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  // Format
  // ラベルに Alt+key の視覚ヒントを埋め (withAltMnemonic 経由) + setBuddy で
  // Alt+key 押下時に対応フィールドへフォーカスを送る。
  m_formatCombo = new QComboBox(this);
  m_formatCombo->addItem(QStringLiteral("zip"),     static_cast<int>(ArchiveCreateWorker::Format::Zip));
  m_formatCombo->addItem(QStringLiteral("tar"),     static_cast<int>(ArchiveCreateWorker::Format::Tar));
  m_formatCombo->addItem(QStringLiteral("tar.gz"),  static_cast<int>(ArchiveCreateWorker::Format::TarGz));
  m_formatCombo->addItem(QStringLiteral("tar.bz2"), static_cast<int>(ArchiveCreateWorker::Format::TarBz2));
  m_formatCombo->addItem(QStringLiteral("tar.xz"),  static_cast<int>(ArchiveCreateWorker::Format::TarXz));
  m_formatCombo->setFocusPolicy(Qt::StrongFocus);
  auto* formatLabel = new QLabel(withAltMnemonic(tr("Format:"), Qt::Key_F), this);
  formatLabel->setBuddy(m_formatCombo);
  form->addRow(formatLabel, m_formatCombo);

  // Output directory + Browse
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
  auto* dirLabel = new QLabel(withAltMnemonic(tr("Directory:"), Qt::Key_D), this);
  dirLabel->setBuddy(m_dirEdit);
  form->addRow(dirLabel, dirRow);

  // Output filename
  m_nameEdit = new QLineEdit(this);
  m_nameEdit->setFocusPolicy(Qt::StrongFocus);
  auto* nameLabel = new QLabel(withAltMnemonic(tr("File name:"), Qt::Key_M), this);
  nameLabel->setBuddy(m_nameEdit);
  form->addRow(nameLabel, m_nameEdit);

  mainLayout->addLayout(form);

  // ボタン
  QHBoxLayout* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  auto* cancelBtn = new QPushButton(tr("Cancel"), this);
  auto* okBtn     = new QPushButton(tr("Create"), this);
  applyAltShortcut(cancelBtn, Qt::Key_X);
  applyAltShortcut(okBtn,     Qt::Key_C);
  okBtn->setDefault(true);
  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(okBtn);
  mainLayout->addLayout(btnLayout);

  connect(cancelBtn,     &QPushButton::clicked, this, &QDialog::reject);
  connect(okBtn,         &QPushButton::clicked, this, &QDialog::accept);
  connect(m_browseButton,&QPushButton::clicked, this, &CreateArchiveDialog::onBrowseDir);
  connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int) { onFormatChanged(); });

  // Tab: format → dir → browse → name → Cancel → OK
  setTabOrder(m_formatCombo,  m_dirEdit);
  setTabOrder(m_dirEdit,      m_browseButton);
  setTabOrder(m_browseButton, m_nameEdit);
  setTabOrder(m_nameEdit,     cancelBtn);
  setTabOrder(cancelBtn,      okBtn);

  // 初期ファイル名
  m_nameEdit->setText(baseName() + extensionForFormat(format()));
  m_nameEdit->setFocus();
  // 拡張子の手前にカーソルを置く
  const int extStart = m_nameEdit->text().indexOf(QLatin1Char('.'));
  if (extStart >= 0) {
    m_nameEdit->setCursorPosition(extStart);
    m_nameEdit->setSelection(0, extStart);
  } else {
    m_nameEdit->selectAll();
  }
}

void CreateArchiveDialog::onBrowseDir() {
  const QString start = m_dirEdit->text().trimmed().isEmpty()
                          ? QDir::homePath()
                          : m_dirEdit->text().trimmed();
  const QString selected = QFileDialog::getExistingDirectory(
    this, tr("Select Output Directory"), start,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!selected.isEmpty()) m_dirEdit->setText(selected);
}

void CreateArchiveDialog::onFormatChanged() {
  // 既存の拡張子を置き換える
  QString name = m_nameEdit->text();
  const QStringList knownExts = {".tar.gz", ".tar.bz2", ".tar.xz", ".tar", ".zip"};
  for (const QString& e : knownExts) {
    if (name.endsWith(e, Qt::CaseInsensitive)) {
      name.chop(e.size());
      break;
    }
  }
  m_nameEdit->setText(name + extensionForFormat(format()));
}

void CreateArchiveDialog::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() & Qt::AltModifier) {
    switch (event->key()) {
      case Qt::Key_F:
        m_formatCombo->setFocus();
        m_formatCombo->showPopup();
        event->accept();
        return;
      case Qt::Key_D:
        m_dirEdit->setFocus();
        m_dirEdit->selectAll();
        event->accept();
        return;
      case Qt::Key_M:
        m_nameEdit->setFocus();
        m_nameEdit->selectAll();
        event->accept();
        return;
      default: break;
    }
  }
  QDialog::keyPressEvent(event);
}

} // namespace Farman

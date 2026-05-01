#include "PropertiesDialog.h"
#include "core/workers/PropertiesWorker.h"
#include "utils/Dialogs.h"
#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

namespace Farman {

namespace {

QString permissionsToString(QFile::Permissions p) {
  QString s = QStringLiteral("---------");
  if (p & QFile::ReadOwner)  s[0] = QLatin1Char('r');
  if (p & QFile::WriteOwner) s[1] = QLatin1Char('w');
  if (p & QFile::ExeOwner)   s[2] = QLatin1Char('x');
  if (p & QFile::ReadGroup)  s[3] = QLatin1Char('r');
  if (p & QFile::WriteGroup) s[4] = QLatin1Char('w');
  if (p & QFile::ExeGroup)   s[5] = QLatin1Char('x');
  if (p & QFile::ReadOther)  s[6] = QLatin1Char('r');
  if (p & QFile::WriteOther) s[7] = QLatin1Char('w');
  if (p & QFile::ExeOther)   s[8] = QLatin1Char('x');
  return s;
}

QString formatSize(qint64 bytes) {
  // バイト数 + 人間向け表記。
  // 例: "1,234,567 B (1.2 MB)"
  const QString num = QLocale(QLocale::English).toString(bytes);
  const QString human = QLocale(QLocale::English).formattedDataSize(bytes);
  if (bytes < 1024) return QStringLiteral("%1 B").arg(num);
  return QStringLiteral("%1 B (%2)").arg(num, human);
}

QString formatDate(const QDateTime& t) {
  if (!t.isValid()) return QStringLiteral("—");
  return t.toString(QStringLiteral("yyyy/MM/dd HH:mm:ss"));
}

} // namespace

PropertiesDialog::PropertiesDialog(const QString& path, QWidget* parent)
  : QDialog(parent)
  , m_path(path) {
  setWindowTitle(tr("Properties"));
  resize(560, 0);
  setModal(true);
  setupUi();
  populateStaticInfo();

  // ディレクトリの場合は別スレッドでサイズ計算開始。
  if (m_isDir) {
    m_sizeLabel->setText(tr("calculating..."));
    m_worker = new PropertiesWorker(m_path, this);
    connect(m_worker, &PropertiesWorker::statsUpdated,
            this, &PropertiesDialog::onStatsUpdated);
    connect(m_worker, &WorkerBase::finished,
            this, &PropertiesDialog::onWorkerFinished);
    m_worker->start();
  }
}

PropertiesDialog::~PropertiesDialog() {
  // 念のため: ダイアログ終了時にワーカーを停止する。
  if (m_worker) {
    m_worker->requestCancel();
    m_worker->wait(2000);
  }
}

void PropertiesDialog::setupUi() {
  auto* mainLayout = new QVBoxLayout(this);

  m_form = new QFormLayout();
  m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  // ラベル名 (Name: / Path: 等) を左寄せにすることで、ラベル列がダイアログの
  // 左端に張り付き、値列も左から始まる。深いパスが入っても自然に見える。
  m_form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto makeValueLabel = [this]() -> QLabel* {
    auto* lbl = new QLabel(this);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl->setWordWrap(true);
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return lbl;
  };

  m_nameLabel        = makeValueLabel();
  m_pathLabel        = makeValueLabel();
  m_typeLabel        = makeValueLabel();
  m_sizeLabel        = makeValueLabel();
  m_modifiedLabel    = makeValueLabel();
  m_createdLabel     = makeValueLabel();
  m_accessedLabel    = makeValueLabel();
  m_permissionsLabel = makeValueLabel();
  m_ownerLabel       = makeValueLabel();
  m_groupLabel       = makeValueLabel();
  m_linkTargetLabel  = makeValueLabel();

  m_form->addRow(tr("Name:"),        m_nameLabel);
  m_form->addRow(tr("Path:"),        m_pathLabel);
  m_form->addRow(tr("Type:"),        m_typeLabel);
  m_form->addRow(tr("Size:"),        m_sizeLabel);
  m_form->addRow(tr("Modified:"),    m_modifiedLabel);
  m_form->addRow(tr("Created:"),     m_createdLabel);
  m_form->addRow(tr("Accessed:"),    m_accessedLabel);
  m_form->addRow(tr("Permissions:"), m_permissionsLabel);
  m_form->addRow(tr("Owner:"),       m_ownerLabel);
  m_form->addRow(tr("Group:"),       m_groupLabel);
  // リンク先はシンボリックリンクの場合のみ後で表示する。行は常に置いておく
  // が空のままでもよい。
  m_form->addRow(tr("Link Target:"), m_linkTargetLabel);

  mainLayout->addLayout(m_form);

  // OK だけのボタンバー (読み取り専用なので Cancel は不要)。
  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
  m_closeButton = buttonBox->button(QDialogButtonBox::Close);
  applyAltShortcut(m_closeButton, Qt::Key_C);
  m_closeButton->setDefault(true);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
  connect(m_closeButton, &QPushButton::clicked,   this, &QDialog::accept);
  mainLayout->addWidget(buttonBox);
}

void PropertiesDialog::populateStaticInfo() {
  QFileInfo fi(m_path);
  m_isDir = fi.isDir() && !fi.isSymLink();

  m_nameLabel->setText(fi.fileName().isEmpty() ? fi.absoluteFilePath()
                                                : fi.fileName());
  m_pathLabel->setText(fi.absoluteFilePath());

  // 種別: ファイル / ディレクトリ / シンボリックリンク
  QString type;
  if (fi.isSymLink())     type = tr("Symbolic link");
  else if (fi.isDir())    type = tr("Directory");
  else if (fi.isFile())   type = tr("File");
  else                     type = tr("Other");
  m_typeLabel->setText(type);

  // サイズ: ファイルなら即座に確定、ディレクトリなら後で worker が更新
  if (m_isDir) {
    m_sizeLabel->setText(tr("calculating..."));
  } else {
    m_sizeLabel->setText(formatSize(fi.size()));
  }

  m_modifiedLabel->setText(formatDate(fi.lastModified()));
  m_createdLabel ->setText(formatDate(fi.birthTime()));
  m_accessedLabel->setText(formatDate(fi.lastRead()));

  m_permissionsLabel->setText(permissionsToString(fi.permissions()));

  const QString owner = fi.owner();
  m_ownerLabel->setText(owner.isEmpty() ? QString::number(fi.ownerId())
                                         : owner);
  const QString group = fi.group();
  m_groupLabel->setText(group.isEmpty() ? QString::number(fi.groupId())
                                         : group);

  if (fi.isSymLink()) {
    m_linkTargetLabel->setText(fi.symLinkTarget());
  } else {
    m_linkTargetLabel->setText(QStringLiteral("—"));
  }

  // プラットフォーム別に意味の薄い行を非表示にする。
  //   - Windows は ACL ベースなので Permissions の rwx 表記は実態と乖離する。
  //     Owner / Group も Unix uid/gid 概念とは違うため隠す。
  //     Link Target はシンボリックリンク自体が稀なので常に隠す。
  //   - macOS / Linux は通常通りすべて表示しつつ、Link Target だけは
  //     シンボリックリンクの場合に限って表示する (それ以外の行高を詰めるため)。
  // Settings → Behavior → List Display の列表示と方針を揃えている。
#ifdef Q_OS_WIN
  m_form->setRowVisible(m_permissionsLabel, false);
  m_form->setRowVisible(m_ownerLabel,       false);
  m_form->setRowVisible(m_groupLabel,       false);
  m_form->setRowVisible(m_linkTargetLabel,  false);
#else
  if (!fi.isSymLink()) {
    m_form->setRowVisible(m_linkTargetLabel, false);
  }
#endif
}

void PropertiesDialog::onStatsUpdated(qint64 totalBytes, int fileCount, int dirCount) {
  m_sizeLabel->setText(
    tr("%1   (%n file(s), %2 directories)", "", fileCount)
      .arg(formatSize(totalBytes))
      .arg(QLocale(QLocale::English).toString(dirCount)));
}

void PropertiesDialog::onWorkerFinished(bool success) {
  if (!success) {
    // キャンセルされた場合は値をそのままに、注釈を付ける。
    m_sizeLabel->setText(m_sizeLabel->text() + tr("  (cancelled)"));
  }
}

void PropertiesDialog::closeEvent(QCloseEvent* event) {
  if (m_worker) {
    m_worker->requestCancel();
  }
  QDialog::closeEvent(event);
}

} // namespace Farman

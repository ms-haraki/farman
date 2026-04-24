#include "AttributesDialog.h"
#include "utils/Dialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Farman {

AttributesDialog::AttributesDialog(const QStringList& paths, QWidget* parent)
  : QDialog(parent)
  , m_paths(paths)
#ifdef Q_OS_WIN
  , m_readOnlyCheck(nullptr)
  , m_hiddenCheck(nullptr)
#else
  , m_ownerRead(nullptr)
  , m_ownerWrite(nullptr)
  , m_ownerExec(nullptr)
  , m_groupRead(nullptr)
  , m_groupWrite(nullptr)
  , m_groupExec(nullptr)
  , m_otherRead(nullptr)
  , m_otherWrite(nullptr)
  , m_otherExec(nullptr)
#endif
  , m_okButton(nullptr) {
  setupUi();
  loadFromFiles();
}

void AttributesDialog::setupUi() {
  setWindowTitle(tr("Attributes"));
  setModal(true);
  resize(420, 0);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // 見出し: 対象ファイル
  QString header;
  if (m_paths.size() == 1) {
    header = QFileInfo(m_paths.first()).fileName();
    if (header.isEmpty()) header = m_paths.first();
  } else {
    header = tr("%1 items").arg(m_paths.size());
  }
  QLabel* headerLabel = new QLabel(header, this);
  headerLabel->setStyleSheet("QLabel { font-weight: bold; }");
  mainLayout->addWidget(headerLabel);

#ifdef Q_OS_WIN
  m_readOnlyCheck = new QCheckBox(tr("Read-only"), this);
  m_hiddenCheck   = new QCheckBox(tr("Hidden"),   this);
  m_readOnlyCheck->setFocusPolicy(Qt::StrongFocus);
  m_hiddenCheck->setFocusPolicy(Qt::StrongFocus);
  if (m_paths.size() > 1) {
    m_readOnlyCheck->setTristate(true);
    m_hiddenCheck->setTristate(true);
  }
  mainLayout->addWidget(m_readOnlyCheck);
  mainLayout->addWidget(m_hiddenCheck);
#else
  QGridLayout* grid = new QGridLayout();
  grid->addWidget(new QLabel(tr("Read"),    this), 0, 1, Qt::AlignHCenter);
  grid->addWidget(new QLabel(tr("Write"),   this), 0, 2, Qt::AlignHCenter);
  grid->addWidget(new QLabel(tr("Execute"), this), 0, 3, Qt::AlignHCenter);

  auto* owner = new QLabel(tr("Owner"), this);
  auto* group = new QLabel(tr("Group"), this);
  auto* other = new QLabel(tr("Other"), this);
  grid->addWidget(owner, 1, 0);
  grid->addWidget(group, 2, 0);
  grid->addWidget(other, 3, 0);

  m_ownerRead  = new QCheckBox(this);
  m_ownerWrite = new QCheckBox(this);
  m_ownerExec  = new QCheckBox(this);
  m_groupRead  = new QCheckBox(this);
  m_groupWrite = new QCheckBox(this);
  m_groupExec  = new QCheckBox(this);
  m_otherRead  = new QCheckBox(this);
  m_otherWrite = new QCheckBox(this);
  m_otherExec  = new QCheckBox(this);

  // macOS の「キーボードナビゲーション」設定に依存せず Tab でフォーカスが
  // 当たるよう、各チェックボックスに StrongFocus を明示する。
  for (auto* cb : {m_ownerRead, m_ownerWrite, m_ownerExec,
                   m_groupRead, m_groupWrite, m_groupExec,
                   m_otherRead, m_otherWrite, m_otherExec}) {
    cb->setFocusPolicy(Qt::StrongFocus);
  }

  if (m_paths.size() > 1) {
    // 混在時は PartiallyChecked 表示。ユーザーが触らなければ変更しない。
    for (auto* cb : {m_ownerRead, m_ownerWrite, m_ownerExec,
                     m_groupRead, m_groupWrite, m_groupExec,
                     m_otherRead, m_otherWrite, m_otherExec}) {
      cb->setTristate(true);
    }
  }

  grid->addWidget(m_ownerRead,  1, 1, Qt::AlignHCenter);
  grid->addWidget(m_ownerWrite, 1, 2, Qt::AlignHCenter);
  grid->addWidget(m_ownerExec,  1, 3, Qt::AlignHCenter);
  grid->addWidget(m_groupRead,  2, 1, Qt::AlignHCenter);
  grid->addWidget(m_groupWrite, 2, 2, Qt::AlignHCenter);
  grid->addWidget(m_groupExec,  2, 3, Qt::AlignHCenter);
  grid->addWidget(m_otherRead,  3, 1, Qt::AlignHCenter);
  grid->addWidget(m_otherWrite, 3, 2, Qt::AlignHCenter);
  grid->addWidget(m_otherExec,  3, 3, Qt::AlignHCenter);

  mainLayout->addLayout(grid);
#endif

  // ボタン（他のダイアログと同じ手配置スタイル）
  QHBoxLayout* btnLayout = new QHBoxLayout();
  btnLayout->addStretch(1);
  auto* cancelBtn = new QPushButton(tr("Cancel"), this);
  m_okButton      = new QPushButton(tr("OK"),     this);
  applyAltShortcut(cancelBtn,  Qt::Key_X);
  applyAltShortcut(m_okButton, Qt::Key_O);
  m_okButton->setDefault(true);
  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(m_okButton);
  mainLayout->addLayout(btnLayout);

  connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
  connect(m_okButton, &QPushButton::clicked, this, &AttributesDialog::onAccepted);

  // Tab 順: チェックボックス群 → Cancel → OK
#ifdef Q_OS_WIN
  setTabOrder(m_readOnlyCheck, m_hiddenCheck);
  setTabOrder(m_hiddenCheck,   cancelBtn);
#else
  setTabOrder(m_ownerRead,  m_ownerWrite);
  setTabOrder(m_ownerWrite, m_ownerExec);
  setTabOrder(m_ownerExec,  m_groupRead);
  setTabOrder(m_groupRead,  m_groupWrite);
  setTabOrder(m_groupWrite, m_groupExec);
  setTabOrder(m_groupExec,  m_otherRead);
  setTabOrder(m_otherRead,  m_otherWrite);
  setTabOrder(m_otherWrite, m_otherExec);
  setTabOrder(m_otherExec,  cancelBtn);
#endif
  setTabOrder(cancelBtn, m_okButton);
}

#ifdef Q_OS_WIN
static DWORD windowsAttributes(const QString& path) {
  DWORD attrs = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
  return attrs;
}
#endif

void AttributesDialog::loadFromFiles() {
  if (m_paths.isEmpty()) return;

#ifdef Q_OS_WIN
  // Read-only / Hidden を全ファイルから収集し、共通 true/false/混在 を判定
  int roTrue = 0, roFalse = 0, hdTrue = 0, hdFalse = 0;
  for (const QString& p : m_paths) {
    DWORD attrs = windowsAttributes(p);
    if (attrs == INVALID_FILE_ATTRIBUTES) continue;
    if (attrs & FILE_ATTRIBUTE_READONLY) ++roTrue; else ++roFalse;
    if (attrs & FILE_ATTRIBUTE_HIDDEN)   ++hdTrue; else ++hdFalse;
  }
  auto tri = [](int t, int f) -> Qt::CheckState {
    if (t > 0 && f == 0) return Qt::Checked;
    if (t == 0 && f > 0) return Qt::Unchecked;
    return Qt::PartiallyChecked;
  };
  m_readOnlyCheck->setCheckState(tri(roTrue, roFalse));
  m_hiddenCheck->setCheckState(tri(hdTrue, hdFalse));
#else
  struct Count { int t = 0; int f = 0; };
  Count oR, oW, oX, gR, gW, gX, xR, xW, xX;

  for (const QString& p : m_paths) {
    QFile f(p);
    const auto perms = f.permissions();
    auto bump = [](Count& c, bool on) { if (on) ++c.t; else ++c.f; };
    // Unix では Owner / User ビットは等価なのでいずれかが立っていれば on 扱い。
    bump(oR, perms.testFlag(QFile::ReadOwner)  || perms.testFlag(QFile::ReadUser));
    bump(oW, perms.testFlag(QFile::WriteOwner) || perms.testFlag(QFile::WriteUser));
    bump(oX, perms.testFlag(QFile::ExeOwner)   || perms.testFlag(QFile::ExeUser));
    bump(gR, perms.testFlag(QFile::ReadGroup));
    bump(gW, perms.testFlag(QFile::WriteGroup));
    bump(gX, perms.testFlag(QFile::ExeGroup));
    bump(xR, perms.testFlag(QFile::ReadOther));
    bump(xW, perms.testFlag(QFile::WriteOther));
    bump(xX, perms.testFlag(QFile::ExeOther));
  }
  auto tri = [](const Count& c) -> Qt::CheckState {
    if (c.t > 0 && c.f == 0) return Qt::Checked;
    if (c.t == 0 && c.f > 0) return Qt::Unchecked;
    return Qt::PartiallyChecked;
  };
  m_ownerRead->setCheckState(tri(oR));
  m_ownerWrite->setCheckState(tri(oW));
  m_ownerExec->setCheckState(tri(oX));
  m_groupRead->setCheckState(tri(gR));
  m_groupWrite->setCheckState(tri(gW));
  m_groupExec->setCheckState(tri(gX));
  m_otherRead->setCheckState(tri(xR));
  m_otherWrite->setCheckState(tri(xW));
  m_otherExec->setCheckState(tri(xX));
#endif
}

void AttributesDialog::applyToFiles() {
#ifdef Q_OS_WIN
  const auto ro = m_readOnlyCheck->checkState();
  const auto hd = m_hiddenCheck->checkState();
  for (const QString& p : m_paths) {
    DWORD attrs = windowsAttributes(p);
    if (attrs == INVALID_FILE_ATTRIBUTES) continue;
    if (ro == Qt::Checked)   attrs |= FILE_ATTRIBUTE_READONLY;
    if (ro == Qt::Unchecked) attrs &= ~FILE_ATTRIBUTE_READONLY;
    if (hd == Qt::Checked)   attrs |= FILE_ATTRIBUTE_HIDDEN;
    if (hd == Qt::Unchecked) attrs &= ~FILE_ATTRIBUTE_HIDDEN;
    SetFileAttributesW(reinterpret_cast<LPCWSTR>(p.utf16()), attrs);
  }
#else
  // PartiallyChecked のフラグは触らない（元の値を維持）。
  // Unix で Owner/User ビットは OR で効くため、Owner 行では両方を同時操作する。
  struct Flag {
    QCheckBox*         cb;
    QFile::Permissions bits;
  };
  const Flag flags[] = {
    { m_ownerRead,  QFile::ReadOwner  | QFile::ReadUser  },
    { m_ownerWrite, QFile::WriteOwner | QFile::WriteUser },
    { m_ownerExec,  QFile::ExeOwner   | QFile::ExeUser   },
    { m_groupRead,  QFile::ReadGroup                     },
    { m_groupWrite, QFile::WriteGroup                    },
    { m_groupExec,  QFile::ExeGroup                      },
    { m_otherRead,  QFile::ReadOther                     },
    { m_otherWrite, QFile::WriteOther                    },
    { m_otherExec,  QFile::ExeOther                      },
  };

  for (const QString& p : m_paths) {
    QFile f(p);
    auto perms = f.permissions();
    for (const auto& flag : flags) {
      switch (flag.cb->checkState()) {
        case Qt::Checked:   perms |=  flag.bits; break;
        case Qt::Unchecked: perms &= ~flag.bits; break;
        case Qt::PartiallyChecked: break;  // 触らない
      }
    }
    f.setPermissions(perms);
  }
#endif
}

void AttributesDialog::onAccepted() {
  applyToFiles();
  accept();
}

} // namespace Farman

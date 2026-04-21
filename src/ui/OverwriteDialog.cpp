#include "OverwriteDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTimer>
#include <QFileInfo>

namespace Farman {

OverwriteDialog::OverwriteDialog(const QString& srcPath,
                                 const QString& dstPath,
                                 QWidget* parent)
  : QDialog(parent)
  , m_renameEdit(nullptr)
  , m_overwriteButton(nullptr)
  , m_renameButton(nullptr)
  , m_cancelButton(nullptr)
  , m_originalName(QFileInfo(dstPath).fileName()) {
  setupUi(srcPath, dstPath);
}

void OverwriteDialog::setupUi(const QString& srcPath, const QString& dstPath) {
  setWindowTitle(tr("File Exists"));
  resize(520, 0);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  QLabel* headline = new QLabel(
    tr("A file already exists at the destination."), this);
  headline->setStyleSheet("QLabel { font-weight: bold; }");
  mainLayout->addWidget(headline);

  QFormLayout* pathForm = new QFormLayout();
  pathForm->addRow(tr("Source:"),      new QLabel(srcPath, this));
  pathForm->addRow(tr("Destination:"), new QLabel(dstPath, this));
  mainLayout->addLayout(pathForm);

  // Rename 用テキスト欄（初期値は元のファイル名）
  QHBoxLayout* renameRow = new QHBoxLayout();
  renameRow->addWidget(new QLabel(tr("Rename to:"), this));
  m_renameEdit = new QLineEdit(this);
  m_renameEdit->setText(m_originalName);
  m_renameEdit->setPlaceholderText(tr("New filename"));
  connect(m_renameEdit, &QLineEdit::textChanged,
          this, &OverwriteDialog::onRenameTextChanged);
  renameRow->addWidget(m_renameEdit, 1);
  mainLayout->addLayout(renameRow);

  // カーソル初期位置: 拡張子の手前（ファイル名部分の末尾）
  // dot-file 保護のため、先頭 '.' は無視して最初の '.' を区切りに。
  int extPos = -1;
  for (int i = 1; i < m_originalName.length(); ++i) {
    if (m_originalName[i] == QLatin1Char('.')) { extPos = i; break; }
  }
  const int cursorPos = (extPos > 0) ? extPos : m_originalName.length();
  // show 後に focus + select による全選択が発動することがあるので、
  // イベントループを回してから確実にカーソル位置を設定する。
  QTimer::singleShot(0, m_renameEdit, [this, cursorPos]() {
    m_renameEdit->setFocus();
    m_renameEdit->deselect();
    m_renameEdit->setCursorPosition(cursorPos);
  });

  // ボタン
  QHBoxLayout* buttonRow = new QHBoxLayout();
  m_overwriteButton = new QPushButton(tr("Overwrite"), this);
  m_renameButton    = new QPushButton(tr("Rename"),    this);
  m_cancelButton    = new QPushButton(tr("Cancel"),    this);
  connect(m_overwriteButton, &QPushButton::clicked, this, &OverwriteDialog::onOverwrite);
  connect(m_renameButton,    &QPushButton::clicked, this, &OverwriteDialog::onRename);
  connect(m_cancelButton,    &QPushButton::clicked, this, &OverwriteDialog::onCancel);
  buttonRow->addStretch();
  buttonRow->addWidget(m_overwriteButton);
  buttonRow->addWidget(m_renameButton);
  buttonRow->addWidget(m_cancelButton);
  mainLayout->addLayout(buttonRow);

  // 初期状態: Rename ボタンは元のファイル名と同じなので無効
  onRenameTextChanged(m_renameEdit->text());
}

void OverwriteDialog::onOverwrite() {
  m_decision.action = OverwriteDecision::Action::Overwrite;
  accept();
}

void OverwriteDialog::onRename() {
  m_decision.action  = OverwriteDecision::Action::Rename;
  m_decision.newName = m_renameEdit->text().trimmed();
  accept();
}

void OverwriteDialog::onCancel() {
  m_decision.action = OverwriteDecision::Action::Cancel;
  reject();
}

void OverwriteDialog::onRenameTextChanged(const QString& text) {
  const QString trimmed = text.trimmed();
  // 空文字 or 元と同じ名前は無効
  m_renameButton->setEnabled(!trimmed.isEmpty() && trimmed != m_originalName);
}

} // namespace Farman

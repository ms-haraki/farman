#pragma once

#include "types.h"
#include <QDialog>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QDialogButtonBox;

namespace Farman {

// コピー／移動開始前の確認ダイアログ。
// - コピー元ディレクトリ
// - コピー対象の一覧（非再帰：選択したトップレベルのみ）
// - コピー先ディレクトリ
// - 上書き時の動作モード
// を表示し、OK で実行、Cancel で中止。
class TransferConfirmDialog : public QDialog {
  Q_OBJECT

public:
  enum Operation { Copy, Move };

  TransferConfirmDialog(Operation op,
                        const QString&     sourceDir,
                        const QStringList& itemPaths,
                        const QString&     destDir,
                        QWidget* parent = nullptr);
  ~TransferConfirmDialog() override = default;

  OverwriteMode overwriteMode() const;
  QString       autoRenameTemplate() const;

protected:
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUi(Operation op,
               const QString&     sourceDir,
               const QStringList& itemPaths,
               const QString&     destDir);

  QComboBox*        m_overwriteModeCombo;
  QLineEdit*        m_autoRenameEdit;
  QDialogButtonBox* m_buttonBox;
};

} // namespace Farman

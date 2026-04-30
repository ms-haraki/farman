#pragma once

#include "types.h"
#include <QDialog>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QDialogButtonBox;
class QToolButton;

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
  // ユーザーが編集した最終的なコピー先ディレクトリ (trim 済み)。
  QString       destinationDir() const;

protected:
  void keyPressEvent(QKeyEvent* event) override;
  // 宛先 LineEdit で ↓ キーを押されたときにコピー元と同じパスを流し込む。
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void setupUi(Operation op,
               const QString&     sourceDir,
               const QStringList& itemPaths,
               const QString&     destDir);
  void onBrowseDestination();

  QString           m_sourceDir;       // ↓ で流し込む値
  QString           m_originalDestDir; // ↑ で戻す値 (反対側ペインの初期値)
  QLineEdit*        m_destEdit         = nullptr;
  QToolButton*      m_destBrowseButton = nullptr;
  QComboBox*        m_overwriteModeCombo;
  QLineEdit*        m_autoRenameEdit;
  QDialogButtonBox* m_buttonBox;
};

} // namespace Farman

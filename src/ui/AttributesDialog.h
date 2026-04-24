#pragma once

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QPushButton;

namespace Farman {

// 選択ファイルの属性を変更するダイアログ。
// - macOS/Linux: owner/group/other × r/w/x の 9 チェックボックス
// - Windows: Read-only / Hidden の 2 チェックボックス
// 複数選択時は共通 ✓→Checked, 共通 ✗→Unchecked, 混在→PartiallyChecked を
// 初期表示。ユーザーが触ったもの（= PartiallyChecked でなくなった項目）のみ
// 適用する。
class AttributesDialog : public QDialog {
  Q_OBJECT

public:
  AttributesDialog(const QStringList& paths, QWidget* parent = nullptr);
  ~AttributesDialog() override = default;

private slots:
  void onAccepted();

private:
  void setupUi();
  void loadFromFiles();
  void applyToFiles();

  QStringList m_paths;

#ifdef Q_OS_WIN
  QCheckBox* m_readOnlyCheck;
  QCheckBox* m_hiddenCheck;
#else
  QCheckBox* m_ownerRead;
  QCheckBox* m_ownerWrite;
  QCheckBox* m_ownerExec;
  QCheckBox* m_groupRead;
  QCheckBox* m_groupWrite;
  QCheckBox* m_groupExec;
  QCheckBox* m_otherRead;
  QCheckBox* m_otherWrite;
  QCheckBox* m_otherExec;
#endif

  QPushButton* m_okButton;
};

} // namespace Farman

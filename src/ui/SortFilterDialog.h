#pragma once

#include "settings/Settings.h"
#include <QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QDialogButtonBox;

namespace Farman {

// 現在開いているディレクトリ用のソート・フィルタ編集ダイアログ。
// OK 時にシグナル経由で編集後の PaneSettings と保存フラグを返す。
class SortFilterDialog : public QDialog {
  Q_OBJECT

public:
  SortFilterDialog(const QString& directoryPath,
                   const PaneSettings& initial,
                   bool initiallySaved,
                   QWidget* parent = nullptr);
  ~SortFilterDialog() override = default;

  PaneSettings result() const { return m_result; }
  bool         saveForDirectory() const { return m_saveForDirectory; }

private slots:
  void onAccepted();

private:
  void setupUi(const QString& directoryPath,
               const PaneSettings& initial,
               bool initiallySaved);

  // Sort controls
  QComboBox* m_sortKeyCombo;
  QComboBox* m_sortOrderCombo;
  QComboBox* m_sortKey2ndCombo;
  QComboBox* m_sortDirsTypeCombo;
  QCheckBox* m_sortDotFirstCheck;
  QCheckBox* m_sortCaseSensitiveCheck;

  // Filter controls
  QCheckBox* m_showHiddenCheck;
  QCheckBox* m_dirsOnlyCheck;
  QCheckBox* m_filesOnlyCheck;
  QLineEdit* m_nameFiltersEdit;

  // Save toggle
  QCheckBox* m_saveCheck;

  QDialogButtonBox* m_buttonBox;

  PaneSettings m_result;
  bool         m_saveForDirectory = false;
};

} // namespace Farman

#pragma once

#include "settings/Settings.h"
#include <QWidget>
#include <QFont>
#include <QColor>

class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;
class QGridLayout;

namespace Farman {

class AppearanceTab : public QWidget {
  Q_OBJECT

public:
  explicit AppearanceTab(QWidget* parent = nullptr);
  ~AppearanceTab() override = default;

  void save();

private slots:
  void onSelectFont();

private:
  void setupUi();
  void loadSettings();

  // カテゴリ×状態（通常／選択）1 組のウィジェットをまとめた構造
  struct CategoryStateRow {
    QPushButton*  fgButton  = nullptr;
    QPushButton*  bgButton  = nullptr;
    QCheckBox*    boldCheck = nullptr;
    CategoryColor value;  // 編集中の値
  };
  struct CategoryRow {
    CategoryStateRow normal;           // active / normal
    CategoryStateRow selected;         // active / selected
    CategoryStateRow inactiveNormal;   // inactive / normal
    CategoryStateRow inactiveSelected; // inactive / selected
  };
  // 1 行分のウィジェット（1 状態ぶん）を構築
  void buildCategoryRow(QGridLayout* grid, int row,
                        FileCategory cat, const QString& label,
                        bool selected, bool inactive);
  // 色ボタンにプレビュー色を反映
  void updateColorButton(QPushButton* btn, const QColor& color);

  // Font settings
  QPushButton*   m_fontButton;
  QFont          m_selectedFont;

  // File size format
  QComboBox*     m_fileSizeFormatCombo;

  // Date/time format
  QComboBox*     m_dateTimeFormatCombo;

  // Category colors (Normal / Hidden / Directory)
  CategoryRow m_categoryRows[static_cast<int>(FileCategory::Count)];
  // 非アクティブペインのカラーを使うかのチェック
  QCheckBox*  m_useInactivePaneColorsCheck = nullptr;
  // 非アクティブ側のグリッド全体（チェック OFF 時に disable）
  QGroupBox*  m_inactivePaneGroup = nullptr;

  // Path label colors
  QPushButton* m_pathFgButton = nullptr;
  QPushButton* m_pathBgButton = nullptr;
  QColor       m_pathFgValue;
  QColor       m_pathBgValue;
  // Cursor colors
  QPushButton* m_cursorActiveButton   = nullptr;
  QPushButton* m_cursorInactiveButton = nullptr;
  QColor       m_cursorActiveValue;
  QColor       m_cursorInactiveValue;

  // Pending changes
  QString m_pendingDateTimeFormat;
};

} // namespace Farman

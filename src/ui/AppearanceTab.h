#pragma once

#include "settings/Settings.h"
#include "settings/ColorScheme.h"
#include <QWidget>
#include <QFont>
#include <QColor>

class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;
class QGridLayout;
class QSpinBox;
class QLabel;

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

  // ── Light / Dark スキーム編集 ─────────────────
  // ダイアログ内では Light/Dark 双方の ColorScheme をシャドーで保持する。
  // ウィジェットには「現在編集中の側」だけが表示され、Mode ラジオを
  // 切替えると現状値を該当スキームへ書き戻してから反対側を読み直す。
  // OK 押下時に setScheme(Light, ...) / setScheme(Dark, ...) で両側を
  // Settings に流し込み、最後に setThemeMode で適用。
  ColorScheme& currentScheme();
  void         loadFromScheme(const ColorScheme& s);
  void         saveToScheme(ColorScheme& s) const;
  // テーマモードラジオの状態を m_dialogMode に反映 + 編集対象を再計算 + 再ロード
  void         applyThemeModeChange();
  // 編集対象 (Light/Dark) を示すラベルを更新
  void         updateEditingTargetLabel();

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
  QPushButton*   m_fontButton;          // File list font
  QFont          m_selectedFont;
  QPushButton*   m_addressFontButton = nullptr;
  QFont          m_addressFontValue;

  // ファイルサイズ・日時の表示形式は Behavior タブ側に移動した

  // ファイルリスト 1 行の縦幅 (0 = Auto)
  QSpinBox*      m_rowHeightSpin = nullptr;

  // Category colors (Normal / Hidden / Directory)
  CategoryRow m_categoryRows[static_cast<int>(FileCategory::Count)];
  // 非アクティブパネル設定 (グループ自体が checkable で ON/OFF を兼ねる)
  QGroupBox*  m_inactivePaneGroup = nullptr;

  // Address bar colors
  QPushButton* m_addressFgButton = nullptr;
  QPushButton* m_addressBgButton = nullptr;
  QColor       m_addressFgValue;
  QColor       m_addressBgValue;
  // Cursor colors / shape
  QPushButton* m_cursorActiveButton   = nullptr;
  QPushButton* m_cursorInactiveButton = nullptr;
  QColor       m_cursorActiveValue;
  QColor       m_cursorInactiveValue;
  QComboBox*   m_cursorShapeCombo     = nullptr;
  QSpinBox*    m_cursorThicknessSpin  = nullptr;

  // ── テーマ (Light / Dark) ─────────────────────
  QComboBox*    m_themeModeCombo     = nullptr;
  QLabel*       m_editingTargetLabel = nullptr;
  // ダイアログ内でのモード設定 (OK で確定)
  ThemeMode     m_dialogMode        = ThemeMode::Auto;
  // 現在ウィジェットが表示しているのが Light か Dark か
  ThemeMode     m_dialogEditingSide = ThemeMode::Light;
  // 「未保存の変更を含む両側のスキームスナップショット」
  ColorScheme   m_dialogLight;
  ColorScheme   m_dialogDark;

  // (旧 m_pendingDateTimeFormat* は Behavior タブへ移動)
};

} // namespace Farman

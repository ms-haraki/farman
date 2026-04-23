#pragma once

#include "types.h"
#include <QObject>
#include <QFont>
#include <QColor>
#include <QMap>
#include <QSize>
#include <QPoint>

namespace Farman {

// ファイル種別ごとのカラーリング設定
struct ColorRule {
  QString pattern;     // glob パターン: "*.cpp", "*.h"
  QColor  foreground;
  QColor  background;
  bool    bold = false;
};

// ペインごとの設定
struct PaneSettings {
  QString         path;
  SortKey         sortKey      = SortKey::Name;
  Qt::SortOrder   sortOrder    = Qt::AscendingOrder;
  SortKey         sortKey2nd   = SortKey::Name;
  SortDirsType    sortDirsType = SortDirsType::First;
  bool            sortDotFirst = true;
  Qt::CaseSensitivity sortCS   = Qt::CaseInsensitive;
  QStringList     nameFilters;
  AttrFilterFlags attrFilter   = AttrFilter::None;
};

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings& instance();

  // ── ペイン設定 ─────────────────────────
  PaneSettings paneSettings(PaneType pane) const;
  void setPaneSettings(PaneType pane, const PaneSettings& s);

  // ── ディレクトリ単位のソート・フィルタ上書き ─
  // 絶対パスをキーに、そのディレクトリ専用の PaneSettings を保持する。
  // PaneSettings::path は未使用。
  bool         hasPathOverride(const QString& path) const;
  PaneSettings pathOverride(const QString& path)    const;
  void         setPathOverride(const QString& path, const PaneSettings& s);
  void         removePathOverride(const QString& path);

  // ── 表示設定 ───────────────────────────
  QFont  font()                        const;
  void   setFont(const QFont& font);

  FileSizeFormat fileSizeFormat()       const;
  void           setFileSizeFormat(FileSizeFormat fmt);

  QString        dateTimeFormat()       const;
  void           setDateTimeFormat(const QString& fmt);

  // ── カラーリング ───────────────────────
  QList<ColorRule> colorRules()         const;
  void             setColorRules(const QList<ColorRule>& rules);

  // カテゴリ別カラー（Normal / Hidden / Directory）。
  // selected=true で選択時、inactive=true で非アクティブペイン側のスタイルを取得・設定する。
  CategoryColor    categoryColor(FileCategory cat, bool selected = false, bool inactive = false) const;
  void             setCategoryColor(FileCategory cat, bool selected, bool inactive, const CategoryColor& c);

  // 非アクティブペインの専用カラーを使用するか
  bool             useInactivePaneColors() const;
  void             setUseInactivePaneColors(bool use);

  // ペイン上部のパスラベルのカラー
  QColor           pathForeground()                 const;
  void             setPathForeground(const QColor& c);
  QColor           pathBackground()                 const;
  void             setPathBackground(const QColor& c);

  // カーソル（現在行下線）のカラー
  QColor           cursorColor(bool active)         const;
  void             setCursorColor(bool active, const QColor& c);

  // ── 起動設定 ───────────────────────────
  // ペインごとの初期表示ディレクトリ
  InitialPathMode initialPathMode(PaneType pane)            const;
  void            setInitialPathMode(PaneType pane, InitialPathMode mode);
  QString         customInitialPath(PaneType pane)          const;
  void            setCustomInitialPath(PaneType pane, const QString& path);

  bool confirmOnExit()                  const;
  void setConfirmOnExit(bool confirm);

  bool cursorLoop()                     const;
  void setCursorLoop(bool loop);

  // コピー／移動の「自動リネーム」で使うサフィックステンプレート。
  // "{n}" がカウンタのプレースホルダ。未指定なら " ({n})" を補う。
  QString autoRenameTemplate()          const;
  void    setAutoRenameTemplate(const QString& tmpl);

  // ── ウィンドウ設定 ──────────────────────
  WindowSizeMode     windowSizeMode()     const;
  void               setWindowSizeMode(WindowSizeMode mode);
  QSize              customWindowSize()   const;
  void               setCustomWindowSize(const QSize& size);
  QSize              lastWindowSize()     const;
  void               setLastWindowSize(const QSize& size);

  WindowPositionMode windowPositionMode() const;
  void               setWindowPositionMode(WindowPositionMode mode);
  QPoint             customWindowPosition() const;
  void               setCustomWindowPosition(const QPoint& pos);
  QPoint             lastWindowPosition() const;
  void               setLastWindowPosition(const QPoint& pos);

  // ── 読み書き ───────────────────────────
  void load();
  void save() const;

signals:
  void settingsChanged();

private:
  explicit Settings(QObject* parent = nullptr);

  PaneSettings     m_paneSettings[static_cast<int>(PaneType::Count)];
  QMap<QString, PaneSettings> m_pathOverrides;
  QFont            m_font;
  FileSizeFormat   m_fileSizeFormat  = FileSizeFormat::Auto;
  QString          m_dateTimeFormat  = "yyyy/MM/dd HH:mm:ss";
  QList<ColorRule> m_colorRules;
  CategoryColor    m_categoryColors[static_cast<int>(FileCategory::Count)];
  CategoryColor    m_selectedCategoryColors[static_cast<int>(FileCategory::Count)];
  CategoryColor    m_inactiveCategoryColors[static_cast<int>(FileCategory::Count)];
  CategoryColor    m_inactiveSelectedCategoryColors[static_cast<int>(FileCategory::Count)];
  bool             m_useInactivePaneColors = false;

  // Path label & cursor colors
  QColor           m_pathForeground       = QColor(Qt::black);
  QColor           m_pathBackground       = QColor(0xE0, 0xE0, 0xE0);
  QColor           m_cursorActiveColor    = QColor(Qt::black);
  QColor           m_cursorInactiveColor  = QColor(Qt::lightGray);
  // Per-pane 初期表示ディレクトリ（デフォルトは LastSession で従来動作と互換）
  InitialPathMode  m_initialPathMode[static_cast<int>(PaneType::Count)] = {
    InitialPathMode::LastSession,
    InitialPathMode::LastSession
  };
  QString          m_customInitialPath[static_cast<int>(PaneType::Count)];

  bool             m_confirmOnExit   = false;
  bool             m_cursorLoop      = false;
  QString          m_autoRenameTemplate = QStringLiteral(" ({n})");

  // Window settings
  WindowSizeMode     m_windowSizeMode     = WindowSizeMode::Default;
  QSize              m_customWindowSize   = QSize(1200, 600);
  QSize              m_lastWindowSize     = QSize(1200, 600);
  WindowPositionMode m_windowPositionMode = WindowPositionMode::Default;
  QPoint             m_customWindowPosition;
  QPoint             m_lastWindowPosition;
};

} // namespace Farman

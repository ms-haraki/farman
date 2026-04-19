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

  // ── 起動設定 ───────────────────────────
  bool restoreLastPath()                const;
  void setRestoreLastPath(bool restore);

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
  QFont            m_font;
  FileSizeFormat   m_fileSizeFormat  = FileSizeFormat::Auto;
  QString          m_dateTimeFormat  = "yyyy/MM/dd HH:mm:ss";
  QList<ColorRule> m_colorRules;
  bool             m_restoreLastPath = true;

  // Window settings
  WindowSizeMode     m_windowSizeMode     = WindowSizeMode::Default;
  QSize              m_customWindowSize   = QSize(1200, 600);
  QSize              m_lastWindowSize     = QSize(1200, 600);
  WindowPositionMode m_windowPositionMode = WindowPositionMode::Default;
  QPoint             m_customWindowPosition;
  QPoint             m_lastWindowPosition;
};

} // namespace Farman

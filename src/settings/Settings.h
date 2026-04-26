#pragma once

#include "types.h"
#include <QObject>
#include <QFont>
#include <QColor>
#include <QMap>
#include <QSize>
#include <QPoint>
#include <QList>

namespace Farman {

// ブックマーク（お気に入りディレクトリ）1件
struct Bookmark {
  QString name;
  QString path;
  // 初回起動で自動注入されるデフォルト（Home/Documents 等）は true。
  // この印が付いたブックマークは削除禁止。Rename/Move は許可。
  bool    isDefault = false;
};

// ファイル種別ごとのカラーリング設定
struct ColorRule {
  QString pattern;     // glob パターン: "*.cpp", "*.h"
  QColor  foreground;
  QColor  background;
  bool    bold = false;
};

// ペインごとの設定
// バイナリビュアー設定のシリアライズ補助
int                binaryViewerUnitToBytes(BinaryViewerUnit unit);
BinaryViewerUnit   bytesToBinaryViewerUnit(int bytes);
QString            binaryViewerEndianToString(BinaryViewerEndian endian);
BinaryViewerEndian stringToBinaryViewerEndian(const QString& str);

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
  // ファイルリスト用フォント (旧名 font())
  QFont  font()                        const;
  void   setFont(const QFont& font);

  // パス表示ラベル用フォント
  QFont  pathFont()                    const;
  void   setPathFont(const QFont& font);

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

  // カーソル形状 (Underline / RowBackground) と Underline 時の線太さ (px)
  CursorShape      cursorShape()                    const;
  void             setCursorShape(CursorShape shape);
  int              cursorThickness()                const;
  void             setCursorThickness(int px);

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

  // Shift+文字キーによる頭文字検索でドットファイル/ディレクトリの 2 文字目もマッチさせるか
  bool typeAheadIncludeDotfiles()       const;
  void setTypeAheadIncludeDotfiles(bool include);

  // ── テキストビュアー設定 ───────────────────
  QFont   textViewerFont()              const;
  void    setTextViewerFont(const QFont& font);
  QString textViewerEncoding()          const;
  void    setTextViewerEncoding(const QString& encoding);
  bool    textViewerShowLineNumbers()   const;
  void    setTextViewerShowLineNumbers(bool show);
  bool    textViewerWordWrap()          const;
  void    setTextViewerWordWrap(bool wrap);
  // 通常テキスト/選択範囲/行番号エリアの前景・背景色
  QColor  textViewerNormalForeground()  const;
  QColor  textViewerNormalBackground()  const;
  QColor  textViewerSelectedForeground()const;
  QColor  textViewerSelectedBackground()const;
  QColor  textViewerLineNumberForeground() const;
  QColor  textViewerLineNumberBackground() const;
  void    setTextViewerNormalForeground(const QColor& c);
  void    setTextViewerNormalBackground(const QColor& c);
  void    setTextViewerSelectedForeground(const QColor& c);
  void    setTextViewerSelectedBackground(const QColor& c);
  void    setTextViewerLineNumberForeground(const QColor& c);
  void    setTextViewerLineNumberBackground(const QColor& c);

  // ── 画像ビュアー設定 ───────────────────────
  int                   imageViewerZoomPercent()       const;
  void                  setImageViewerZoomPercent(int percent);
  bool                  imageViewerFitToWindow()       const;
  void                  setImageViewerFitToWindow(bool fit);
  bool                  imageViewerAnimation()         const;
  void                  setImageViewerAnimation(bool on);
  // 透明部分の表示モード (デフォルト) と各モードの色設定
  ImageTransparencyMode imageViewerTransparencyMode()  const;
  void                  setImageViewerTransparencyMode(ImageTransparencyMode mode);
  QColor                imageViewerSolidColor()        const;
  void                  setImageViewerSolidColor(const QColor& c);
  QColor                imageViewerCheckerColor1()     const;
  void                  setImageViewerCheckerColor1(const QColor& c);
  QColor                imageViewerCheckerColor2()     const;
  void                  setImageViewerCheckerColor2(const QColor& c);

  // ── バイナリビュアー設定 ───────────────────
  BinaryViewerUnit   binaryViewerUnit()     const;
  void               setBinaryViewerUnit(BinaryViewerUnit unit);
  BinaryViewerEndian binaryViewerEndian()   const;
  void               setBinaryViewerEndian(BinaryViewerEndian endian);
  QString            binaryViewerEncoding() const;
  void               setBinaryViewerEncoding(const QString& encoding);
  QFont              binaryViewerFont()     const;
  void               setBinaryViewerFont(const QFont& font);

  // ディレクトリ履歴を終了時に保存し、起動時に復元するか
  bool persistHistory()                 const;
  void setPersistHistory(bool persist);

  // ペインごとの履歴エントリ（最新順）
  QStringList paneHistory(PaneType pane) const;
  void        setPaneHistory(PaneType pane, const QStringList& entries);

  // ── ブックマーク ─────────────────────────
  QList<Bookmark> bookmarks() const;
  void            setBookmarks(const QList<Bookmark>& list);

  // コピー／移動の「自動リネーム」で使うサフィックステンプレート。
  // "{n}" がカウンタのプレースホルダ。未指定なら " ({n})" を補う。
  QString autoRenameTemplate()          const;
  void    setAutoRenameTemplate(const QString& tmpl);

  // 削除時にゴミ箱へ送るのが既定か（false なら完全削除が既定）。
  // 削除確認ダイアログで都度切替可能。
  bool    defaultDeleteToTrash()        const;
  void    setDefaultDeleteToTrash(bool toTrash);

  // ファイル検索の除外ディレクトリ（ディレクトリ名の glob パターンのリスト）。
  // 検索ダイアログの初期値に使われ、ダイアログ上で一時的に編集できる。
  QStringList searchExcludeDirs()       const;
  void        setSearchExcludeDirs(const QStringList& patterns);

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
  void settingsChanged() const;

private:
  explicit Settings(QObject* parent = nullptr);

  PaneSettings     m_paneSettings[static_cast<int>(PaneType::Count)];
  QMap<QString, PaneSettings> m_pathOverrides;
  QFont            m_font;
  QFont            m_pathFont;
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
  CursorShape      m_cursorShape          = CursorShape::Underline;
  int              m_cursorThickness      = 2;  // px (Underline 時のみ)
  // Per-pane 初期表示ディレクトリ（デフォルトは LastSession で従来動作と互換）
  InitialPathMode  m_initialPathMode[static_cast<int>(PaneType::Count)] = {
    InitialPathMode::LastSession,
    InitialPathMode::LastSession
  };
  QString          m_customInitialPath[static_cast<int>(PaneType::Count)];

  bool             m_confirmOnExit   = false;
  bool             m_cursorLoop      = false;
  bool             m_typeAheadIncludeDotfiles = true;

  // Text viewer
  QFont              m_textViewerFont;  // 既定は monospace (コンストラクタで初期化)
  QString            m_textViewerEncoding         = QStringLiteral("UTF-8");
  bool               m_textViewerShowLineNumbers  = true;
  bool               m_textViewerWordWrap         = false;
  QColor             m_textViewerNormalFg         = QColor(Qt::black);
  QColor             m_textViewerNormalBg;
  QColor             m_textViewerSelectedFg       = QColor(Qt::white);
  QColor             m_textViewerSelectedBg       = QColor(0x31, 0x6A, 0xC5);
  QColor             m_textViewerLineNumberFg     = QColor(Qt::darkGray);
  QColor             m_textViewerLineNumberBg     = QColor(0xF0, 0xF0, 0xF0);

  // Image viewer
  int                   m_imageViewerZoomPercent      = 100;
  bool                  m_imageViewerFitToWindow      = false;
  bool                  m_imageViewerAnimation        = false;
  ImageTransparencyMode m_imageViewerTransparencyMode = ImageTransparencyMode::Checker;
  QColor                m_imageViewerSolidColor       = QColor(Qt::white);
  QColor                m_imageViewerCheckerColor1    = QColor(0xC8, 0xC8, 0xC8);
  QColor                m_imageViewerCheckerColor2    = QColor(0xF0, 0xF0, 0xF0);

  // Binary viewer
  BinaryViewerUnit   m_binaryViewerUnit     = BinaryViewerUnit::Byte1;
  BinaryViewerEndian m_binaryViewerEndian   = BinaryViewerEndian::Little;
  QString            m_binaryViewerEncoding = QStringLiteral("UTF-8");
  QFont              m_binaryViewerFont;  // 既定は monospace (コンストラクタで初期化)
  bool             m_persistHistory  = false;
  QStringList      m_paneHistory[static_cast<int>(PaneType::Count)];
  QString          m_autoRenameTemplate = QStringLiteral(" ({n})");
  bool             m_defaultDeleteToTrash = true;
  QStringList      m_searchExcludeDirs    = { QStringLiteral(".*") };
  QList<Bookmark>  m_bookmarks;
  // 初回起動時のデフォルトブックマーク注入を一度きりに制限するためのフラグ。
  // false のままの設定ファイルを読むと、既存ブックマークにマージしてから true にする。
  bool             m_defaultBookmarksInstalled = false;

  // Window settings
  WindowSizeMode     m_windowSizeMode     = WindowSizeMode::Default;
  QSize              m_customWindowSize   = QSize(1200, 600);
  QSize              m_lastWindowSize     = QSize(1200, 600);
  WindowPositionMode m_windowPositionMode = WindowPositionMode::Default;
  QPoint             m_customWindowPosition;
  QPoint             m_lastWindowPosition;
};

} // namespace Farman

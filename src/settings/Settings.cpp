#include "Settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QGuiApplication>
#include <QSize>
#include <QPoint>

namespace Farman {

Settings& Settings::instance() {
  static Settings instance;
  return instance;
}

Settings::Settings(QObject* parent) : QObject(parent) {
  // Initialize with default font
  m_font = QGuiApplication::font();

  // Initialize pane settings with defaults
  for (int i = 0; i < static_cast<int>(PaneType::Count); ++i) {
    m_paneSettings[i].path = QDir::homePath();
    m_paneSettings[i].sortKey = SortKey::Name;
    m_paneSettings[i].sortOrder = Qt::AscendingOrder;
    m_paneSettings[i].sortKey2nd = SortKey::None;
    m_paneSettings[i].sortDirsType = SortDirsType::First;
    m_paneSettings[i].sortDotFirst = true;
    m_paneSettings[i].sortCS = Qt::CaseInsensitive;
    m_paneSettings[i].attrFilter = AttrFilter::None;
  }

  // Default category colors (normal state)
  m_categoryColors[static_cast<int>(FileCategory::Normal)].foreground    = QColor(Qt::black);
  m_categoryColors[static_cast<int>(FileCategory::Normal)].background    = QColor();
  m_categoryColors[static_cast<int>(FileCategory::Normal)].bold          = false;
  m_categoryColors[static_cast<int>(FileCategory::Hidden)].foreground    = QColor(140, 140, 140);
  m_categoryColors[static_cast<int>(FileCategory::Hidden)].background    = QColor();
  m_categoryColors[static_cast<int>(FileCategory::Hidden)].bold          = false;
  m_categoryColors[static_cast<int>(FileCategory::Directory)].foreground = QColor(30, 90, 200);
  m_categoryColors[static_cast<int>(FileCategory::Directory)].background = QColor();
  m_categoryColors[static_cast<int>(FileCategory::Directory)].bold       = true;

  // Default category colors (selected state): preserve existing blue/white look
  const QColor kSelBg(0, 120, 215);
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    m_selectedCategoryColors[i].foreground = QColor(Qt::white);
    m_selectedCategoryColors[i].background = kSelBg;
    m_selectedCategoryColors[i].bold       = m_categoryColors[i].bold;
  }

  // 非アクティブペイン用のデフォルト。当初は使わないがユーザー ON 時の初期値として。
  // 通常のカテゴリカラーを薄めたグレー系にする。
  auto dim = [](const QColor& c) {
    if (!c.isValid()) return QColor();
    const int avg = (c.red() + c.green() + c.blue()) / 3;
    const int dimmed = (avg + 170) / 2;  // グレーに寄せる
    return QColor(dimmed, dimmed, dimmed);
  };
  for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
    m_inactiveCategoryColors[i].foreground = dim(m_categoryColors[i].foreground);
    m_inactiveCategoryColors[i].background = m_categoryColors[i].background;
    m_inactiveCategoryColors[i].bold       = m_categoryColors[i].bold;

    m_inactiveSelectedCategoryColors[i].foreground = QColor(Qt::white);
    m_inactiveSelectedCategoryColors[i].background = QColor(140, 140, 160);
    m_inactiveSelectedCategoryColors[i].bold       = m_selectedCategoryColors[i].bold;
  }
}

PaneSettings Settings::paneSettings(PaneType pane) const {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) {
    qWarning() << "Settings::paneSettings: invalid pane type" << idx;
    return {};
  }
  return m_paneSettings[idx];
}

void Settings::setPaneSettings(PaneType pane, const PaneSettings& s) {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) {
    qWarning() << "Settings::setPaneSettings: invalid pane type" << idx;
    return;
  }
  m_paneSettings[idx] = s;
}

bool Settings::hasPathOverride(const QString& path) const {
  return m_pathOverrides.contains(path);
}

PaneSettings Settings::pathOverride(const QString& path) const {
  return m_pathOverrides.value(path);
}

void Settings::setPathOverride(const QString& path, const PaneSettings& s) {
  m_pathOverrides.insert(path, s);
}

void Settings::removePathOverride(const QString& path) {
  m_pathOverrides.remove(path);
}

QFont Settings::font() const {
  return m_font;
}

void Settings::setFont(const QFont& font) {
  m_font = font;
}

FileSizeFormat Settings::fileSizeFormat() const {
  return m_fileSizeFormat;
}

void Settings::setFileSizeFormat(FileSizeFormat fmt) {
  m_fileSizeFormat = fmt;
}

QString Settings::dateTimeFormat() const {
  return m_dateTimeFormat;
}

void Settings::setDateTimeFormat(const QString& fmt) {
  m_dateTimeFormat = fmt;
}

QList<ColorRule> Settings::colorRules() const {
  return m_colorRules;
}

void Settings::setColorRules(const QList<ColorRule>& rules) {
  m_colorRules = rules;
}

CategoryColor Settings::categoryColor(FileCategory cat, bool selected, bool inactive) const {
  int idx = static_cast<int>(cat);
  if (idx < 0 || idx >= static_cast<int>(FileCategory::Count)) return {};
  if (inactive) {
    return selected ? m_inactiveSelectedCategoryColors[idx]
                    : m_inactiveCategoryColors[idx];
  }
  return selected ? m_selectedCategoryColors[idx] : m_categoryColors[idx];
}

void Settings::setCategoryColor(FileCategory cat, bool selected, bool inactive,
                                const CategoryColor& c) {
  int idx = static_cast<int>(cat);
  if (idx < 0 || idx >= static_cast<int>(FileCategory::Count)) return;
  if (inactive) {
    if (selected) m_inactiveSelectedCategoryColors[idx] = c;
    else          m_inactiveCategoryColors[idx]         = c;
  } else {
    if (selected) m_selectedCategoryColors[idx] = c;
    else          m_categoryColors[idx]         = c;
  }
}

bool Settings::useInactivePaneColors() const {
  return m_useInactivePaneColors;
}

void Settings::setUseInactivePaneColors(bool use) {
  m_useInactivePaneColors = use;
}

QColor Settings::pathForeground() const { return m_pathForeground; }
void   Settings::setPathForeground(const QColor& c) { m_pathForeground = c; }
QColor Settings::pathBackground() const { return m_pathBackground; }
void   Settings::setPathBackground(const QColor& c) { m_pathBackground = c; }

QColor Settings::cursorColor(bool active) const {
  return active ? m_cursorActiveColor : m_cursorInactiveColor;
}
void Settings::setCursorColor(bool active, const QColor& c) {
  if (active) m_cursorActiveColor   = c;
  else        m_cursorInactiveColor = c;
}

InitialPathMode Settings::initialPathMode(PaneType pane) const {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) {
    return InitialPathMode::LastSession;
  }
  return m_initialPathMode[idx];
}

void Settings::setInitialPathMode(PaneType pane, InitialPathMode mode) {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) return;
  m_initialPathMode[idx] = mode;
}

QString Settings::customInitialPath(PaneType pane) const {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) return {};
  return m_customInitialPath[idx];
}

void Settings::setCustomInitialPath(PaneType pane, const QString& path) {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) return;
  m_customInitialPath[idx] = path;
}

bool Settings::confirmOnExit() const {
  return m_confirmOnExit;
}

void Settings::setConfirmOnExit(bool confirm) {
  m_confirmOnExit = confirm;
}

bool Settings::cursorLoop() const {
  return m_cursorLoop;
}

void Settings::setCursorLoop(bool loop) {
  m_cursorLoop = loop;
}

bool Settings::persistHistory() const {
  return m_persistHistory;
}

void Settings::setPersistHistory(bool persist) {
  m_persistHistory = persist;
}

QStringList Settings::paneHistory(PaneType pane) const {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) return {};
  return m_paneHistory[idx];
}

void Settings::setPaneHistory(PaneType pane, const QStringList& entries) {
  int idx = static_cast<int>(pane);
  if (idx < 0 || idx >= static_cast<int>(PaneType::Count)) return;
  m_paneHistory[idx] = entries;
}

QList<Bookmark> Settings::bookmarks() const {
  return m_bookmarks;
}

void Settings::setBookmarks(const QList<Bookmark>& list) {
  m_bookmarks = list;
}

QString Settings::autoRenameTemplate() const {
  return m_autoRenameTemplate;
}

void Settings::setAutoRenameTemplate(const QString& tmpl) {
  m_autoRenameTemplate = tmpl;
}

bool Settings::defaultDeleteToTrash() const {
  return m_defaultDeleteToTrash;
}

void Settings::setDefaultDeleteToTrash(bool toTrash) {
  m_defaultDeleteToTrash = toTrash;
}

WindowSizeMode Settings::windowSizeMode() const {
  return m_windowSizeMode;
}

void Settings::setWindowSizeMode(WindowSizeMode mode) {
  m_windowSizeMode = mode;
}

QSize Settings::customWindowSize() const {
  return m_customWindowSize;
}

void Settings::setCustomWindowSize(const QSize& size) {
  m_customWindowSize = size;
}

QSize Settings::lastWindowSize() const {
  return m_lastWindowSize;
}

void Settings::setLastWindowSize(const QSize& size) {
  m_lastWindowSize = size;
}

WindowPositionMode Settings::windowPositionMode() const {
  return m_windowPositionMode;
}

void Settings::setWindowPositionMode(WindowPositionMode mode) {
  m_windowPositionMode = mode;
}

QPoint Settings::customWindowPosition() const {
  return m_customWindowPosition;
}

void Settings::setCustomWindowPosition(const QPoint& pos) {
  m_customWindowPosition = pos;
}

QPoint Settings::lastWindowPosition() const {
  return m_lastWindowPosition;
}

void Settings::setLastWindowPosition(const QPoint& pos) {
  m_lastWindowPosition = pos;
}

// macOS / Windows / Linux 共通のデフォルトブックマークを返す。
// 存在するパスのみを含め、isDefault=true を付与する（= 削除不可）。
// macOS のルート "/" は実用性が低いため除外。必要な /Volumes/* や
// クラウドマウントは動的検出 (Detected locations) で扱う。
static QList<Bookmark> buildDefaultBookmarks() {
  QList<Bookmark> list;

  auto addIfExists = [&](const QString& name, const QString& path) {
    if (!path.isEmpty() && QDir(path).exists()) {
      Bookmark b;
      b.name      = name;
      b.path      = path;
      b.isDefault = true;
      list.append(b);
    }
  };

  addIfExists(QObject::tr("Home"),
              QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
  addIfExists(QObject::tr("Desktop"),
              QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
  addIfExists(QObject::tr("Documents"),
              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  addIfExists(QObject::tr("Downloads"),
              QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
  addIfExists(QObject::tr("Pictures"),
              QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
  addIfExists(QObject::tr("Music"),
              QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
  addIfExists(QObject::tr("Movies"),
              QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));

  // ドライブは macOS の "/" を除きデフォルトに含める（Windows の C:/ 等を想定）。
  const QFileInfoList drives = QDir::drives();
  for (const QFileInfo& fi : drives) {
    const QString path = fi.absoluteFilePath();
    if (path == QStringLiteral("/")) continue;
    addIfExists(path, path);
  }

  return list;
}

// Helper functions for JSON conversion
namespace {

QString sortKeyToString(SortKey key) {
  switch (key) {
    case SortKey::None: return "none";
    case SortKey::Name: return "name";
    case SortKey::Size: return "size";
    case SortKey::Type: return "type";
    case SortKey::LastModified: return "lastModified";
  }
  return "name";
}

SortKey stringToSortKey(const QString& str) {
  if (str == "none") return SortKey::None;
  if (str == "size") return SortKey::Size;
  if (str == "type") return SortKey::Type;
  if (str == "lastModified") return SortKey::LastModified;
  return SortKey::Name;
}

QString sortDirsTypeToString(SortDirsType type) {
  switch (type) {
    case SortDirsType::First: return "first";
    case SortDirsType::Last: return "last";
    case SortDirsType::Mixed: return "mixed";
  }
  return "first";
}

SortDirsType stringToSortDirsType(const QString& str) {
  if (str == "last") return SortDirsType::Last;
  if (str == "mixed") return SortDirsType::Mixed;
  return SortDirsType::First;
}

QString fileSizeFormatToString(FileSizeFormat fmt) {
  switch (fmt) {
    case FileSizeFormat::Bytes: return "bytes";
    case FileSizeFormat::SI: return "si";
    case FileSizeFormat::IEC: return "iec";
    case FileSizeFormat::Auto: return "auto";
  }
  return "auto";
}

FileSizeFormat stringToFileSizeFormat(const QString& str) {
  if (str == "bytes") return FileSizeFormat::Bytes;
  if (str == "si") return FileSizeFormat::SI;
  if (str == "iec") return FileSizeFormat::IEC;
  return FileSizeFormat::Auto;
}

QString windowSizeModeToString(WindowSizeMode mode) {
  switch (mode) {
    case WindowSizeMode::Default: return "default";
    case WindowSizeMode::LastSession: return "lastSession";
    case WindowSizeMode::Custom: return "custom";
  }
  return "default";
}

WindowSizeMode stringToWindowSizeMode(const QString& str) {
  if (str == "lastSession") return WindowSizeMode::LastSession;
  if (str == "custom") return WindowSizeMode::Custom;
  return WindowSizeMode::Default;
}

QString windowPositionModeToString(WindowPositionMode mode) {
  switch (mode) {
    case WindowPositionMode::Default: return "default";
    case WindowPositionMode::LastSession: return "lastSession";
    case WindowPositionMode::Custom: return "custom";
  }
  return "default";
}

WindowPositionMode stringToWindowPositionMode(const QString& str) {
  if (str == "lastSession") return WindowPositionMode::LastSession;
  if (str == "custom") return WindowPositionMode::Custom;
  return WindowPositionMode::Default;
}

QString initialPathModeToString(InitialPathMode mode) {
  switch (mode) {
    case InitialPathMode::Default:     return "default";
    case InitialPathMode::LastSession: return "lastSession";
    case InitialPathMode::Custom:      return "custom";
  }
  return "lastSession";
}

InitialPathMode stringToInitialPathMode(const QString& str) {
  if (str == "default") return InitialPathMode::Default;
  if (str == "custom")  return InitialPathMode::Custom;
  return InitialPathMode::LastSession;
}

QJsonObject paneSettingsToJson(const PaneSettings& pane) {
  QJsonObject obj;
  obj["path"] = pane.path;
  obj["sortKey"] = sortKeyToString(pane.sortKey);
  obj["sortOrder"] = (pane.sortOrder == Qt::AscendingOrder) ? "ascending" : "descending";
  obj["sortKey2nd"] = sortKeyToString(pane.sortKey2nd);
  obj["sortDirsType"] = sortDirsTypeToString(pane.sortDirsType);
  obj["sortDotFirst"] = pane.sortDotFirst;
  obj["sortCaseSensitive"] = (pane.sortCS == Qt::CaseSensitive);

  QJsonArray filters;
  for (const QString& filter : pane.nameFilters) {
    filters.append(filter);
  }
  obj["nameFilters"] = filters;

  obj["showHidden"] = static_cast<bool>(pane.attrFilter & AttrFilter::ShowHidden);
  obj["showSystem"] = static_cast<bool>(pane.attrFilter & AttrFilter::ShowSystem);
  obj["dirsOnly"] = static_cast<bool>(pane.attrFilter & AttrFilter::DirsOnly);
  obj["filesOnly"] = static_cast<bool>(pane.attrFilter & AttrFilter::FilesOnly);

  return obj;
}

PaneSettings jsonToPaneSettings(const QJsonObject& obj) {
  PaneSettings pane;
  pane.path = obj.value("path").toString(QDir::homePath());
  pane.sortKey = stringToSortKey(obj.value("sortKey").toString());
  pane.sortOrder = (obj.value("sortOrder").toString() == "descending") ?
                   Qt::DescendingOrder : Qt::AscendingOrder;
  pane.sortKey2nd = stringToSortKey(obj.value("sortKey2nd").toString());
  pane.sortDirsType = stringToSortDirsType(obj.value("sortDirsType").toString());
  pane.sortDotFirst = obj.value("sortDotFirst").toBool(true);
  pane.sortCS = obj.value("sortCaseSensitive").toBool(false) ?
                Qt::CaseSensitive : Qt::CaseInsensitive;

  QJsonArray filters = obj.value("nameFilters").toArray();
  for (const QJsonValue& val : filters) {
    pane.nameFilters.append(val.toString());
  }

  AttrFilterFlags flags = AttrFilter::None;
  if (obj.value("showHidden").toBool(false))
    flags |= AttrFilter::ShowHidden;
  if (obj.value("showSystem").toBool(false))
    flags |= AttrFilter::ShowSystem;
  if (obj.value("dirsOnly").toBool(false))
    flags |= AttrFilter::DirsOnly;
  if (obj.value("filesOnly").toBool(false))
    flags |= AttrFilter::FilesOnly;
  pane.attrFilter = flags;

  return pane;
}

QString fileCategoryToString(FileCategory cat) {
  switch (cat) {
    case FileCategory::Normal:    return "normal";
    case FileCategory::Hidden:    return "hidden";
    case FileCategory::Directory: return "directory";
    case FileCategory::Count:     break;
  }
  return "normal";
}

QJsonObject categoryColorToJson(const CategoryColor& c) {
  QJsonObject obj;
  obj["foreground"] = c.foreground.isValid() ? c.foreground.name(QColor::HexArgb) : QString();
  obj["background"] = c.background.isValid() ? c.background.name(QColor::HexArgb) : QString();
  obj["bold"]       = c.bold;
  return obj;
}

CategoryColor jsonToCategoryColor(const QJsonObject& obj, const CategoryColor& fallback) {
  CategoryColor c = fallback;
  const QString fg = obj.value("foreground").toString();
  const QString bg = obj.value("background").toString();
  if (!fg.isEmpty()) c.foreground = QColor(fg);
  if (!bg.isEmpty()) c.background = QColor(bg);
  if (obj.contains("bold")) c.bold = obj.value("bold").toBool();
  return c;
}

QJsonObject colorRuleToJson(const ColorRule& rule) {
  QJsonObject obj;
  obj["pattern"] = rule.pattern;
  obj["foreground"] = rule.foreground.name(QColor::HexArgb);
  obj["background"] = rule.background.name(QColor::HexArgb);
  obj["bold"] = rule.bold;
  return obj;
}

ColorRule jsonToColorRule(const QJsonObject& obj) {
  ColorRule rule;
  rule.pattern = obj.value("pattern").toString();
  rule.foreground = QColor(obj.value("foreground").toString());
  rule.background = QColor(obj.value("background").toString());
  rule.bold = obj.value("bold").toBool(false);
  return rule;
}

} // anonymous namespace

void Settings::load() {
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QString filePath = configPath + "/settings.json";

  QFile file(filePath);
  if (!file.exists()) {
    qDebug() << "Settings::load: settings file not found, using defaults:" << filePath;
    // 完全に初回起動なのでデフォルトブックマークを注入して完了とする。
    m_bookmarks = buildDefaultBookmarks();
    m_defaultBookmarksInstalled = true;
    return;
  }

  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Settings::load: failed to open settings file:" << filePath;
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) {
    qWarning() << "Settings::load: invalid JSON format";
    return;
  }

  QJsonObject root = doc.object();
  const int fileVersion = root.value("version").toInt(1);

  // Load appearance settings
  QJsonObject appearance = root.value("appearance").toObject();
  if (appearance.contains("font")) {
    QJsonObject fontObj = appearance.value("font").toObject();
    m_font.setFamily(fontObj.value("family").toString());
    if (fontObj.contains("pointSize")) {
      m_font.setPointSize(fontObj.value("pointSize").toInt());
    }
  }

  m_fileSizeFormat = stringToFileSizeFormat(appearance.value("fileSizeFormat").toString());
  m_dateTimeFormat = appearance.value("dateTimeFormat").toString("yyyy/MM/dd HH:mm:ss");

  // Load color rules
  QJsonArray colorRulesArray = appearance.value("colorRules").toArray();
  m_colorRules.clear();
  for (const QJsonValue& val : colorRulesArray) {
    if (val.isObject()) {
      m_colorRules.append(jsonToColorRule(val.toObject()));
    }
  }

  // Load category colors:
  //   新形式:  { active: { normal:{}, selected:{} }, inactive: { normal:{}, selected:{} } }
  //   中期形式: { normal:{}, selected:{} }                        ← active として扱う
  //   旧形式:  { <category>:{} ... }                               ← active/normal として扱う
  QJsonObject catColors = appearance.value("categoryColors").toObject();

  auto extractGroup = [](const QJsonObject& root, const QString& key) {
    return root.contains(key) && root.value(key).isObject()
             ? root.value(key).toObject()
             : QJsonObject();
  };

  QJsonObject activeNormal;
  QJsonObject activeSelected;
  QJsonObject inactiveNormal;
  QJsonObject inactiveSel;

  if (catColors.contains("active") && catColors.value("active").isObject()) {
    // 新形式: categoryColors: { active: {...}, inactive: {...} }
    QJsonObject active   = catColors.value("active").toObject();
    QJsonObject inactive = extractGroup(catColors, "inactive");
    activeNormal   = extractGroup(active,   "normal");
    activeSelected = extractGroup(active,   "selected");
    inactiveNormal = extractGroup(inactive, "normal");
    inactiveSel    = extractGroup(inactive, "selected");
  } else if (catColors.value("normal").toObject().contains("foreground")) {
    // 最古のフラット形式: categoryColors: { normal: {fg/bg/bold}, hidden: {...}, directory: {...} }
    activeNormal = catColors;
  } else {
    // 中期形式: categoryColors: { normal: {<category>:{...}}, selected: {<category>:{...}} }
    activeNormal   = extractGroup(catColors, "normal");
    activeSelected = extractGroup(catColors, "selected");
  }

  auto loadCat = [&](FileCategory cat) {
    int idx = static_cast<int>(cat);
    const QString key = fileCategoryToString(cat);
    if (activeNormal.contains(key)) {
      m_categoryColors[idx] = jsonToCategoryColor(
        activeNormal.value(key).toObject(), m_categoryColors[idx]);
    }
    if (activeSelected.contains(key)) {
      m_selectedCategoryColors[idx] = jsonToCategoryColor(
        activeSelected.value(key).toObject(), m_selectedCategoryColors[idx]);
    }
    if (inactiveNormal.contains(key)) {
      m_inactiveCategoryColors[idx] = jsonToCategoryColor(
        inactiveNormal.value(key).toObject(), m_inactiveCategoryColors[idx]);
    }
    if (inactiveSel.contains(key)) {
      m_inactiveSelectedCategoryColors[idx] = jsonToCategoryColor(
        inactiveSel.value(key).toObject(), m_inactiveSelectedCategoryColors[idx]);
    }
  };
  loadCat(FileCategory::Normal);
  loadCat(FileCategory::Hidden);
  loadCat(FileCategory::Directory);

  m_useInactivePaneColors =
    appearance.value("useInactivePaneColors").toBool(false);

  // Path label colors
  QJsonObject pathColors = appearance.value("pathColors").toObject();
  if (pathColors.contains("foreground")) {
    QColor fg(pathColors.value("foreground").toString());
    if (fg.isValid()) m_pathForeground = fg;
  }
  if (pathColors.contains("background")) {
    QColor bg(pathColors.value("background").toString());
    if (bg.isValid()) m_pathBackground = bg;
  }

  // Cursor colors
  QJsonObject cursorColors = appearance.value("cursorColors").toObject();
  if (cursorColors.contains("active")) {
    QColor a(cursorColors.value("active").toString());
    if (a.isValid()) m_cursorActiveColor = a;
  }
  if (cursorColors.contains("inactive")) {
    QColor i(cursorColors.value("inactive").toString());
    if (i.isValid()) m_cursorInactiveColor = i;
  }

  // Load behavior settings
  QJsonObject behavior = root.value("behavior").toObject();
  m_confirmOnExit = behavior.value("confirmOnExit").toBool(false);
  m_cursorLoop = behavior.value("cursorLoop").toBool(false);
  m_persistHistory = behavior.value("persistHistory").toBool(false);
  m_autoRenameTemplate = behavior.value("autoRenameTemplate").toString(" ({n})");
  m_defaultDeleteToTrash = behavior.value("defaultDeleteToTrash").toBool(true);
  m_defaultBookmarksInstalled = behavior.value("defaultBookmarksInstalled").toBool(false);

  // ペイン履歴（ON の時のみ読む。OFF の時は必ず空にする）
  for (int i = 0; i < static_cast<int>(PaneType::Count); ++i) {
    m_paneHistory[i].clear();
  }
  if (m_persistHistory) {
    QJsonObject hist = root.value("paneHistory").toObject();
    auto load = [&](PaneType pane, const QString& key) {
      int idx = static_cast<int>(pane);
      QJsonArray arr = hist.value(key).toArray();
      for (const QJsonValue& v : arr) {
        const QString s = v.toString();
        if (!s.isEmpty()) m_paneHistory[idx].append(s);
      }
    };
    load(PaneType::Left,  "left");
    load(PaneType::Right, "right");
  }

  // Per-pane 初期表示ディレクトリ。旧 restoreLastPath が残っていれば
  // LastSession/Default にマップして互換性を保つ。
  const bool legacyRestore = behavior.value("restoreLastPath").toBool(true);
  const InitialPathMode legacyMode = legacyRestore ? InitialPathMode::LastSession
                                                   : InitialPathMode::Default;

  QJsonObject initialPaths = root.value("initialPaths").toObject();
  auto loadInitialPath = [&](PaneType pane, const QString& key) {
    int idx = static_cast<int>(pane);
    if (initialPaths.contains(key)) {
      QJsonObject obj = initialPaths.value(key).toObject();
      m_initialPathMode[idx]   = stringToInitialPathMode(obj.value("mode").toString());
      m_customInitialPath[idx] = obj.value("customPath").toString();
    } else {
      m_initialPathMode[idx]   = legacyMode;
      m_customInitialPath[idx].clear();
    }
  };
  loadInitialPath(PaneType::Left,  "left");
  loadInitialPath(PaneType::Right, "right");

  // Load window settings
  QJsonObject window = root.value("window").toObject();
  m_windowSizeMode = stringToWindowSizeMode(window.value("sizeMode").toString());
  if (window.contains("customSize")) {
    QJsonObject customSize = window.value("customSize").toObject();
    m_customWindowSize = QSize(
      customSize.value("width").toInt(1200),
      customSize.value("height").toInt(600)
    );
  }
  if (window.contains("lastSize")) {
    QJsonObject lastSize = window.value("lastSize").toObject();
    m_lastWindowSize = QSize(
      lastSize.value("width").toInt(1200),
      lastSize.value("height").toInt(600)
    );
  }

  m_windowPositionMode = stringToWindowPositionMode(window.value("positionMode").toString());
  if (window.contains("customPosition")) {
    QJsonObject customPos = window.value("customPosition").toObject();
    m_customWindowPosition = QPoint(
      customPos.value("x").toInt(0),
      customPos.value("y").toInt(0)
    );
  }
  if (window.contains("lastPosition")) {
    QJsonObject lastPos = window.value("lastPosition").toObject();
    m_lastWindowPosition = QPoint(
      lastPos.value("x").toInt(0),
      lastPos.value("y").toInt(0)
    );
  }

  // Load pane settings
  QJsonObject panes = root.value("panes").toObject();
  if (panes.contains("left")) {
    m_paneSettings[static_cast<int>(PaneType::Left)] =
      jsonToPaneSettings(panes.value("left").toObject());
  }
  if (panes.contains("right")) {
    m_paneSettings[static_cast<int>(PaneType::Right)] =
      jsonToPaneSettings(panes.value("right").toObject());
  }

  // Load per-path overrides
  m_pathOverrides.clear();
  QJsonObject overrides = root.value("pathOverrides").toObject();
  for (auto it = overrides.begin(); it != overrides.end(); ++it) {
    if (it.value().isObject()) {
      m_pathOverrides.insert(it.key(), jsonToPaneSettings(it.value().toObject()));
    }
  }

  // Load bookmarks
  m_bookmarks.clear();
  const QList<Bookmark> defaults = buildDefaultBookmarks();
  QJsonArray bmArr = root.value("bookmarks").toArray();
  for (const QJsonValue& val : bmArr) {
    if (!val.isObject()) continue;
    QJsonObject obj = val.toObject();
    Bookmark b;
    b.name = obj.value("name").toString();
    b.path = obj.value("path").toString();
    if (obj.contains("isDefault")) {
      b.isDefault = obj.value("isDefault").toBool(false);
    } else {
      // 旧フォーマット: isDefault フィールドなし。現行デフォルトリストに
      // パスが含まれていれば、デフォルトとして扱う（削除不可に昇格）。
      for (const Bookmark& d : defaults) {
        if (d.path == b.path) { b.isDefault = true; break; }
      }
    }
    if (!b.path.isEmpty()) m_bookmarks.append(b);
  }

  // version < 2: 旧 Farman で Root ("/") が自動注入されたことがあるため除去。
  // ユーザーが明示的に追加した可能性もあるが、現行仕様では Root をデフォルトに
  // 含めないため、一律撤去する（必要なら再度手動追加できる）。
  if (fileVersion < 2) {
    for (int i = m_bookmarks.size() - 1; i >= 0; --i) {
      if (m_bookmarks[i].path == QStringLiteral("/")) {
        m_bookmarks.removeAt(i);
      }
    }
  }

  // 初回のみ: デフォルトブックマークを既存リストにマージ（重複パスはスキップ）。
  if (!m_defaultBookmarksInstalled) {
    for (const Bookmark& d : defaults) {
      bool dup = false;
      for (const Bookmark& b : m_bookmarks) {
        if (b.path == d.path) { dup = true; break; }
      }
      if (!dup) m_bookmarks.append(d);
    }
    m_defaultBookmarksInstalled = true;
  }

  qDebug() << "Settings::load: loaded settings from" << filePath;
  emit settingsChanged();
}

void Settings::save() const {
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QDir dir;
  if (!dir.exists(configPath)) {
    if (!dir.mkpath(configPath)) {
      qWarning() << "Settings::save: failed to create config directory:" << configPath;
      return;
    }
  }

  QString filePath = configPath + "/settings.json";

  QJsonObject root;
  root["version"] = 2;

  // Save appearance settings
  QJsonObject appearance;
  QJsonObject fontObj;
  fontObj["family"] = m_font.family();
  fontObj["pointSize"] = m_font.pointSize();
  appearance["font"] = fontObj;
  appearance["fileSizeFormat"] = fileSizeFormatToString(m_fileSizeFormat);
  appearance["dateTimeFormat"] = m_dateTimeFormat;

  QJsonArray colorRulesArray;
  for (const ColorRule& rule : m_colorRules) {
    colorRulesArray.append(colorRuleToJson(rule));
  }
  appearance["colorRules"] = colorRulesArray;

  auto catStateToJson = [](const CategoryColor colors[]) {
    QJsonObject obj;
    for (int i = 0; i < static_cast<int>(FileCategory::Count); ++i) {
      const QString key = fileCategoryToString(static_cast<FileCategory>(i));
      obj[key] = categoryColorToJson(colors[i]);
    }
    return obj;
  };
  QJsonObject active;
  active["normal"]   = catStateToJson(m_categoryColors);
  active["selected"] = catStateToJson(m_selectedCategoryColors);
  QJsonObject inactive;
  inactive["normal"]   = catStateToJson(m_inactiveCategoryColors);
  inactive["selected"] = catStateToJson(m_inactiveSelectedCategoryColors);
  QJsonObject catColors;
  catColors["active"]   = active;
  catColors["inactive"] = inactive;
  appearance["categoryColors"]      = catColors;
  appearance["useInactivePaneColors"] = m_useInactivePaneColors;

  QJsonObject pathColors;
  pathColors["foreground"] = m_pathForeground.name(QColor::HexArgb);
  pathColors["background"] = m_pathBackground.name(QColor::HexArgb);
  appearance["pathColors"] = pathColors;

  QJsonObject cursorColorsJson;
  cursorColorsJson["active"]   = m_cursorActiveColor.name(QColor::HexArgb);
  cursorColorsJson["inactive"] = m_cursorInactiveColor.name(QColor::HexArgb);
  appearance["cursorColors"] = cursorColorsJson;

  root["appearance"] = appearance;

  // Save behavior settings
  QJsonObject behavior;
  behavior["confirmOnExit"] = m_confirmOnExit;
  behavior["cursorLoop"] = m_cursorLoop;
  behavior["persistHistory"] = m_persistHistory;
  behavior["autoRenameTemplate"] = m_autoRenameTemplate;
  behavior["defaultDeleteToTrash"] = m_defaultDeleteToTrash;
  behavior["defaultBookmarksInstalled"] = m_defaultBookmarksInstalled;
  root["behavior"] = behavior;

  // ペイン履歴（ON のときだけディスクに出す）
  if (m_persistHistory) {
    QJsonObject hist;
    auto toArr = [](const QStringList& list) {
      QJsonArray arr;
      for (const QString& s : list) arr.append(s);
      return arr;
    };
    hist["left"]  = toArr(m_paneHistory[static_cast<int>(PaneType::Left)]);
    hist["right"] = toArr(m_paneHistory[static_cast<int>(PaneType::Right)]);
    root["paneHistory"] = hist;
  }

  // Per-pane 初期表示ディレクトリ
  QJsonObject initialPaths;
  auto paneInitial = [this](PaneType pane) -> QJsonObject {
    int idx = static_cast<int>(pane);
    QJsonObject obj;
    obj["mode"]       = initialPathModeToString(m_initialPathMode[idx]);
    obj["customPath"] = m_customInitialPath[idx];
    return obj;
  };
  initialPaths["left"]  = paneInitial(PaneType::Left);
  initialPaths["right"] = paneInitial(PaneType::Right);
  root["initialPaths"]  = initialPaths;

  // Save window settings
  QJsonObject window;
  window["sizeMode"] = windowSizeModeToString(m_windowSizeMode);

  QJsonObject customSize;
  customSize["width"] = m_customWindowSize.width();
  customSize["height"] = m_customWindowSize.height();
  window["customSize"] = customSize;

  QJsonObject lastSize;
  lastSize["width"] = m_lastWindowSize.width();
  lastSize["height"] = m_lastWindowSize.height();
  window["lastSize"] = lastSize;

  window["positionMode"] = windowPositionModeToString(m_windowPositionMode);

  QJsonObject customPos;
  customPos["x"] = m_customWindowPosition.x();
  customPos["y"] = m_customWindowPosition.y();
  window["customPosition"] = customPos;

  QJsonObject lastPos;
  lastPos["x"] = m_lastWindowPosition.x();
  lastPos["y"] = m_lastWindowPosition.y();
  window["lastPosition"] = lastPos;

  root["window"] = window;

  // Save pane settings
  QJsonObject panes;
  panes["left"] = paneSettingsToJson(m_paneSettings[static_cast<int>(PaneType::Left)]);
  panes["right"] = paneSettingsToJson(m_paneSettings[static_cast<int>(PaneType::Right)]);
  root["panes"] = panes;

  // Save per-path overrides
  QJsonObject overrides;
  for (auto it = m_pathOverrides.begin(); it != m_pathOverrides.end(); ++it) {
    overrides[it.key()] = paneSettingsToJson(it.value());
  }
  root["pathOverrides"] = overrides;

  // Save bookmarks
  QJsonArray bmArr;
  for (const Bookmark& b : m_bookmarks) {
    QJsonObject obj;
    obj["name"] = b.name;
    obj["path"] = b.path;
    if (b.isDefault) obj["isDefault"] = true;
    bmArr.append(obj);
  }
  root["bookmarks"] = bmArr;

  QJsonDocument doc(root);
  QByteArray data = doc.toJson(QJsonDocument::Indented);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    qWarning() << "Settings::save: failed to open settings file for writing:" << filePath;
    return;
  }

  file.write(data);
  file.close();

  qDebug() << "Settings::save: saved settings to" << filePath;
}

} // namespace Farman

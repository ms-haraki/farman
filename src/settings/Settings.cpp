#include "Settings.h"
#include <QStandardPaths>
#include <QDir>
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

bool Settings::restoreLastPath() const {
  return m_restoreLastPath;
}

void Settings::setRestoreLastPath(bool restore) {
  m_restoreLastPath = restore;
}

bool Settings::confirmOnExit() const {
  return m_confirmOnExit;
}

void Settings::setConfirmOnExit(bool confirm) {
  m_confirmOnExit = confirm;
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

  // Load behavior settings
  QJsonObject behavior = root.value("behavior").toObject();
  m_restoreLastPath = behavior.value("restoreLastPath").toBool(true);
  m_confirmOnExit = behavior.value("confirmOnExit").toBool(false);

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
  root["version"] = 1;

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

  root["appearance"] = appearance;

  // Save behavior settings
  QJsonObject behavior;
  behavior["restoreLastPath"] = m_restoreLastPath;
  behavior["confirmOnExit"] = m_confirmOnExit;
  root["behavior"] = behavior;

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

#include "Settings.h"

namespace Farman {

Settings& Settings::instance() {
  static Settings instance;
  return instance;
}

Settings::Settings(QObject* parent) : QObject(parent) {
}

PaneSettings Settings::paneSettings(PaneType pane) const {
  // TODO: 実装
  return {};
}

void Settings::setPaneSettings(PaneType pane, const PaneSettings& s) {
  // TODO: 実装
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

void Settings::load() {
  // TODO: 実装
}

void Settings::save() const {
  // TODO: 実装
}

} // namespace Farman

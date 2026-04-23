#include "DirectoryHistory.h"

namespace Farman {

void DirectoryHistory::push(const QString& path) {
  if (path.isEmpty()) return;

  const int existing = m_entries.indexOf(path);
  if (existing == 0) return;  // 既に先頭（= 直前と同じパス）なら何もしない
  if (existing > 0) m_entries.removeAt(existing);

  m_entries.prepend(path);

  while (m_entries.size() > kMaxEntries) {
    m_entries.removeLast();
  }
}

void DirectoryHistory::setEntries(const QStringList& entries) {
  m_entries = entries;
  while (m_entries.size() > kMaxEntries) {
    m_entries.removeLast();
  }
}

void DirectoryHistory::clear() {
  m_entries.clear();
}

} // namespace Farman

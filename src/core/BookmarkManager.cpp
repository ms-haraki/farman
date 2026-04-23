#include "BookmarkManager.h"
#include "settings/Settings.h"

namespace Farman {

BookmarkManager& BookmarkManager::instance() {
  static BookmarkManager s;
  return s;
}

BookmarkManager::BookmarkManager(QObject* parent) : QObject(parent) {}

QList<Bookmark> BookmarkManager::bookmarks() const {
  return Settings::instance().bookmarks();
}

void BookmarkManager::setBookmarks(const QList<Bookmark>& list) {
  Settings::instance().setBookmarks(list);
  Settings::instance().save();
  emit bookmarksChanged();
}

void BookmarkManager::add(const QString& name, const QString& path) {
  if (path.isEmpty()) return;
  QList<Bookmark> list = bookmarks();
  list.append({name, path});
  setBookmarks(list);
}

void BookmarkManager::removeAt(int index) {
  QList<Bookmark> list = bookmarks();
  if (index < 0 || index >= list.size()) return;
  if (list[index].isDefault) return;  // デフォルトブックマークは削除不可
  list.removeAt(index);
  setBookmarks(list);
}

void BookmarkManager::rename(int index, const QString& newName) {
  QList<Bookmark> list = bookmarks();
  if (index < 0 || index >= list.size()) return;
  list[index].name = newName;
  setBookmarks(list);
}

void BookmarkManager::move(int from, int to) {
  QList<Bookmark> list = bookmarks();
  if (from < 0 || from >= list.size()) return;
  if (to   < 0 || to   >= list.size()) return;
  if (from == to) return;
  list.move(from, to);
  setBookmarks(list);
}

bool BookmarkManager::contains(const QString& path) const {
  return findByPath(path) >= 0;
}

int BookmarkManager::findByPath(const QString& path) const {
  const QList<Bookmark> list = bookmarks();
  for (int i = 0; i < list.size(); ++i) {
    if (list[i].path == path) return i;
  }
  return -1;
}

} // namespace Farman

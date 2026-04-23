#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "settings/Settings.h"

namespace Farman {

// ブックマーク（お気に入りディレクトリ）の管理。
// - 実データは Settings (settings.json) に集約して永続化する。
// - CRUD のいずれかで変更が発生したら直ちに Settings::save() を呼び、
//   bookmarksChanged() を発行する。
class BookmarkManager : public QObject {
  Q_OBJECT

public:
  static BookmarkManager& instance();

  QList<Bookmark> bookmarks() const;
  void            setBookmarks(const QList<Bookmark>& list);

  void add(const QString& name, const QString& path);
  void removeAt(int index);
  void rename(int index, const QString& newName);
  // from の要素を to の位置に移動（QList::move 準拠）
  void move(int from, int to);

  bool contains(const QString& path) const;
  int  findByPath(const QString& path) const;

signals:
  void bookmarksChanged();

private:
  explicit BookmarkManager(QObject* parent = nullptr);
};

} // namespace Farman

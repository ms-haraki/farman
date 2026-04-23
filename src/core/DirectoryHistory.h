#pragma once

#include <QString>
#include <QStringList>

namespace Farman {

// 最近訪問したディレクトリを最新順で保持する。
// - push() で同じパスが既にあれば既存を削除して先頭に移す（=最新訪問）。
// - kMaxEntries を超えたら末尾から切り捨てる。
// - 戻る/進む スタックではない。履歴ウィンドウから選択して遷移する想定。
class DirectoryHistory {
public:
  static constexpr int kMaxEntries = 10;

  void push(const QString& path);

  QStringList entries() const { return m_entries; }
  bool        isEmpty() const { return m_entries.isEmpty(); }

  // 永続化された履歴からの復元用
  void setEntries(const QStringList& entries);
  void clear();

private:
  QStringList m_entries;
};

} // namespace Farman

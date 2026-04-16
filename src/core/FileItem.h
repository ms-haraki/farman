#pragma once

#include "types.h"
#include <QFileInfo>
#include <QString>
#include <QDateTime>

namespace Farman {

class FileItem {
public:
  explicit FileItem(const QFileInfo& info);

  // 基本情報
  QString   name()         const;
  QString   absolutePath() const;
  QString   suffix()       const;   // 拡張子（小文字）
  QString   mimeType()     const;   // 遅延取得
  qint64    size()         const;   // ディレクトリは -1
  QDateTime lastModified() const;

  // 種別
  bool isDir()     const;
  bool isFile()    const;
  bool isHidden()  const;
  bool isSymLink() const;
  bool isDotDot()  const;   // ".." エントリ

  // 選択状態
  bool isSelected() const;
  void setSelected(bool selected);

  // 生の QFileInfo が必要な場合
  const QFileInfo& fileInfo() const;

private:
  QFileInfo m_info;
  bool      m_selected = false;
  mutable QString m_mimeType;  // 遅延初期化
};

} // namespace Farman

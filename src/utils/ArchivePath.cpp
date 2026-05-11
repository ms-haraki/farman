#include "ArchivePath.h"
#include <QStringList>

namespace Farman::ArchivePath {

namespace {

// アーカイブとして扱う拡張子のリスト (大文字小文字無視)。
// 順序は長いもの優先 (.tar.gz が .tar より先にマッチしてほしい)。
const QStringList& archiveExtensions() {
  static const QStringList exts = {
    QStringLiteral(".tar.gz"), QStringLiteral(".tar.bz2"), QStringLiteral(".tar.xz"),
    QStringLiteral(".tgz"),    QStringLiteral(".tbz2"),    QStringLiteral(".txz"),
    QStringLiteral(".tar"),    QStringLiteral(".zip"),
  };
  return exts;
}

} // namespace

bool isArchiveExtension(const QString& path) {
  for (const QString& e : archiveExtensions()) {
    if (path.endsWith(e, Qt::CaseInsensitive)) return true;
  }
  return false;
}

Split splitArchivePath(const QString& path) {
  Split s;
  const int sep = path.indexOf(QLatin1Char('!'));
  if (sep < 0) {
    s.archivePath = path;
    return s;  // valid=false
  }
  const QString archive = path.left(sep);
  if (!isArchiveExtension(archive)) {
    s.archivePath = path;
    return s;  // valid=false (`!` を含むが拡張子が違うので通常パス扱い)
  }
  QString inner = path.mid(sep + 1);
  if (inner.isEmpty() || !inner.startsWith(QLatin1Char('/'))) {
    inner = QStringLiteral("/") + inner;
  }
  // 末尾の '/' を削る (ルート "/" は維持)
  while (inner.size() > 1 && inner.endsWith(QLatin1Char('/'))) {
    inner.chop(1);
  }
  s.archivePath = archive;
  s.innerPath   = inner;
  s.valid       = true;
  return s;
}

QString joinArchivePath(const QString& archivePath, const QString& innerPath) {
  QString inner = innerPath;
  if (inner.isEmpty() || !inner.startsWith(QLatin1Char('/'))) {
    inner = QStringLiteral("/") + inner;
  }
  return archivePath + QLatin1Char('!') + inner;
}

QString parentInnerPath(const QString& innerPath) {
  if (innerPath.isEmpty() || innerPath == QStringLiteral("/")) {
    return QStringLiteral("/");
  }
  const int slash = innerPath.lastIndexOf(QLatin1Char('/'));
  if (slash <= 0) return QStringLiteral("/");
  return innerPath.left(slash);
}

QString innerBaseName(const QString& innerPath) {
  if (innerPath.isEmpty() || innerPath == QStringLiteral("/")) return {};
  const int slash = innerPath.lastIndexOf(QLatin1Char('/'));
  return innerPath.mid(slash + 1);
}

} // namespace Farman::ArchivePath

#include "ArchivePath.h"
#include <QDir>
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

QString safeJoinExtractPath(const QString& outputDir, const QString& entryName) {
  if (entryName.isEmpty() || outputDir.isEmpty()) return {};

  // Windows のバックスラッシュも内部表現の '/' に揃えてから判定する。
  QString rel = QDir::fromNativeSeparators(entryName);

  // 絶対パス (Unix `/...`、Windows `C:\...` / `\\server\...`) は拒否。
  if (QDir::isAbsolutePath(rel)) return {};

  // 分解して `..` / `.` セグメント、NUL 文字、空セグメントを弾く。
  // 通常エントリは `foo/bar/baz` のような清潔な形だが、悪意あるアーカイブは
  // `../etc/passwd` や `foo/../../../../etc/passwd` を含む。
  const QStringList parts = rel.split(QLatin1Char('/'), Qt::KeepEmptyParts);
  QStringList clean;
  clean.reserve(parts.size());
  for (const QString& part : parts) {
    if (part.isEmpty()) return {};                       // 連続する '/' は不正
    if (part == QStringLiteral("."))  return {};
    if (part == QStringLiteral("..")) return {};
    if (part.contains(QChar(0)))      return {};         // NUL 注入を弾く
    clean.append(part);
  }
  if (clean.isEmpty()) return {};

  const QString outClean = QDir::cleanPath(outputDir);
  const QString joined   = outClean + QLatin1Char('/') + clean.join(QLatin1Char('/'));
  const QString resolved = QDir::cleanPath(joined);

  // cleanPath 後にも outputDir 配下に収まっていることを念のため確認する
  // (Windows の大文字小文字差や、ヘンなセパレータが混入した場合の防御線)。
  if (resolved != outClean
      && !resolved.startsWith(outClean + QLatin1Char('/'))) {
    return {};
  }
  return resolved;
}

} // namespace Farman::ArchivePath

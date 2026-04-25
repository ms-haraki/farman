#include "BinaryView.h"

#include <QFile>
#include <QFontDatabase>
#include <QString>

namespace Farman {

namespace {

QString formatHexDump(const QByteArray& data, qint64 baseOffset = 0) {
  constexpr int bytesPerLine = 16;
  const int lineCount = (data.size() + bytesPerLine - 1) / bytesPerLine;

  QString out;
  // 1 行あたり: 8 (addr) + 2 (sp) + 16*3 (hex+sp) + 1 (mid extra sp) + 16 (ascii) + 1 (\n) ≈ 76
  out.reserve(lineCount * 80);

  static const char hex[] = "0123456789abcdef";

  for (int line = 0; line < lineCount; ++line) {
    const int off = line * bytesPerLine;
    const qint64 addr = baseOffset + off;

    // address: 8 桁 16 進
    char addrBuf[9];
    for (int i = 7; i >= 0; --i) {
      addrBuf[i] = hex[(addr >> ((7 - i) * 4)) & 0xF];
    }
    addrBuf[8] = '\0';
    out.append(QLatin1String(addrBuf, 8));
    out.append(QLatin1String("  "));

    // hex columns
    for (int i = 0; i < bytesPerLine; ++i) {
      if (i == 8) {
        out.append(QLatin1Char(' '));
      }
      const int idx = off + i;
      if (idx < data.size()) {
        const unsigned char b = static_cast<unsigned char>(data[idx]);
        out.append(QLatin1Char(hex[b >> 4]));
        out.append(QLatin1Char(hex[b & 0xF]));
      } else {
        out.append(QLatin1String("  "));
      }
      out.append(QLatin1Char(' '));
    }

    out.append(QLatin1Char(' '));

    // ASCII column
    for (int i = 0; i < bytesPerLine; ++i) {
      const int idx = off + i;
      if (idx < data.size()) {
        const unsigned char b = static_cast<unsigned char>(data[idx]);
        out.append(QChar((b >= 0x20 && b <= 0x7E) ? static_cast<char>(b) : '.'));
      } else {
        out.append(QLatin1Char(' '));
      }
    }

    out.append(QLatin1Char('\n'));
  }

  return out;
}

} // namespace

BinaryView::BinaryView(QWidget* parent)
  : QPlainTextEdit(parent)
{
  setReadOnly(true);
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

bool BinaryView::loadFile(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  const qint64 totalSize = file.size();
  const qint64 readSize = qMin<qint64>(totalSize, kMaxBytes);
  const QByteArray data = file.read(readSize);
  file.close();

  QString text = formatHexDump(data);
  if (totalSize > readSize) {
    text.append(QStringLiteral("...\n[truncated: showing first %1 of %2 bytes]\n")
                  .arg(readSize)
                  .arg(totalSize));
  }

  setPlainText(text);
  moveCursor(QTextCursor::Start);
  return true;
}

} // namespace Farman

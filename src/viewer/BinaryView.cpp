#include "BinaryView.h"
#include "settings/Settings.h"

#include <QComboBox>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLocale>
#include <QLabel>
#include <QPalette>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStringDecoder>
#include <QSyntaxHighlighter>
#include <QTextCodec>
#include <QVBoxLayout>

namespace Farman {

// 各行先頭 8 桁の 16 進アドレスをハイライトする。色は Settings から都度取得する。
class AddressHighlighter : public QSyntaxHighlighter {
public:
  using QSyntaxHighlighter::QSyntaxHighlighter;

protected:
  void highlightBlock(const QString& text) override {
    static const QRegularExpression re(QStringLiteral("^[0-9a-fA-F]{8}"));
    const auto m = re.match(text);
    if (!m.hasMatch()) return;
    const Settings& s = Settings::instance();
    QTextCharFormat fmt;
    fmt.setForeground(s.binaryViewerAddressForeground());
    if (s.binaryViewerAddressBackground().isValid()) {
      fmt.setBackground(s.binaryViewerAddressBackground());
    }
    setFormat(m.capturedStart(), m.capturedLength(), fmt);
  }
};

namespace {

constexpr int kBytesPerLine = 16;
constexpr char kHexDigits[] = "0123456789abcdef";

void appendUnitHex(QString& out, const unsigned char* valueBytes, int unitBytes,
                   BinaryViewerEndian endian) {
  if (endian == BinaryViewerEndian::Big) {
    for (int i = 0; i < unitBytes; ++i) {
      out.append(QLatin1Char(kHexDigits[valueBytes[i] >> 4]));
      out.append(QLatin1Char(kHexDigits[valueBytes[i] & 0xF]));
    }
  } else {
    for (int i = unitBytes - 1; i >= 0; --i) {
      out.append(QLatin1Char(kHexDigits[valueBytes[i] >> 4]));
      out.append(QLatin1Char(kHexDigits[valueBytes[i] & 0xF]));
    }
  }
}

QString decodeStringColumn(const QByteArray& chunk, const QString& encoding) {
  // Qt6 ネイティブの QStringDecoder は UTF-* / Latin1 / System のみ。
  // Shift_JIS / EUC-JP 等は Qt5Compat の QTextCodec にフォールバック。
  QString decoded;
  QStringDecoder decoder(encoding.toUtf8().constData());
  if (decoder.isValid()) {
    decoded = decoder.decode(chunk);
  } else if (QTextCodec* codec = QTextCodec::codecForName(encoding.toUtf8())) {
    decoded = codec->toUnicode(chunk);
  } else {
    decoded = QStringDecoder(QStringDecoder::Utf8).decode(chunk);
  }

  QString out;
  out.reserve(decoded.size());
  for (QChar ch : decoded) {
    if (ch == QChar(0xFFFD) || !ch.isPrint() || ch.isSpace()) {
      out.append(QLatin1Char('.'));
    } else {
      out.append(ch);
    }
  }
  return out;
}

QString formatHexDump(const QByteArray& data, BinaryViewerUnit unit,
                      BinaryViewerEndian endian, const QString& encoding) {
  const int unitBytes  = binaryViewerUnitToBytes(unit);
  const int unitsPerLine = kBytesPerLine / unitBytes;
  const int midUnit    = unitsPerLine / 2;
  const int lineCount  = (data.size() + kBytesPerLine - 1) / kBytesPerLine;

  QString out;
  out.reserve(lineCount * (8 + 2 + unitsPerLine * (unitBytes * 2 + 1) + 2 + kBytesPerLine + 1));

  for (int line = 0; line < lineCount; ++line) {
    const int off  = line * kBytesPerLine;
    const qint64 addr = static_cast<qint64>(off);

    char addrBuf[9];
    for (int i = 7; i >= 0; --i) {
      addrBuf[i] = kHexDigits[(addr >> ((7 - i) * 4)) & 0xF];
    }
    addrBuf[8] = '\0';
    out.append(QLatin1String(addrBuf, 8));
    out.append(QLatin1String("  "));

    for (int u = 0; u < unitsPerLine; ++u) {
      if (u == midUnit && midUnit > 0) {
        out.append(QLatin1Char(' '));
      }
      const int unitOff = off + u * unitBytes;
      const int avail = qMin(unitBytes, static_cast<int>(data.size()) - unitOff);
      if (avail >= unitBytes) {
        appendUnitHex(out, reinterpret_cast<const unsigned char*>(data.constData() + unitOff),
                      unitBytes, endian);
      } else if (avail > 0) {
        for (int i = 0; i < avail; ++i) {
          const unsigned char b = static_cast<unsigned char>(data[unitOff + i]);
          out.append(QLatin1Char(kHexDigits[b >> 4]));
          out.append(QLatin1Char(kHexDigits[b & 0xF]));
        }
        for (int i = avail; i < unitBytes; ++i) {
          out.append(QLatin1String("  "));
        }
      } else {
        for (int i = 0; i < unitBytes; ++i) {
          out.append(QLatin1String("  "));
        }
      }
      out.append(QLatin1Char(' '));
    }

    out.append(QLatin1Char(' '));

    const int chunkLen = qMin(kBytesPerLine, static_cast<int>(data.size()) - off);
    if (chunkLen > 0) {
      const QByteArray chunk(data.constData() + off, chunkLen);
      out.append(decodeStringColumn(chunk, encoding));
    }

    out.append(QLatin1Char('\n'));
  }

  return out;
}

} // namespace

BinaryView::BinaryView(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
  syncFromSettings();

  // グローバル設定が変更されたらローカル上書きをリセット
  connect(&Settings::instance(), &Settings::settingsChanged, this, [this] {
    syncFromSettings();
    if (!m_filePath.isEmpty()) {
      render();
    }
  });
}

void BinaryView::setupUi() {
  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // ローカル設定オーバーレイ
  QWidget* toolbar = new QWidget(this);
  QHBoxLayout* tb = new QHBoxLayout(toolbar);
  tb->setContentsMargins(4, 2, 4, 2);
  tb->setSpacing(8);

  tb->addWidget(new QLabel(tr("Unit:"), toolbar));
  m_unitCombo = new QComboBox(toolbar);
  m_unitCombo->addItem(tr("1 Byte"), 1);
  m_unitCombo->addItem(tr("2 Byte"), 2);
  m_unitCombo->addItem(tr("4 Byte"), 4);
  m_unitCombo->addItem(tr("8 Byte"), 8);
  // macOS の「キーボードナビゲーション」設定に依存せず Tab で巡回させる
  m_unitCombo->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_unitCombo);

  tb->addWidget(new QLabel(tr("Endian:"), toolbar));
  m_endianCombo = new QComboBox(toolbar);
  m_endianCombo->addItem(tr("Little"), static_cast<int>(BinaryViewerEndian::Little));
  m_endianCombo->addItem(tr("Big"),    static_cast<int>(BinaryViewerEndian::Big));
  m_endianCombo->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_endianCombo);

  tb->addWidget(new QLabel(tr("Encoding:"), toolbar));
  m_encodingCombo = new QComboBox(toolbar);
  m_encodingCombo->setEditable(true);
  m_encodingCombo->setFocusPolicy(Qt::StrongFocus);
  rebuildEncodingItems();
  tb->addWidget(m_encodingCombo);

  tb->addStretch();
  root->addWidget(toolbar);

  // 16 進ダンプ本体 (フォントは syncFromSettings で適用)
  m_textArea = new QPlainTextEdit(this);
  m_textArea->setReadOnly(true);
  m_textArea->setLineWrapMode(QPlainTextEdit::NoWrap);
  // 行頭 8 桁のアドレスを別色で塗るシンタックスハイライタ
  m_addressHighlighter = new AddressHighlighter(m_textArea->document());
  root->addWidget(m_textArea, /*stretch*/ 1);

  // ローカルでの変更は Settings に保存しない (render のみ)
  connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    m_unit = bytesToBinaryViewerUnit(m_unitCombo->currentData().toInt());
    if (!m_filePath.isEmpty()) render();
  });
  connect(m_endianCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    m_endian = static_cast<BinaryViewerEndian>(m_endianCombo->currentData().toInt());
    if (!m_filePath.isEmpty()) render();
  });
  connect(m_encodingCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;
    m_encoding = trimmed;
    if (!m_filePath.isEmpty()) render();
  });
}

void BinaryView::rebuildEncodingItems() {
  m_encodingCombo->clear();
  m_encodingCombo->addItem(QStringLiteral("UTF-8"));
  m_encodingCombo->addItem(QStringLiteral("UTF-16LE"));
  m_encodingCombo->addItem(QStringLiteral("UTF-16BE"));
  m_encodingCombo->addItem(QStringLiteral("Shift_JIS"));
  m_encodingCombo->addItem(QStringLiteral("EUC-JP"));
  m_encodingCombo->addItem(QStringLiteral("ISO-8859-1"));
}

void BinaryView::syncFromSettings() {
  const Settings& s = Settings::instance();
  m_unit     = s.binaryViewerUnit();
  m_endian   = s.binaryViewerEndian();
  m_encoding = s.binaryViewerEncoding();
  m_textArea->setFont(s.binaryViewerFont());

  // 通常 / 選択カラーは QPalette 経由で適用
  QPalette pal = m_textArea->palette();
  pal.setColor(QPalette::Text, s.binaryViewerNormalForeground());
  if (s.binaryViewerNormalBackground().isValid()) {
    pal.setColor(QPalette::Base, s.binaryViewerNormalBackground());
  }
  pal.setColor(QPalette::HighlightedText, s.binaryViewerSelectedForeground());
  pal.setColor(QPalette::Highlight,       s.binaryViewerSelectedBackground());
  m_textArea->setPalette(pal);

  // アドレス列の色は AddressHighlighter が描画時に Settings から直接読むので
  // 明示的に再ハイライトを要求して反映させる
  if (m_addressHighlighter) m_addressHighlighter->rehighlight();

  // コンボの再選択 (シグナルを抑止して二重 render を防ぐ)
  {
    QSignalBlocker b(m_unitCombo);
    const int unitBytes = binaryViewerUnitToBytes(m_unit);
    for (int i = 0; i < m_unitCombo->count(); ++i) {
      if (m_unitCombo->itemData(i).toInt() == unitBytes) {
        m_unitCombo->setCurrentIndex(i);
        break;
      }
    }
  }
  {
    QSignalBlocker b(m_endianCombo);
    for (int i = 0; i < m_endianCombo->count(); ++i) {
      if (m_endianCombo->itemData(i).toInt() == static_cast<int>(m_endian)) {
        m_endianCombo->setCurrentIndex(i);
        break;
      }
    }
  }
  {
    QSignalBlocker b(m_encodingCombo);
    m_encodingCombo->setCurrentText(m_encoding);
  }
}

bool BinaryView::loadFile(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  m_filePath   = filePath;
  m_totalSize  = file.size();
  m_loadedSize = qMin<qint64>(m_totalSize, kMaxBytes);
  m_data       = file.read(m_loadedSize);
  file.close();

  render();
  return true;
}

void BinaryView::clearContent() {
  m_filePath.clear();
  m_data.clear();
  m_totalSize  = 0;
  m_loadedSize = 0;
  m_textArea->clear();
}

QString BinaryView::statusInfo() const {
  if (m_filePath.isEmpty()) return QString();
  QString s = QLocale(QLocale::English).formattedDataSize(m_totalSize);
  if (m_totalSize > m_loadedSize) {
    s += tr("  ·  truncated to first %1")
           .arg(QLocale(QLocale::English).formattedDataSize(m_loadedSize));
  }
  return s;
}

void BinaryView::render() {
  QString text = formatHexDump(m_data, m_unit, m_endian, m_encoding);
  if (m_totalSize > m_loadedSize) {
    text.append(QStringLiteral("...\n[truncated: showing first %1 of %2 bytes]\n")
                  .arg(m_loadedSize)
                  .arg(m_totalSize));
  }
  m_textArea->setPlainText(text);
  m_textArea->moveCursor(QTextCursor::Start);
}

} // namespace Farman

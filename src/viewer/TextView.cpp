#include "TextView.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QLocale>
#include <QStringDecoder>
#include <QTextBlock>
#include <QTextCodec>
#include <QVBoxLayout>

namespace Farman {

// 行番号付きの読み取り専用エディタ本体。TextView から駆動される。
class TextEditArea : public QPlainTextEdit {
public:
  explicit TextEditArea(TextView* host)
    : QPlainTextEdit(host)
    , m_host(host)
  {
    setReadOnly(true);
    m_lineNumberArea = new QWidget(this);
    m_lineNumberArea->installEventFilter(nullptr);  // 自前 paintEvent は emit
    m_lineNumberArea->setAttribute(Qt::WA_OpaquePaintEvent);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, [this](int) { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect& rect, int dy) {
              if (dy) {
                m_lineNumberArea->scroll(0, dy);
              } else {
                m_lineNumberArea->update(0, rect.y(),
                                          m_lineNumberArea->width(), rect.height());
              }
            });

    // 行番号エリアは小ウィジェットでイベントフィルタ経由で paint する
    m_lineNumberArea->installEventFilter(this);
    updateLineNumberAreaWidth();
  }

  // TextView::syncFromSettings から呼ばれる。エディタ全体の見た目を再適用する。
  void applyAppearance() {
    const Settings& s = Settings::instance();
    setFont(s.textViewerFont());

    QPalette pal = palette();
    pal.setColor(QPalette::Text, s.textViewerNormalForeground());
    if (s.textViewerNormalBackground().isValid()) {
      pal.setColor(QPalette::Base, s.textViewerNormalBackground());
    }
    pal.setColor(QPalette::HighlightedText, s.textViewerSelectedForeground());
    pal.setColor(QPalette::Highlight,       s.textViewerSelectedBackground());
    setPalette(pal);
  }

  // ローカルで設定変更 (line numbers / word wrap) 後に呼ばれる。
  void applyLocalLayout(bool wordWrap, bool showLineNumbers) {
    setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    m_lineNumberArea->setVisible(showLineNumbers);
    updateLineNumberAreaWidth();
    m_lineNumberArea->update();
  }

  int lineNumberAreaWidth() const {
    if (!m_host->showLineNumbers()) return 0;
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 8;
  }

protected:
  void resizeEvent(QResizeEvent* event) override {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                         lineNumberAreaWidth(), cr.height()));
  }

  bool eventFilter(QObject* watched, QEvent* event) override {
    if (watched == m_lineNumberArea && event->type() == QEvent::Paint) {
      paintLineNumbers(static_cast<QPaintEvent*>(event));
      return true;
    }
    return QPlainTextEdit::eventFilter(watched, event);
  }

private:
  void updateLineNumberAreaWidth() {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
  }

  void paintLineNumbers(QPaintEvent* event) {
    if (!m_host->showLineNumbers()) return;

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_host->lineNumberBackground());

    QTextBlock block = firstVisibleBlock();
    int blockNumber  = block.blockNumber();
    int top          = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom       = top + qRound(blockBoundingRect(block).height());

    painter.setPen(m_host->lineNumberForeground());

    while (block.isValid() && top <= event->rect().bottom()) {
      if (block.isVisible() && bottom >= event->rect().top()) {
        const QString num = QString::number(blockNumber + 1);
        painter.drawText(0, top, m_lineNumberArea->width() - 6,
                         fontMetrics().height(), Qt::AlignRight, num);
      }
      block       = block.next();
      top         = bottom;
      bottom      = top + qRound(blockBoundingRect(block).height());
      ++blockNumber;
    }
  }

  TextView* m_host;
  QWidget*  m_lineNumberArea;
};

// ---------- TextView ----------

TextView::TextView(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
  syncFromSettings();
  applyEditAreaSettings();

  connect(&Settings::instance(), &Settings::settingsChanged, this, [this] {
    syncFromSettings();
    applyEditAreaSettings();
    if (!m_data.isEmpty()) {
      reloadFromBuffer();
    }
  });
}

void TextView::setupUi() {
  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // ツールバー
  QWidget* toolbar = new QWidget(this);
  QHBoxLayout* tb = new QHBoxLayout(toolbar);
  tb->setContentsMargins(4, 2, 4, 2);
  tb->setSpacing(8);

  tb->addWidget(new QLabel(tr("Encoding:"), toolbar));
  m_encodingCombo = new QComboBox(toolbar);
  m_encodingCombo->setEditable(true);
  m_encodingCombo->addItem(QStringLiteral("UTF-8"));
  m_encodingCombo->addItem(QStringLiteral("UTF-16LE"));
  m_encodingCombo->addItem(QStringLiteral("UTF-16BE"));
  m_encodingCombo->addItem(QStringLiteral("Shift_JIS"));
  m_encodingCombo->addItem(QStringLiteral("EUC-JP"));
  m_encodingCombo->addItem(QStringLiteral("ISO-8859-1"));
  m_encodingCombo->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_encodingCombo);

  m_lineNumbersCheck = new QCheckBox(tr("Line Numbers"), toolbar);
  m_lineNumbersCheck->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_lineNumbersCheck);

  m_wordWrapCheck = new QCheckBox(tr("Word Wrap"), toolbar);
  m_wordWrapCheck->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_wordWrapCheck);

  tb->addStretch();
  root->addWidget(toolbar);

  // 本体
  m_editArea = new TextEditArea(this);
  root->addWidget(m_editArea, /*stretch*/ 1);
  // ViewerPanel など外側から setFocus されたとき、フォーカスを本体に流す
  setFocusProxy(m_editArea);

  // ローカル変更: Settings には保存しない
  connect(m_encodingCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;
    m_encoding = trimmed;
    if (!m_data.isEmpty()) reloadFromBuffer();
  });
  connect(m_lineNumbersCheck, &QCheckBox::toggled, this, [this](bool on) {
    m_showLineNumbers = on;
    applyEditAreaSettings();
  });
  connect(m_wordWrapCheck, &QCheckBox::toggled, this, [this](bool on) {
    m_wordWrap = on;
    applyEditAreaSettings();
  });
}

void TextView::syncFromSettings() {
  const Settings& s = Settings::instance();
  m_encoding        = s.textViewerEncoding();
  m_showLineNumbers = s.textViewerShowLineNumbers();
  m_wordWrap        = s.textViewerWordWrap();

  {
    QSignalBlocker b(m_encodingCombo);
    m_encodingCombo->setCurrentText(m_encoding);
  }
  {
    QSignalBlocker b(m_lineNumbersCheck);
    m_lineNumbersCheck->setChecked(m_showLineNumbers);
  }
  {
    QSignalBlocker b(m_wordWrapCheck);
    m_wordWrapCheck->setChecked(m_wordWrap);
  }
}

void TextView::applyEditAreaSettings() {
  if (!m_editArea) return;
  m_editArea->applyAppearance();
  m_editArea->applyLocalLayout(m_wordWrap, m_showLineNumbers);
}

bool TextView::loadFile(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  m_filePath = filePath;
  m_data     = file.readAll();
  file.close();
  reloadFromBuffer();
  return true;
}

void TextView::clearContent() {
  m_filePath.clear();
  m_data.clear();
  m_editArea->clear();
}

void TextView::reloadFromBuffer() {
  const QString text = decodeBytes(m_data, m_encoding);
  m_editArea->setPlainText(text);
  m_editArea->moveCursor(QTextCursor::Start);
}

QString TextView::decodeBytes(const QByteArray& data, const QString& encoding) {
  QStringDecoder decoder(encoding.toUtf8().constData());
  if (decoder.isValid()) {
    return decoder.decode(data);
  }
  if (QTextCodec* codec = QTextCodec::codecForName(encoding.toUtf8())) {
    return codec->toUnicode(data);
  }
  return QStringDecoder(QStringDecoder::Utf8).decode(data);
}

QColor TextView::lineNumberForeground() const {
  return Settings::instance().textViewerLineNumberForeground();
}

QColor TextView::lineNumberBackground() const {
  return Settings::instance().textViewerLineNumberBackground();
}

QString TextView::statusInfo() const {
  if (m_filePath.isEmpty()) return QString();
  const int lines = m_editArea ? m_editArea->blockCount() : 0;
  return QStringLiteral("%1  ·  %2  ·  %3 lines")
    .arg(m_encoding)
    .arg(QLocale(QLocale::English).formattedDataSize(static_cast<qint64>(m_data.size())))
    .arg(lines);
}

} // namespace Farman

#include "ImageView.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLocale>
#include <QMovie>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace Farman {

// 画像描画ウィジェット。背景 (チェック柄 / 単色) と画像を自前描画する。
class ImageDisplay : public QWidget {
public:
  // host: ImageView 本体への参照 (ローカルの透明モードを取得するため)
  explicit ImageDisplay(QWidget* parent, ImageView* host)
    : QWidget(parent), m_host(host) {
    setAutoFillBackground(false);
  }

  // 静止画モード: pixmap セット (movie はクリア)
  void setStaticPixmap(const QPixmap& p) {
    m_movie = nullptr;
    m_pixmap = p;
    updateGeometryAndPaint();
  }

  // アニメモード: 外部所有の QMovie を表示。frameChanged で再描画。
  void setMovie(QMovie* movie) {
    m_movie = movie;
    if (m_movie) {
      m_pixmap = m_movie->currentPixmap();
      // QMovie はサイズが分かっていないこともあるので、初期サイズは pixmap から
      QObject::connect(m_movie, &QMovie::frameChanged, this, [this](int) {
        if (m_movie) {
          m_pixmap = m_movie->currentPixmap();
        }
        update();
      });
    }
    updateGeometryAndPaint();
  }

  void clearImage() {
    m_movie = nullptr;
    m_pixmap = QPixmap();
    updateGeometryAndPaint();
  }

  void setZoomPercent(int percent) {
    m_zoomPercent = qBound(1, percent, 1000);
    m_fit = false;
    updateGeometryAndPaint();
  }
  void setFitToWindow(bool fit) {
    m_fit = fit;
    updateGeometryAndPaint();
  }

  // viewport (= QScrollArea のビューポート) を渡す。Fit 計算で使う。
  void setViewportWidget(QWidget* viewport) { m_viewport = viewport; }

  // Fit 時に算出される実効ズーム % (UI 表示用)。
  int effectiveZoomPercent() const {
    const QSize ds = displaySize();
    if (m_pixmap.isNull() || ds.isEmpty()) return m_zoomPercent;
    const double rx = static_cast<double>(ds.width())  / m_pixmap.width();
    const double ry = static_cast<double>(ds.height()) / m_pixmap.height();
    return qMax(1, static_cast<int>(qRound(qMin(rx, ry) * 100)));
  }

  // 親ウィジェットのリサイズを反映するため呼ばれる (Fit 時のスケール再計算と、
  // 非 Fit 時の背景フィル用に常に呼び出す)。
  void onViewportResized() {
    updateGeometryAndPaint();
  }

protected:
  void paintEvent(QPaintEvent* event) override {
    QPainter p(this);
    paintBackground(p, event->rect());
    if (!m_pixmap.isNull()) {
      const QSize ds = displaySize();
      const QRect target((width() - ds.width()) / 2, (height() - ds.height()) / 2,
                          ds.width(), ds.height());
      p.drawPixmap(target, m_pixmap);
    }
  }

private:
  QSize displaySize() const {
    if (m_pixmap.isNull()) return QSize();
    if (m_fit && m_viewport) {
      const QSize avail = m_viewport->size();
      return m_pixmap.size().scaled(avail, Qt::KeepAspectRatio);
    }
    return m_pixmap.size() * (m_zoomPercent / 100.0);
  }

  void updateGeometryAndPaint() {
    QSize image = displaySize();
    QSize s = image;
    // 透明部分の背景でビューポート全体を埋めるため、Fit でなくても画像が
    // ビューポートより小さい場合はウィジェットをビューポートサイズまで広げる
    if (m_viewport) {
      const QSize vp = m_viewport->size();
      s.setWidth(qMax(s.width(),  vp.width()));
      s.setHeight(qMax(s.height(), vp.height()));
    }
    if (s.isEmpty()) s = QSize(1, 1);
    resize(s);
    update();
  }

  void paintBackground(QPainter& p, const QRect& rect) {
    const Settings& s = Settings::instance();
    // モードはローカル上書きを優先 (ImageView 経由で取得)、色は Settings 由来
    const ImageTransparencyMode mode =
      m_host ? m_host->transparencyMode() : s.imageViewerTransparencyMode();
    if (mode == ImageTransparencyMode::SolidColor) {
      p.fillRect(rect, s.imageViewerSolidColor());
      return;
    }
    // チェック柄 (16px) — 2 つの色は Settings から取得
    const int cell = 16;
    const QColor c1 = s.imageViewerCheckerColor1();
    const QColor c2 = s.imageViewerCheckerColor2();
    const int x0 = (rect.left() / cell) * cell;
    const int y0 = (rect.top()  / cell) * cell;
    for (int y = y0; y <= rect.bottom(); y += cell) {
      for (int x = x0; x <= rect.right(); x += cell) {
        const bool odd = ((x / cell) + (y / cell)) & 1;
        p.fillRect(QRect(x, y, cell, cell), odd ? c2 : c1);
      }
    }
  }

  QPixmap         m_pixmap;
  QMovie*         m_movie    = nullptr;
  int             m_zoomPercent = 100;
  bool            m_fit         = false;
  QPointer<QWidget> m_viewport;
  ImageView*      m_host = nullptr;
};

// ---------- ImageView ----------

ImageView::ImageView(QWidget* parent)
  : QWidget(parent)
{
  setupUi();
  syncFromSettings();
  applyDisplayState();

  connect(&Settings::instance(), &Settings::settingsChanged, this, [this] {
    syncFromSettings();
    applyDisplayState();
    if (m_display) m_display->update();
  });
}

void ImageView::setupUi() {
  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // ツールバー
  QWidget* toolbar = new QWidget(this);
  QHBoxLayout* tb = new QHBoxLayout(toolbar);
  tb->setContentsMargins(4, 2, 4, 2);
  tb->setSpacing(8);

  tb->addWidget(new QLabel(tr("Zoom:"), toolbar));
  m_zoomCombo = new QComboBox(toolbar);
  m_zoomCombo->setEditable(true);
  for (int p : { 25, 50, 75, 100, 200 }) {
    m_zoomCombo->addItem(QString::number(p) + QLatin1Char('%'), p);
  }
  m_zoomCombo->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_zoomCombo);

  m_fitCheck = new QCheckBox(tr("Fit to Window"), toolbar);
  m_fitCheck->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_fitCheck);

  m_animButton = new QPushButton(toolbar);
  m_animButton->setCheckable(true);
  m_animButton->setFocusPolicy(Qt::StrongFocus);
  m_animButton->setToolTip(tr("Play / stop animation (GIF / WebP)"));
  tb->addWidget(m_animButton);

  tb->addWidget(new QLabel(tr("Transparency:"), toolbar));
  m_transparencyCombo = new QComboBox(toolbar);
  m_transparencyCombo->addItem(tr("Checker"),     static_cast<int>(ImageTransparencyMode::Checker));
  m_transparencyCombo->addItem(tr("Solid Color"), static_cast<int>(ImageTransparencyMode::SolidColor));
  m_transparencyCombo->setFocusPolicy(Qt::StrongFocus);
  tb->addWidget(m_transparencyCombo);

  tb->addStretch();
  root->addWidget(toolbar);

  // 画像表示部 (スクロール可)
  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setWidgetResizable(false);
  m_scrollArea->setAlignment(Qt::AlignCenter);
  m_display = new ImageDisplay(m_scrollArea, this);
  m_display->setViewportWidget(m_scrollArea->viewport());
  m_scrollArea->setWidget(m_display);
  // ビューポートのリサイズで ImageDisplay のサイズを再計算する
  m_scrollArea->viewport()->installEventFilter(this);
  root->addWidget(m_scrollArea, /*stretch*/ 1);

  setFocusProxy(m_scrollArea);

  // ローカル変更
  connect(m_zoomCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;
    QString num = trimmed;
    if (num.endsWith(QLatin1Char('%'))) num.chop(1);
    bool ok = false;
    int v = num.toInt(&ok);
    if (!ok) return;
    m_zoomPercent = qBound(1, v, 1000);
    if (!m_fitToWindow) {
      m_display->setZoomPercent(m_zoomPercent);
    }
  });
  connect(m_fitCheck, &QCheckBox::toggled, this, [this](bool on) {
    if (on) {
      m_fitToWindow = true;
      updateZoomEnabled();
      m_display->setFitToWindow(true);
      // Fit 時はコンボの表示値を実効ズームに合わせる (シグナル抑止)
      QSignalBlocker b(m_zoomCombo);
      m_zoomCombo->setCurrentText(QString::number(m_display->effectiveZoomPercent()) + QLatin1Char('%'));
    } else {
      // Fit OFF: 直前の実効ズームを引き継ぐ (フィット結果のまま固定)
      const int held = m_display->effectiveZoomPercent();
      m_fitToWindow = false;
      updateZoomEnabled();
      m_display->setFitToWindow(false);
      m_zoomPercent = qBound(1, held, 1000);
      m_display->setZoomPercent(m_zoomPercent);
      QSignalBlocker b(m_zoomCombo);
      m_zoomCombo->setCurrentText(QString::number(m_zoomPercent) + QLatin1Char('%'));
    }
  });
  connect(m_animButton, &QPushButton::toggled, this, [this](bool on) {
    m_animation = on;
    m_animButton->setText(on ? tr("⏸ Stop") : tr("▶ Play"));
    applyDisplayState();
  });
  connect(m_transparencyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    m_transparencyMode = static_cast<ImageTransparencyMode>(
      m_transparencyCombo->currentData().toInt());
    if (m_display) m_display->update();
  });
}

void ImageView::syncFromSettings() {
  const Settings& s = Settings::instance();
  m_zoomPercent      = s.imageViewerZoomPercent();
  m_fitToWindow      = s.imageViewerFitToWindow();
  m_animation        = s.imageViewerAnimation();
  m_transparencyMode = s.imageViewerTransparencyMode();

  {
    QSignalBlocker b(m_zoomCombo);
    m_zoomCombo->setCurrentText(QString::number(m_zoomPercent) + QLatin1Char('%'));
  }
  {
    QSignalBlocker b(m_fitCheck);
    m_fitCheck->setChecked(m_fitToWindow);
  }
  {
    QSignalBlocker b(m_animButton);
    m_animButton->setChecked(m_animation);
    m_animButton->setText(m_animation ? tr("⏸ Stop") : tr("▶ Play"));
  }
  {
    QSignalBlocker b(m_transparencyCombo);
    for (int i = 0; i < m_transparencyCombo->count(); ++i) {
      if (m_transparencyCombo->itemData(i).toInt() == static_cast<int>(m_transparencyMode)) {
        m_transparencyCombo->setCurrentIndex(i);
        break;
      }
    }
  }
  updateZoomEnabled();
}

void ImageView::updateZoomEnabled() {
  // Fit 時は手動ズーム指定を無効化 (SPEC: ON 時は設定不可)
  m_zoomCombo->setEnabled(!m_fitToWindow);
}

void ImageView::applyDisplayState() {
  if (!m_display) return;
  m_display->setFitToWindow(m_fitToWindow);
  if (!m_fitToWindow) {
    m_display->setZoomPercent(m_zoomPercent);
  }

  // アニメ再生 / 停止の切替
  if (m_movie) {
    if (m_fileIsAnimated && m_animation) {
      if (m_movie->state() != QMovie::Running) m_movie->start();
    } else {
      if (m_movie->state() == QMovie::Running) m_movie->stop();
      m_movie->jumpToFrame(0);
    }
  }
}

bool ImageView::loadFile(const QString& filePath) {
  // 同期ロード経路: prepareLoad + applyPreparedLoad を続けて呼ぶだけ。
  // 非同期化したい呼び出し元 (ViewerPanel) は両者を別スレッドに振り分ける。
  PreparedLoad p = prepareLoad(filePath);
  if (!p.ok) return false;
  applyPreparedLoad(p);
  return true;
}

ImageView::PreparedLoad ImageView::prepareLoad(const QString& filePath) {
  PreparedLoad r;
  r.filePath   = filePath;
  r.isAnimated = detectAnimated(filePath);

  if (!r.isAnimated) {
    // 静止画は QImage で読み込む (QPixmap はメインスレッド限定なので bg では避ける)。
    QImage img;
    if (!img.load(filePath)) {
      r.ok = false;
      return r;
    }
    r.image = img;
  }
  // アニメ画像の場合、QMovie 構築は applyPreparedLoad (main thread) で行う。
  r.ok = true;
  return r;
}

void ImageView::applyPreparedLoad(const PreparedLoad& r) {
  // 既存の Movie を破棄
  if (m_movie) {
    m_movie->deleteLater();
    m_movie = nullptr;
  }
  m_fileIsAnimated = r.isAnimated;
  m_filePath       = r.filePath;

  if (m_fileIsAnimated) {
    auto* movie = new QMovie(r.filePath, QByteArray(), this);
    if (!movie->isValid()) {
      movie->deleteLater();
      m_fileIsAnimated = false;
    } else {
      m_movie = movie;
      m_display->setMovie(movie);
      // 初期 1 フレーム描画
      movie->jumpToFrame(0);
    }
  }

  if (!m_fileIsAnimated) {
    // bg で QImage に読み込んだものを main で QPixmap に変換して反映。
    const QPixmap pm = QPixmap::fromImage(r.image);
    if (!pm.isNull()) {
      m_display->setStaticPixmap(pm);
    }
  }

  applyDisplayState();
}

void ImageView::clearContent() {
  if (m_movie) {
    m_movie->deleteLater();
    m_movie = nullptr;
  }
  m_filePath.clear();
  m_fileIsAnimated = false;
  m_display->clearImage();
}

bool ImageView::eventFilter(QObject* watched, QEvent* event) {
  if (m_scrollArea && watched == m_scrollArea->viewport()
      && event->type() == QEvent::Resize) {
    if (m_display) m_display->onViewportResized();
  }
  return QWidget::eventFilter(watched, event);
}

bool ImageView::detectAnimated(const QString& filePath) {
  QImageReader reader(filePath);
  return reader.supportsAnimation() && reader.imageCount() > 1;
}

QString ImageView::statusInfo() const {
  if (m_filePath.isEmpty()) return QString();
  QImageReader reader(m_filePath);
  const QString fmt = QString::fromLatin1(reader.format()).toUpper();
  const QSize sz   = reader.size();
  const qint64 bytes = QFileInfo(m_filePath).size();
  QString s = QStringLiteral("%1  ·  %2x%3")
                .arg(fmt.isEmpty() ? QStringLiteral("?") : fmt)
                .arg(sz.width()).arg(sz.height());
  if (m_fileIsAnimated && m_movie) {
    s += QStringLiteral("  ·  %1 frames").arg(m_movie->frameCount());
  }
  s += QStringLiteral("  ·  %1").arg(QLocale(QLocale::English).formattedDataSize(bytes));
  return s;
}

} // namespace Farman

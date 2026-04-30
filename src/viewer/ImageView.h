#pragma once

#include "types.h"
#include <QImage>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QMovie;
class QPushButton;
class QScrollArea;

namespace Farman {

class ImageDisplay;

// 画像ファイル表示用ウィジェット。
//   - 上部のツールバーで Zoom (%) / Fit / Animation をその場で上書き可能
//     (上書き内容は Settings に保存されない)
//   - 透明部分の表示方法 (単色 / チェック柄) と背景色は Settings から取得
//   - GIF / WebP (アニメあり) は QMovie で再生 (APNG は Qt PNG プラグインの
//     ビルド依存のため現状非対応)
//   - Settings::settingsChanged でローカルの上書きはリセットされる
class ImageView : public QWidget {
  Q_OBJECT

public:
  // 非同期ロード用の中間表現。アニメ画像はメインスレッドで QMovie を作る
  // 必要があるため bg では QImage を使わず「アニメかどうか」だけ確定させる。
  struct PreparedLoad {
    bool    ok          = false;
    QString filePath;
    bool    isAnimated  = false;
    QImage  image;       // 静止画のときのみ有効
  };

  explicit ImageView(QWidget* parent = nullptr);

  // 失敗時 (open エラー or 画像でない) は false。
  bool loadFile(const QString& filePath);

  // ワーカースレッドで実行可能なロード処理 (UI 非依存)。
  static PreparedLoad prepareLoad(const QString& filePath);
  // ワーカーの結果を UI に反映する。必ずメインスレッドから呼ぶこと。
  void applyPreparedLoad(const PreparedLoad& result);

  // 表示中ファイルをクリア。
  void clearContent();

  // ステータスバー表示用の要約 ("PNG · 800x600 · 24 KB" 等、アニメ時はフレーム数も)
  QString statusInfo() const;

  // ImageDisplay から参照される実効透明モード (ローカル上書き)。
  ImageTransparencyMode transparencyMode() const { return m_transparencyMode; }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void setupUi();
  void syncFromSettings();
  void applyDisplayState();
  void updateZoomEnabled();
  static bool detectAnimated(const QString& filePath);

  // ツールバー
  QComboBox*   m_zoomCombo         = nullptr;
  QCheckBox*   m_fitCheck          = nullptr;
  QPushButton* m_animButton        = nullptr;  // checkable: ▶ Play / ⏸ Stop トグル
  QComboBox*   m_transparencyCombo = nullptr;

  // 表示部
  QScrollArea*  m_scrollArea = nullptr;
  ImageDisplay* m_display    = nullptr;

  QString             m_filePath;
  QPointer<QMovie>    m_movie;        // アニメ再生用 (m_display の親で破棄)
  bool                m_fileIsAnimated = false;

  // 表示に使う実効設定 (ローカル上書き)。Settings 変更で再同期される。
  int                   m_zoomPercent     = 100;
  bool                  m_fitToWindow     = false;
  bool                  m_animation       = false;
  ImageTransparencyMode m_transparencyMode = ImageTransparencyMode::Checker;
};

} // namespace Farman

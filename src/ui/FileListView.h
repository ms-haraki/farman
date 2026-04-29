#pragma once

#include <QPoint>
#include <QTableView>
#include <QUrl>
#include <functional>

namespace Farman {

// Drag & Drop 対応の QTableView。
//
// - **Drag Out (送信側)**:
//   左ボタン押下 + ドラッグ閾値を超えると `m_urlsProvider` を呼んで対象 URL を
//   取得し、`text/uri-list` の MIME で QDrag を起動する。Provider は呼び出し側
//   (FileListPane) が「farman の選択状態 + カーソル行」をベースに作って渡す。
//
// - **Drop In (受信側)**:
//   外部 / 反対側ペインからドロップされた URL を `externalUrlsDropped` で
//   通知する。同じビューからの自分自身宛て drop は無視する。
class FileListView : public QTableView {
  Q_OBJECT

public:
  using UrlsProvider = std::function<QList<QUrl>()>;

  explicit FileListView(QWidget* parent = nullptr);

  // Drag-out 時にどの URL 群を引っ張るかを供給するコールバックを設定する。
  void setUrlsProvider(UrlsProvider provider) { m_urlsProvider = std::move(provider); }

signals:
  // 外部 (or 反対側ペイン) からファイル / ディレクトリがドロップされた。
  void externalUrlsDropped(const QList<QUrl>& urls);

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

private:
  void startExternalDrag();

  QPoint       m_dragStartPos;
  UrlsProvider m_urlsProvider;
};

} // namespace Farman

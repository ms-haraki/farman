#pragma once

#include "UserCommand.h"

#include <QObject>
#include <functional>

namespace Farman {

struct PaneContext;

// Settings の userCommands() と CommandRegistry / Tools メニューの間を仲介する。
//
// - 起動時 (sync()): Settings を読んで CommandRegistry に各エントリを登録
// - Settings 変更時: 古い user.cmd.* を解除して新しいものを登録、
//                   userCommandsChanged() を発火して Tools メニューを再構築させる
// - 実行時:        指定 ID のコマンドを引いて UserCommandRunner で起動する
//
// ContextProvider は MainWindow / FileManagerPanel から PaneContext を集める
// 関数。CommandRegistry に登録するラムダから毎回呼ぶことで、起動時点でカーソル
// 位置や選択ファイルが正しく取れる。
class UserCommandManager : public QObject {
  Q_OBJECT

public:
  using ContextProvider = std::function<PaneContext()>;

  static UserCommandManager& instance();

  // PaneContext の供給元を設定する。Manager 自体はファイラ UI を直接知らないので、
  // MainWindow 構築時に lambda を渡してもらう形。未設定でも動くが、
  // 当然プレースホルダはアクティブペイン情報無しで展開される (= 空文字)。
  void setContextProvider(ContextProvider provider);

  // Settings::userCommands() を反映して CommandRegistry を作り直す。
  // 起動時、および Settings::settingsChanged 受信時に呼ぶ。
  void sync();

  // 現在 Manager が握っている UserCommand 一覧。Tools メニュー構築用。
  // Settings の値そのものではなく sync() 時にスナップショットしたもの。
  const QList<UserCommand>& commands() const { return m_commands; }

  // 1 件を起動する。errorOut にユーザー向けの失敗メッセージが入る。
  bool run(const QString& commandId, QString* errorOut = nullptr);

  // 「テスト起動」用: Settings 保存前のテンポラリな UserCommand を渡して即起動。
  bool runTransient(const UserCommand& cmd, QString* errorOut = nullptr);

signals:
  // Tools メニューや関連 UI を作り直すべきタイミング (sync 後)。
  void userCommandsChanged();

private:
  explicit UserCommandManager(QObject* parent = nullptr);

  // 登録済み user.cmd.* を CommandRegistry から落とす。
  void clearRegistered();

  // 1 件を CommandRegistry に登録する。
  void registerOne(const UserCommand& cmd);

  PaneContext        currentContext() const;

  ContextProvider    m_contextProvider;
  QList<UserCommand> m_commands;
};

} // namespace Farman

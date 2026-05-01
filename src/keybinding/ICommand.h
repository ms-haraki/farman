#pragma once

#include <QString>
#include <functional>

namespace Farman {

class ICommand {
public:
  virtual ~ICommand() = default;

  virtual QString id()       const = 0;  // "copy", "move", "delete"
  virtual QString label()    const = 0;  // "コピー"（設定画面に表示）
  virtual QString category() const { return "general"; }
  // ショートカット一覧で表示する補足説明 (任意)。空なら label のみ表示。
  virtual QString description() const { return {}; }

  virtual bool isEnabled()   const { return true; }
  virtual void execute()           = 0;
};

// ラムダからコマンドを手軽に作るヘルパー
class LambdaCommand : public ICommand {
public:
  LambdaCommand(
    QString               id,
    QString               label,
    std::function<void()> fn,
    QString               category    = "general",
    QString               description = {}
  );

  QString id()          const override;
  QString label()       const override;
  QString category()    const override;
  QString description() const override;
  void    execute()           override;

private:
  QString               m_id;
  QString               m_label;
  QString               m_category;
  QString               m_description;
  std::function<void()> m_fn;
};

} // namespace Farman

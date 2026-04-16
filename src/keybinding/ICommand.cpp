#include "ICommand.h"

namespace Farman {

LambdaCommand::LambdaCommand(
  QString               id,
  QString               label,
  std::function<void()> fn,
  QString               category)
  : m_id(std::move(id))
  , m_label(std::move(label))
  , m_category(std::move(category))
  , m_fn(std::move(fn)) {
}

QString LambdaCommand::id() const {
  return m_id;
}

QString LambdaCommand::label() const {
  return m_label;
}

QString LambdaCommand::category() const {
  return m_category;
}

void LambdaCommand::execute() {
  if (m_fn) {
    m_fn();
  }
}

} // namespace Farman

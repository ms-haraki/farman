#pragma once

#include "BaseCommand.h"

namespace Farman {

// Quit application
class AppQuitCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "app.quit"; }
  QString label() const override { return "Quit"; }
  QString category() const override { return "application"; }
  void execute() override;
};

// View file
class ViewFileCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "view.file"; }
  QString label() const override { return "View File"; }
  QString category() const override { return "view"; }
  void execute() override;
};

} // namespace Farman

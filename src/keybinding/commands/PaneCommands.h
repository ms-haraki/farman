#pragma once

#include "BaseCommand.h"

namespace Farman {

// Switch active pane
class PaneSwitchCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "pane.switch"; }
  QString label() const override { return "Switch Pane"; }
  QString category() const override { return "pane"; }
  void execute() override;
};

// Toggle single pane mode
class PaneToggleSingleCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "pane.toggle_single"; }
  QString label() const override { return "Toggle Single Pane Mode"; }
  QString category() const override { return "pane"; }
  void execute() override;
};

} // namespace Farman

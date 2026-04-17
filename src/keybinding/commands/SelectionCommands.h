#pragma once

#include "BaseCommand.h"

namespace Farman {

// Toggle selection of current item
class SelectToggleCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "select.toggle"; }
  QString label() const override { return "Toggle Selection"; }
  QString category() const override { return "selection"; }
  void execute() override;
};

// Toggle selection and move down
class SelectToggleAndDownCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "select.toggle_and_down"; }
  QString label() const override { return "Toggle Selection and Move Down"; }
  QString category() const override { return "selection"; }
  void execute() override;
};

// Invert selection
class SelectInvertCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "select.invert"; }
  QString label() const override { return "Invert Selection"; }
  QString category() const override { return "selection"; }
  void execute() override;
};

// Select all
class SelectAllCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "select.all"; }
  QString label() const override { return "Select All"; }
  QString category() const override { return "selection"; }
  void execute() override;
};

} // namespace Farman

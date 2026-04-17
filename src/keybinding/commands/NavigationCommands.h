#pragma once

#include "BaseCommand.h"

namespace Farman {

// Navigate up in file list
class NavigateUpCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.up"; }
  QString label() const override { return "Up"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate down in file list
class NavigateDownCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.down"; }
  QString label() const override { return "Down"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate left (move cursor left or switch pane)
class NavigateLeftCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.left"; }
  QString label() const override { return "Left"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate right (move cursor right or switch pane)
class NavigateRightCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.right"; }
  QString label() const override { return "Right"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate to home (top of list)
class NavigateHomeCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.home"; }
  QString label() const override { return "Home"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate to end (bottom of list)
class NavigateEndCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.end"; }
  QString label() const override { return "End"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate page up
class NavigatePageUpCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.pageup"; }
  QString label() const override { return "Page Up"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate page down
class NavigatePageDownCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.pagedown"; }
  QString label() const override { return "Page Down"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Enter directory or open file
class NavigateEnterCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.enter"; }
  QString label() const override { return "Enter"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

// Navigate to parent directory
class NavigateParentCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "navigate.parent"; }
  QString label() const override { return "Parent Directory"; }
  QString category() const override { return "navigation"; }
  void execute() override;
};

} // namespace Farman

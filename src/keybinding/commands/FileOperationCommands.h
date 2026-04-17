#pragma once

#include "BaseCommand.h"

namespace Farman {

// Copy selected files
class FileCopyCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "file.copy"; }
  QString label() const override { return "Copy"; }
  QString category() const override { return "file"; }
  void execute() override;
};

// Move selected files
class FileMoveCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "file.move"; }
  QString label() const override { return "Move"; }
  QString category() const override { return "file"; }
  void execute() override;
};

// Delete selected files
class FileDeleteCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "file.delete"; }
  QString label() const override { return "Delete"; }
  QString category() const override { return "file"; }
  void execute() override;
};

// Create new directory
class FileMkdirCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "file.mkdir"; }
  QString label() const override { return "Make Directory"; }
  QString category() const override { return "file"; }
  void execute() override;
};

} // namespace Farman

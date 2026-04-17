#pragma once

#include "keybinding/ICommand.h"

namespace Farman {

class MainWindow;

// Base class for commands that need access to MainWindow
class BaseCommand : public ICommand {
public:
  explicit BaseCommand(MainWindow* mainWindow);
  virtual ~BaseCommand() override = default;

  bool isEnabled() const override;

protected:
  MainWindow* mainWindow() const { return m_mainWindow; }

private:
  MainWindow* m_mainWindow;
};

} // namespace Farman

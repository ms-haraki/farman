# KeyBinding and Settings System Integration Guide

This document describes the keybinding and settings infrastructure implemented for farman and provides guidance on integrating it with MainWindow.

## Overview

The system consists of four main components:

1. **ICommand** - Interface for all commands
2. **CommandRegistry** - Central registry for all commands
3. **KeyBindingManager** - Maps key sequences to command IDs
4. **Settings** - Manages application settings with JSON persistence

## File Locations

### Core Infrastructure
- `/Users/ms-haraki/Works/farman/src/keybinding/ICommand.h/cpp` - Command interface and LambdaCommand helper
- `/Users/ms-haraki/Works/farman/src/keybinding/CommandRegistry.h/cpp` - Command registry (fully implemented)
- `/Users/ms-haraki/Works/farman/src/keybinding/KeyBindingManager.h/cpp` - Key binding manager (fully implemented)
- `/Users/ms-haraki/Works/farman/src/settings/Settings.h/cpp` - Settings manager with JSON support (fully implemented)

### Command Classes
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/BaseCommand.h/cpp` - Base class for commands that need MainWindow access
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/NavigationCommands.h/cpp` - Navigation commands (up, down, enter, etc.)
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/SelectionCommands.h/cpp` - Selection commands (toggle, invert, select all)
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/PaneCommands.h/cpp` - Pane management commands (switch, toggle single mode)
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/FileOperationCommands.h/cpp` - File operations (copy, move, delete, mkdir)
- `/Users/ms-haraki/Works/farman/src/keybinding/commands/ApplicationCommands.h/cpp` - Application commands (quit, view file)

## Implementation Status

### Completed
- CommandRegistry with full command registration, execution, and lookup
- KeyBindingManager with JSON-based keybinding persistence via QSettings
- Settings with comprehensive JSON file support at ~/.config/farman/settings.json (platform-aware)
- All basic command classes with stub implementations
- Default keybindings defined in KeyBindingManager::loadDefaults()

### Current State
- Commands are implemented as stubs that log to qDebug()
- Commands need to be connected to MainWindow functionality
- The system is ready for integration but not yet wired into MainWindow

## Default Keybindings

The system includes these default keybindings (defined in KeyBindingManager::loadDefaults()):

### Navigation
- Up Arrow → navigate.up
- Down Arrow → navigate.down
- Left Arrow → navigate.left
- Right Arrow → navigate.right
- Home → navigate.home
- End → navigate.end
- PageUp → navigate.pageup
- PageDown → navigate.pagedown
- Return/Enter → navigate.enter
- Backspace → navigate.parent

### Selection
- Space → select.toggle
- Insert → select.toggle_and_down
- Asterisk (*) → select.invert
- Ctrl+A → select.all

### Pane Management
- Tab → pane.switch
- Ctrl+O → pane.toggle_single

### File Operations
- F5 → file.copy
- F6 → file.move
- F8 → file.delete
- F7 → file.mkdir

### View
- F3 → view.file

### Application
- F10 → app.quit
- Ctrl+Q → app.quit

## Settings File Format

Settings are stored as human-editable JSON at `~/.config/farman/settings.json` (macOS: `~/Library/Application Support/farman/settings.json`):

```json
{
    "version": 1,
    "appearance": {
        "font": {
            "family": "Monaco",
            "pointSize": 12
        },
        "fileSizeFormat": "auto",
        "dateTimeFormat": "yyyy/MM/dd HH:mm:ss",
        "colorRules": [
            {
                "pattern": "*.cpp",
                "foreground": "#ff0000ff",
                "background": "#00000000",
                "bold": false
            }
        ]
    },
    "behavior": {
        "restoreLastPath": true
    },
    "panes": {
        "left": {
            "path": "/Users/username",
            "sortKey": "name",
            "sortOrder": "ascending",
            "sortKey2nd": "name",
            "sortDirsType": "first",
            "sortDotFirst": true,
            "sortCaseSensitive": false,
            "nameFilters": [],
            "showHidden": false,
            "showSystem": false,
            "dirsOnly": false,
            "filesOnly": false
        },
        "right": { /* same structure */ }
    }
}
```

Keybindings are stored separately in QSettings under the key "keybindings/json":

```json
{
    "version": 1,
    "bindings": [
        {
            "key": "Up",
            "command": "navigate.up"
        },
        {
            "key": "Ctrl+Q",
            "command": "app.quit"
        }
    ]
}
```

## Integration with MainWindow

### Step 1: Initialize Systems in MainWindow Constructor

```cpp
// In MainWindow constructor, after setupUi()
MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_activePane(PaneType::Left)
  , m_singlePaneMode(false)
{
  setupUi();

  // Load settings
  Settings::instance().load();

  // Register commands
  registerCommands();

  // Load keybindings
  KeyBindingManager::instance().loadFromSettings();

  loadInitialPath();
}
```

### Step 2: Register Commands

Add a private method to MainWindow:

```cpp
// In MainWindow.h
private:
  void registerCommands();

// In MainWindow.cpp
void MainWindow::registerCommands() {
  auto& registry = CommandRegistry::instance();

  // Navigation commands
  registry.registerCommand(std::make_shared<NavigateUpCommand>(this));
  registry.registerCommand(std::make_shared<NavigateDownCommand>(this));
  registry.registerCommand(std::make_shared<NavigateLeftCommand>(this));
  registry.registerCommand(std::make_shared<NavigateRightCommand>(this));
  registry.registerCommand(std::make_shared<NavigateHomeCommand>(this));
  registry.registerCommand(std::make_shared<NavigateEndCommand>(this));
  registry.registerCommand(std::make_shared<NavigatePageUpCommand>(this));
  registry.registerCommand(std::make_shared<NavigatePageDownCommand>(this));
  registry.registerCommand(std::make_shared<NavigateEnterCommand>(this));
  registry.registerCommand(std::make_shared<NavigateParentCommand>(this));

  // Selection commands
  registry.registerCommand(std::make_shared<SelectToggleCommand>(this));
  registry.registerCommand(std::make_shared<SelectToggleAndDownCommand>(this));
  registry.registerCommand(std::make_shared<SelectInvertCommand>(this));
  registry.registerCommand(std::make_shared<SelectAllCommand>(this));

  // Pane commands
  registry.registerCommand(std::make_shared<PaneSwitchCommand>(this));
  registry.registerCommand(std::make_shared<PaneToggleSingleCommand>(this));

  // File operation commands
  registry.registerCommand(std::make_shared<FileCopyCommand>(this));
  registry.registerCommand(std::make_shared<FileMoveCommand>(this));
  registry.registerCommand(std::make_shared<FileDeleteCommand>(this));
  registry.registerCommand(std::make_shared<FileMkdirCommand>(this));

  // Application commands
  registry.registerCommand(std::make_shared<AppQuitCommand>(this));
  registry.registerCommand(std::make_shared<ViewFileCommand>(this));
}
```

### Step 3: Route Key Events Through KeyBindingManager

Modify MainWindow::keyPressEvent():

```cpp
void MainWindow::keyPressEvent(QKeyEvent* event) {
  // Try to find a keybinding for this key
  QKeySequence key(event->key() | event->modifiers());
  QString commandId = KeyBindingManager::instance().commandFor(key);

  if (!commandId.isEmpty()) {
    // Execute the command through the registry
    if (CommandRegistry::instance().execute(commandId)) {
      event->accept();
      return;
    }
  }

  // If no keybinding found or command failed, use default handling
  QMainWindow::keyPressEvent(event);
}
```

### Step 4: Implement Command Methods in MainWindow

Add public methods to MainWindow that commands can call:

```cpp
// In MainWindow.h
public:
  // Navigation
  void navigateUp();
  void navigateDown();
  void navigateLeft();
  void navigateRight();
  void navigateHome();
  void navigateEnd();
  void navigatePageUp();
  void navigatePageDown();
  void navigateEnter();
  void navigateParent();

  // Selection
  void selectToggle();
  void selectToggleAndDown();
  void selectInvert();
  void selectAll();

  // Pane
  void paneSwitch();
  void paneToggleSingle();

  // File operations
  void fileCopy();
  void fileMove();
  void fileDelete();
  void fileMkdir();

  // View
  void viewFile();
```

### Step 5: Update Command Implementations

Then update each command to call these methods. For example:

```cpp
// In NavigationCommands.cpp
void NavigateUpCommand::execute() {
  if (mainWindow()) {
    mainWindow()->navigateUp();
  }
}

void NavigateDownCommand::execute() {
  if (mainWindow()) {
    mainWindow()->navigateDown();
  }
}

// etc.
```

### Step 6: Save Settings on Exit

```cpp
MainWindow::~MainWindow() {
  // Save current pane settings
  Settings& settings = Settings::instance();
  settings.setPaneSettings(PaneType::Left, /* left pane settings */);
  settings.setPaneSettings(PaneType::Right, /* right pane settings */);
  settings.save();

  // Keybindings are saved automatically when changed,
  // but you can also save them here if needed
  KeyBindingManager::instance().saveToSettings();
}
```

## Adding New Commands

To add a new command:

1. Create a new class inheriting from BaseCommand (or ICommand)
2. Implement id(), label(), category(), and execute() methods
3. Add the .cpp file to CMakeLists.txt
4. Register the command in MainWindow::registerCommands()
5. Add default keybinding in KeyBindingManager::loadDefaults() if desired

Example:

```cpp
// In a new or existing command file
class RefreshCommand : public BaseCommand {
public:
  using BaseCommand::BaseCommand;

  QString id() const override { return "view.refresh"; }
  QString label() const override { return "Refresh"; }
  QString category() const override { return "view"; }

  void execute() override {
    if (mainWindow()) {
      mainWindow()->refresh();
    }
  }
};

// In KeyBindingManager::loadDefaults()
bind(QKeySequence(Qt::Key_F5), "view.refresh");

// In MainWindow::registerCommands()
registry.registerCommand(std::make_shared<RefreshCommand>(this));
```

## Using LambdaCommand for Quick Commands

For simple commands, you can use LambdaCommand instead of creating a new class:

```cpp
registry.registerCommand(std::make_shared<LambdaCommand>(
  "debug.toggle",
  "Toggle Debug Mode",
  [this]() {
    m_debugMode = !m_debugMode;
    qDebug() << "Debug mode:" << m_debugMode;
  },
  "debug"
));
```

## Testing the System

1. Build and run the application
2. Check the debug output to see commands being registered
3. Press keys and check that commands are executed (currently logs to qDebug)
4. Check settings file at the appropriate location for your platform
5. Modify keybindings in the JSON and restart to verify loading

## Next Steps

1. Implement the MainWindow methods called by commands
2. Connect command execution to actual UI functionality
3. Add settings UI for customizing keybindings
4. Add settings UI for appearance and behavior options
5. Implement proper error handling and user feedback
6. Add command history/undo functionality if needed
7. Consider adding command palette (Ctrl+P style) for command discovery

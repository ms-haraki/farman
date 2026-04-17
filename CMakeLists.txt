cmake_minimum_required(VERSION 3.21)
project(farman VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_add_executable(farman
  src/main.cpp
  src/ui/MainWindow.cpp
  src/ui/FileListPane.cpp
  src/ui/FileListDelegate.cpp
  src/ui/ProgressDialog.cpp
  src/core/FileItem.cpp
  src/core/workers/WorkerBase.cpp
  src/core/workers/CopyWorker.cpp
  src/core/workers/MoveWorker.cpp
  src/core/workers/RemoveWorker.cpp
  src/model/FileListModel.cpp
  src/viewer/ViewerDispatcher.cpp
  src/viewer/TextViewerWindow.cpp
  src/viewer/ImageViewerWindow.cpp
  src/viewer/TextViewerPlugin.cpp
  src/viewer/ImageViewerPlugin.cpp
  src/keybinding/ICommand.cpp
  src/keybinding/CommandRegistry.cpp
  src/keybinding/KeyBindingManager.cpp
  src/keybinding/commands/BaseCommand.cpp
  src/keybinding/commands/NavigationCommands.cpp
  src/keybinding/commands/SelectionCommands.cpp
  src/keybinding/commands/PaneCommands.cpp
  src/keybinding/commands/FileOperationCommands.cpp
  src/keybinding/commands/ApplicationCommands.cpp
  src/settings/Settings.cpp
)

target_include_directories(farman PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(farman PRIVATE
  Qt6::Core
  Qt6::Widgets
)

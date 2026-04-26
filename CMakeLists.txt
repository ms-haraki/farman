cmake_minimum_required(VERSION 3.21)
project(farman VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Core5Compat)

# Homebrew の libarchive は keg-only なので明示的にパスを通す
if(APPLE AND EXISTS /opt/homebrew/opt/libarchive)
  list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/libarchive)
endif()
find_package(LibArchive REQUIRED)

qt_add_executable(farman
  src/main.cpp
  src/ui/MainWindow.cpp
  src/ui/FileListPane.cpp
  src/ui/FileListDelegate.cpp
  src/ui/FileManagerPanel.cpp
  src/ui/ViewerPanel.cpp
  src/ui/ProgressDialog.cpp
  src/ui/SettingsDialog.cpp
  src/ui/KeybindingTab.cpp
  src/ui/AppearanceTab.cpp
  src/ui/BehaviorTab.cpp
  src/ui/ViewersTab.cpp
  src/ui/SortFilterDialog.cpp
  src/ui/TransferConfirmDialog.cpp
  src/ui/OverwriteDialog.cpp
  src/ui/DeleteConfirmDialog.cpp
  src/ui/AttributesDialog.cpp
  src/ui/BookmarkListDialog.cpp
  src/ui/BookmarkEditDialog.cpp
  src/ui/HistoryDialog.cpp
  src/ui/SearchDialog.cpp
  src/ui/CreateArchiveDialog.cpp
  src/ui/ExtractArchiveDialog.cpp
  src/ui/ClickableLabel.cpp
  src/core/FileItem.cpp
  src/core/BookmarkManager.cpp
  src/core/DirectoryHistory.cpp
  src/utils/Dialogs.cpp
  src/core/workers/WorkerBase.cpp
  src/core/workers/CopyWorker.cpp
  src/core/workers/MoveWorker.cpp
  src/core/workers/RemoveWorker.cpp
  src/core/workers/SearchWorker.cpp
  src/core/workers/ArchiveCreateWorker.cpp
  src/core/workers/ArchiveExtractWorker.cpp
  src/model/FileListModel.cpp
  src/viewer/ViewerDispatcher.cpp
  src/viewer/TextViewerWindow.cpp
  src/viewer/ImageViewerWindow.cpp
  src/viewer/BinaryView.cpp
  src/viewer/BinaryViewerWindow.cpp
  src/viewer/TextViewerPlugin.cpp
  src/viewer/ImageViewerPlugin.cpp
  src/viewer/BinaryViewerPlugin.cpp
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
  Qt6::Core5Compat
  LibArchive::LibArchive
)

# Test executable for Worker functionality
qt_add_executable(test_worker
  src/test_worker.cpp
  src/core/workers/WorkerBase.cpp
  src/core/workers/CopyWorker.cpp
  src/core/workers/MoveWorker.cpp
  src/ui/ProgressDialog.cpp
  src/utils/Dialogs.cpp
)

target_include_directories(test_worker PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_worker PRIVATE
  Qt6::Core
  Qt6::Widgets
)

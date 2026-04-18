#include "KeybindingTab.h"
#include "keybinding/KeyBindingManager.h"
#include "keybinding/CommandRegistry.h"
#include "keybinding/ICommand.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QLabel>
#include <QFrame>

namespace Farman {

KeybindingTab::KeybindingTab(QWidget* parent)
  : QWidget(parent)
  , m_table(nullptr)
  , m_resetButton(nullptr)
  , m_clearButton(nullptr)
  , m_recordFrame(nullptr)
  , m_recordCommandLabel(nullptr)
  , m_recordCurrentKeyLabel(nullptr)
  , m_recordNewKeyLabel(nullptr)
  , m_recordOkButton(nullptr)
  , m_recordCancelButton(nullptr)
  , m_editingRow(-1)
  , m_inRecordMode(false) {
  setupUi();
  loadKeybindings();
}

void KeybindingTab::setupUi() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);

  // Info label
  QLabel* tableInfoLabel = new QLabel(tr("Press Enter or double-click to change a keybinding."), this);
  tableInfoLabel->setWordWrap(true);
  mainLayout->addWidget(tableInfoLabel);

  // Table
  m_table = new QTableWidget(this);
  m_table->setColumnCount(3);
  m_table->setHorizontalHeaderLabels({tr("Command"), tr("Current Key"), tr("New Key")});
  m_table->horizontalHeader()->setStretchLastSection(false);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // Command column stretches
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);   // Current Key fixed width
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);   // New Key fixed width
  m_table->horizontalHeader()->resizeSection(1, 200); // Current Key width
  m_table->horizontalHeader()->resizeSection(2, 200); // New Key width (same as Current Key)
  m_table->verticalHeader()->setVisible(false);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Install event filter on table for Enter key
  m_table->installEventFilter(this);

  connect(m_table, &QTableWidget::cellDoubleClicked, this, &KeybindingTab::onCellDoubleClicked);

  mainLayout->addWidget(m_table);

  // Key recording frame (initially hidden, shown below table when editing)
  m_recordFrame = new QFrame(this);
  m_recordFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  m_recordFrame->setVisible(false);

  QVBoxLayout* recordLayout = new QVBoxLayout(m_recordFrame);
  recordLayout->setSpacing(16);
  recordLayout->setContentsMargins(16, 16, 16, 16);

  // Command name section
  QHBoxLayout* commandLayout = new QHBoxLayout();
  QLabel* commandTitleLabel = new QLabel(tr("Command:"), m_recordFrame);
  QFont titleFont = commandTitleLabel->font();
  titleFont.setBold(true);
  commandTitleLabel->setFont(titleFont);
  commandLayout->addWidget(commandTitleLabel);

  m_recordCommandLabel = new QLabel(m_recordFrame);
  m_recordCommandLabel->setWordWrap(true);
  commandLayout->addWidget(m_recordCommandLabel, 1);
  recordLayout->addLayout(commandLayout);

  // Key binding section (old -> new)
  QFont monoFont("Monospace");
  monoFont.setStyleHint(QFont::TypeWriter);

  QHBoxLayout* keyLayout = new QHBoxLayout();

  // Current key
  m_recordCurrentKeyLabel = new QLabel(m_recordFrame);
  m_recordCurrentKeyLabel->setStyleSheet("padding: 10px; background-color: palette(light); border-radius: 4px;");
  m_recordCurrentKeyLabel->setFont(monoFont);
  m_recordCurrentKeyLabel->setAlignment(Qt::AlignCenter);
  m_recordCurrentKeyLabel->setMinimumWidth(150);
  keyLayout->addWidget(m_recordCurrentKeyLabel);

  // Spacer before arrow
  keyLayout->addStretch(1);

  // Arrow
  QLabel* arrowLabel = new QLabel(tr("→"), m_recordFrame);
  QFont arrowFont = arrowLabel->font();
  arrowFont.setPointSize(arrowFont.pointSize() + 4);
  arrowLabel->setFont(arrowFont);
  keyLayout->addWidget(arrowLabel);

  // Spacer after arrow
  keyLayout->addStretch(1);

  // New key
  m_recordNewKeyLabel = new QLabel(tr("<press a key>"), m_recordFrame);
  m_recordNewKeyLabel->setStyleSheet("padding: 12px; background-color: palette(highlight); color: palette(highlighted-text); border-radius: 4px; font-weight: bold;");
  QFont newKeyFont = monoFont;
  newKeyFont.setPointSize(newKeyFont.pointSize() + 2);
  newKeyFont.setBold(true);
  m_recordNewKeyLabel->setFont(newKeyFont);
  m_recordNewKeyLabel->setAlignment(Qt::AlignCenter);
  m_recordNewKeyLabel->setMinimumWidth(150);
  keyLayout->addWidget(m_recordNewKeyLabel);

  recordLayout->addLayout(keyLayout);

  // Buttons
  QHBoxLayout* recordButtonLayout = new QHBoxLayout();

#ifdef Q_OS_MACOS
  m_recordCancelButton = new QPushButton(tr("Cancel (Cmd+C)"), m_recordFrame);
  m_recordOkButton = new QPushButton(tr("OK (Enter)"), m_recordFrame);
#else
  m_recordCancelButton = new QPushButton(tr("Cancel (Ctrl+C)"), m_recordFrame);
  m_recordOkButton = new QPushButton(tr("OK (Enter)"), m_recordFrame);
#endif

  connect(m_recordCancelButton, &QPushButton::clicked, this, &KeybindingTab::onRecordCancel);
  connect(m_recordOkButton, &QPushButton::clicked, this, &KeybindingTab::onRecordOk);

  recordButtonLayout->addStretch();
  recordButtonLayout->addWidget(m_recordCancelButton);
  recordButtonLayout->addWidget(m_recordOkButton);

  recordLayout->addLayout(recordButtonLayout);

  // Install event filter on record frame to capture keys
  m_recordFrame->installEventFilter(this);

  mainLayout->addWidget(m_recordFrame);

  // Buttons
  QHBoxLayout* buttonLayout = new QHBoxLayout();

  m_clearButton = new QPushButton(tr("Clear Binding"), this);
  m_clearButton->setToolTip(tr("Remove the keybinding for the selected command"));
  connect(m_clearButton, &QPushButton::clicked, this, &KeybindingTab::onClearBinding);
  buttonLayout->addWidget(m_clearButton);

  m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
  m_resetButton->setToolTip(tr("Reset all keybindings to their default values"));
  connect(m_resetButton, &QPushButton::clicked, this, &KeybindingTab::onResetToDefaults);
  buttonLayout->addWidget(m_resetButton);

  buttonLayout->addStretch();

  mainLayout->addLayout(buttonLayout);
}

void KeybindingTab::loadKeybindings() {
  m_pendingChanges.clear();
  updateTable();

  // Select first row by default
  if (m_table->rowCount() > 0) {
    m_table->selectRow(0);
    m_table->setFocus();
  }
}

void KeybindingTab::updateTable() {
  m_table->setRowCount(0);

  auto& registry = CommandRegistry::instance();
  auto& keyManager = KeyBindingManager::instance();

  QList<ICommand*> commands = registry.allCommands();

  // Sort commands by category and then by label
  std::sort(commands.begin(), commands.end(), [](ICommand* a, ICommand* b) {
    if (a->category() != b->category()) {
      return a->category() < b->category();
    }
    return a->label() < b->label();
  });

  m_table->setRowCount(commands.size());

  for (int i = 0; i < commands.size(); ++i) {
    ICommand* cmd = commands[i];
    QString commandId = cmd->id();

    // Command name (with category)
    QString displayName = QString("%1 / %2").arg(cmd->category(), cmd->label());
    QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);
    nameItem->setData(Qt::UserRole, commandId); // Store command ID
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(i, 0, nameItem);

    // Current key
    QList<QKeySequence> keys = keyManager.keysForCommand(commandId);
    QString currentKey;
    if (!keys.isEmpty()) {
      currentKey = keySequenceToString(keys.first());
      if (keys.size() > 1) {
        currentKey += tr(" (and %1 more)").arg(keys.size() - 1);
      }
    }
    QTableWidgetItem* currentItem = new QTableWidgetItem(currentKey);
    currentItem->setFlags(currentItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(i, 1, currentItem);

    // New key (pending changes)
    QString newKey;
    if (m_pendingChanges.contains(commandId)) {
      QKeySequence seq = m_pendingChanges[commandId];
      if (!seq.isEmpty()) {
        newKey = keySequenceToString(seq);
      } else {
        newKey = tr("(unbound)");
      }
    }
    QTableWidgetItem* newItem = new QTableWidgetItem(newKey);
    newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
    if (!newKey.isEmpty()) {
      QFont font = newItem->font();
      font.setBold(true);
      newItem->setFont(font);
    }
    m_table->setItem(i, 2, newItem);
  }

  // Don't call resizeColumnsToContents() as we have fixed column sizes
}

QString KeybindingTab::keySequenceToString(const QKeySequence& key) const {
  return key.toString(QKeySequence::NativeText);
}

void KeybindingTab::onCellDoubleClicked(int row, int column) {
  Q_UNUSED(column);

  if (row < 0 || row >= m_table->rowCount()) {
    return;
  }

  startRecording(row);
}

void KeybindingTab::startRecording(int row) {
  if (row < 0 || row >= m_table->rowCount()) {
    return;
  }

  QTableWidgetItem* nameItem = m_table->item(row, 0);
  if (!nameItem) {
    return;
  }

  m_editingCommandId = nameItem->data(Qt::UserRole).toString();
  m_editingRow = row;
  m_inRecordMode = true;
  m_recordedKey = QKeySequence();

  // Get current key binding
  auto& keyManager = KeyBindingManager::instance();
  QList<QKeySequence> keys = keyManager.keysForCommand(m_editingCommandId);
  QString currentKey = tr("(unbound)");
  if (!keys.isEmpty()) {
    currentKey = keySequenceToString(keys.first());
  }

  // Update record frame labels
  m_recordCommandLabel->setText(nameItem->text());
  m_recordCurrentKeyLabel->setText(currentKey);
  m_recordNewKeyLabel->setText(tr("<press a key>"));

  // Hide buttons and show record frame
  m_clearButton->setVisible(false);
  m_resetButton->setVisible(false);
  m_recordFrame->setVisible(true);
  m_recordFrame->setFocus();
}

bool KeybindingTab::eventFilter(QObject* obj, QEvent* event) {
  // Handle Enter key on table to start recording
  if (obj == m_table && event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
      int row = m_table->currentRow();
      if (row >= 0) {
        startRecording(row);
        return true;
      }
    }

    // Let other keys pass through for normal table navigation
    return QWidget::eventFilter(obj, event);
  }

  // Handle key presses in the record frame
  if (obj == m_recordFrame && event->type() == QEvent::KeyPress && m_inRecordMode) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    // Ignore modifier-only keys
    if (keyEvent->key() == Qt::Key_Control ||
        keyEvent->key() == Qt::Key_Shift ||
        keyEvent->key() == Qt::Key_Alt ||
        keyEvent->key() == Qt::Key_Meta) {
      return QWidget::eventFilter(obj, event);
    }

    // Confirm on Enter (without modifiers)
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
      if (keyEvent->modifiers() == Qt::NoModifier ||
          keyEvent->modifiers() == Qt::KeypadModifier) {
        // Validate and apply the recorded key
        if (!m_recordedKey.isEmpty()) {
          if (validateBinding(m_recordedKey, m_editingCommandId)) {
            m_pendingChanges[m_editingCommandId] = m_recordedKey;
            updateTable();
          }
        }
        // Hide record frame and show buttons
        m_recordFrame->setVisible(false);
        m_clearButton->setVisible(true);
        m_resetButton->setVisible(true);
        m_inRecordMode = false;
        m_recordedKey = QKeySequence();
        // Return focus to table
        m_table->setFocus();
        if (m_editingRow >= 0 && m_editingRow < m_table->rowCount()) {
          m_table->selectRow(m_editingRow);
        }
        return true;
      }
    }

    // Cancel on Cmd+C (macOS) or Ctrl+C (others), or Cmd+G / Ctrl+G
    // Note: On macOS, Qt maps Command key to ControlModifier by default
    if (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_G) {
      Qt::KeyboardModifiers mods = keyEvent->modifiers() & ~Qt::KeypadModifier;
      // On all platforms including macOS, check for ControlModifier
      // (on macOS, Command key is mapped to ControlModifier)
      if (mods == Qt::ControlModifier) {
        // Cancel recording
        m_recordFrame->setVisible(false);
        m_clearButton->setVisible(true);
        m_resetButton->setVisible(true);
        m_inRecordMode = false;
        m_recordedKey = QKeySequence();
        // Return focus to table
        m_table->setFocus();
        if (m_editingRow >= 0 && m_editingRow < m_table->rowCount()) {
          m_table->selectRow(m_editingRow);
        }
        return true;
      }
    }

    // Create key sequence
    int key = keyEvent->key();
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

    // Remove the keypad modifier as it's not needed
    modifiers &= ~Qt::KeypadModifier;

    QKeySequence seq(key | modifiers);

    // Store the recorded key
    m_recordedKey = seq;

    // Update the label to show what was pressed
    m_recordNewKeyLabel->setText(keySequenceToString(seq));

    return true;
  }

  return QWidget::eventFilter(obj, event);
}

bool KeybindingTab::validateBinding(const QKeySequence& newKey, const QString& commandId) {
  if (newKey.isEmpty()) {
    return true;
  }

  auto& keyManager = KeyBindingManager::instance();

  // Check if this key is already bound to another command
  QString existingCommand = keyManager.commandFor(newKey);

  // Check pending changes too
  for (auto it = m_pendingChanges.begin(); it != m_pendingChanges.end(); ++it) {
    if (it.value() == newKey && it.key() != commandId) {
      existingCommand = it.key();
      break;
    }
  }

  if (!existingCommand.isEmpty() && existingCommand != commandId) {
    auto& registry = CommandRegistry::instance();
    ICommand* cmd = registry.command(existingCommand);
    QString cmdLabel = cmd ? cmd->label() : existingCommand;

    QMessageBox::StandardButton reply = QMessageBox::question(
      this,
      tr("Key Already Bound"),
      tr("The key '%1' is already bound to '%2'.\nDo you want to rebind it to this command?")
        .arg(keySequenceToString(newKey), cmdLabel),
      QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
      return false;
    }

    // Unbind the old command
    m_pendingChanges[existingCommand] = QKeySequence();
  }

  return true;
}

void KeybindingTab::onClearBinding() {
  QList<QTableWidgetItem*> selected = m_table->selectedItems();
  if (selected.isEmpty()) {
    QMessageBox::information(this, tr("No Selection"),
                            tr("Please select a command to clear its binding."));
    return;
  }

  int row = m_table->currentRow();
  if (row < 0) {
    return;
  }

  QTableWidgetItem* nameItem = m_table->item(row, 0);
  if (!nameItem) {
    return;
  }

  QString commandId = nameItem->data(Qt::UserRole).toString();

  // Set to empty sequence (unbound)
  m_pendingChanges[commandId] = QKeySequence();

  updateTable();
}

void KeybindingTab::onResetToDefaults() {
  QMessageBox::StandardButton reply = QMessageBox::question(
    this,
    tr("Reset to Defaults"),
    tr("Are you sure you want to reset all keybindings to their default values?\nThis will discard all custom keybindings."),
    QMessageBox::Yes | QMessageBox::No
  );

  if (reply == QMessageBox::Yes) {
    m_pendingChanges.clear();

    // Load defaults temporarily to show in table
    auto& keyManager = KeyBindingManager::instance();
    keyManager.loadDefaults();

    updateTable();
  }
}

void KeybindingTab::onRecordOk() {
  if (!m_inRecordMode) {
    return;
  }

  // Validate and apply the recorded key
  if (!m_recordedKey.isEmpty()) {
    if (validateBinding(m_recordedKey, m_editingCommandId)) {
      m_pendingChanges[m_editingCommandId] = m_recordedKey;
      updateTable();
    }
  }

  // Hide record frame and show buttons
  m_recordFrame->setVisible(false);
  m_clearButton->setVisible(true);
  m_resetButton->setVisible(true);
  m_inRecordMode = false;
  m_recordedKey = QKeySequence();

  // Return focus to table
  m_table->setFocus();
  if (m_editingRow >= 0 && m_editingRow < m_table->rowCount()) {
    m_table->selectRow(m_editingRow);
  }
}

void KeybindingTab::onRecordCancel() {
  if (!m_inRecordMode) {
    return;
  }

  // Cancel recording
  m_recordFrame->setVisible(false);
  m_clearButton->setVisible(true);
  m_resetButton->setVisible(true);
  m_inRecordMode = false;
  m_recordedKey = QKeySequence();

  // Return focus to table
  m_table->setFocus();
  if (m_editingRow >= 0 && m_editingRow < m_table->rowCount()) {
    m_table->selectRow(m_editingRow);
  }
}

void KeybindingTab::save() {
  if (m_pendingChanges.isEmpty()) {
    return;
  }

  auto& keyManager = KeyBindingManager::instance();

  // Apply all pending changes
  for (auto it = m_pendingChanges.begin(); it != m_pendingChanges.end(); ++it) {
    QString commandId = it.key();
    QKeySequence newKey = it.value();

    // First, remove all existing bindings for this command
    QList<QKeySequence> oldKeys = keyManager.keysForCommand(commandId);
    for (const QKeySequence& oldKey : oldKeys) {
      keyManager.unbind(oldKey);
    }

    // Then bind the new key (if not empty)
    if (!newKey.isEmpty()) {
      keyManager.bind(newKey, commandId);
    }
  }

  m_pendingChanges.clear();
}

} // namespace Farman

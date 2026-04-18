#pragma once

#include <QWidget>
#include <QMap>
#include <QKeySequence>

class QTableWidget;
class QPushButton;
class QTableWidgetItem;
class QFrame;
class QLabel;

namespace Farman {

class KeybindingTab : public QWidget {
  Q_OBJECT

public:
  explicit KeybindingTab(QWidget* parent = nullptr);
  ~KeybindingTab() override = default;

  void save();

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onCellDoubleClicked(int row, int column);
  void onResetToDefaults();
  void onClearBinding();
  void onRecordOk();
  void onRecordCancel();

private:
  void setupUi();
  void loadKeybindings();
  void updateTable();
  QString keySequenceToString(const QKeySequence& key) const;
  bool validateBinding(const QKeySequence& newKey, const QString& commandId);
  void startRecording(int row);

  QTableWidget* m_table;
  QPushButton*  m_resetButton;
  QPushButton*  m_clearButton;

  // Key recording frame (shown below table when editing)
  QFrame*       m_recordFrame;
  QLabel*       m_recordCommandLabel;
  QLabel*       m_recordCurrentKeyLabel;
  QLabel*       m_recordNewKeyLabel;
  QPushButton*  m_recordOkButton;
  QPushButton*  m_recordCancelButton;

  // Track pending changes: commandId -> new key sequence
  QMap<QString, QKeySequence> m_pendingChanges;

  // Track the command being edited
  QString m_editingCommandId;
  int m_editingRow;

  // Flag to indicate we're in recording mode
  bool m_inRecordMode;
  QKeySequence m_recordedKey;
};

} // namespace Farman

#include "ShortcutListDialog.h"
#include "keybinding/CommandLayout.h"
#include "keybinding/CommandRegistry.h"
#include "keybinding/KeyBindingManager.h"
#include "settings/Settings.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace Farman {

namespace {

QString keysToText(const QList<QKeySequence>& keys) {
  // 複数バインドがあるときは "C, Ctrl+C" のようにカンマ区切り。
  // ネイティブ表記を使うので macOS では ⌘C / Win/Linux では Ctrl+C 表示。
  // テンキー (Numpad) の Enter は Key_Return とペアで登録するのが慣例
  // (例: file.execute / view.choose) で表示が冗長になるので、一覧からは
  // 除外する。バインド自体は有効。
  QStringList parts;
  for (const auto& k : keys) {
    bool isNumpadEnter = false;
    for (int i = 0; i < k.count(); ++i) {
      if (k[i].key() == Qt::Key_Enter) { isNumpadEnter = true; break; }
    }
    if (isNumpadEnter) continue;

    QString s = k.toString(QKeySequence::NativeText);
    // macOS の NativeText は Home/End/PageUp/PageDown を斜め矢印で出して
    // 区別しづらいので、わかりやすい文字列に置換する。
    s.replace(QChar(0x2196), QStringLiteral("Home"));   // ↖
    s.replace(QChar(0x2198), QStringLiteral("End"));    // ↘
    s.replace(QChar(0x21DE), QStringLiteral("PgUp"));   // ⇞
    s.replace(QChar(0x21DF), QStringLiteral("PgDn"));   // ⇟

    if (!s.isEmpty()) parts << s;
  }
  return parts.isEmpty() ? QStringLiteral("—") : parts.join(QStringLiteral(", "));
}

} // namespace

ShortcutListDialog::ShortcutListDialog(QWidget* parent)
  : QDialog(parent) {
  // モードレスダイアログ。閉じるまで親ウィンドウの操作を妨げない。
  setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
  setWindowTitle(tr("Keyboard Shortcuts"));
  setModal(false);
  resize(640, 720);
  setupUi();
  rebuild();

  // Settings で keybindings が変わったら追従
  connect(&Settings::instance(), &Settings::settingsChanged,
          this, &ShortcutListDialog::rebuild);
}

void ShortcutListDialog::setupUi() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);

  m_table = new QTableWidget(this);
  m_table->setColumnCount(2);
  m_table->setHorizontalHeaderLabels({ tr("Key"), tr("Command") });
  m_table->verticalHeader()->setVisible(false);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setShowGrid(false);
  // カテゴリ見出し行に独自背景色を設定するため alternating は OFF
  // (alternating ON だとセル背景と競合してチラつく)
  m_table->setAlternatingRowColors(false);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_table->setColumnWidth(0, 150);
  layout->addWidget(m_table);
}

void ShortcutListDialog::rebuild() {
  if (!m_table) return;

  m_table->setRowCount(0);
  m_table->setUpdatesEnabled(false);

  const auto groups = commandsGroupedByCategory(CommandRegistry::instance());
  for (const auto& group : groups) {
    // 見出し行 (1 セルを 2 列にまたがらせる)
    const int hdrRow = m_table->rowCount();
    m_table->insertRow(hdrRow);
    auto* hdr = new QTableWidgetItem(group.display);
    QFont f = hdr->font();
    f.setBold(true);
    hdr->setFont(f);
    // 通常セルとはっきり差別化されるよう、Mid 系の濃いめの背景色を使う。
    // ライト/ダーク両テーマの palette に追従させる。
    hdr->setBackground(palette().color(QPalette::Mid));
    hdr->setForeground(palette().color(QPalette::WindowText));
    hdr->setFlags(Qt::ItemIsEnabled);  // 選択不可
    m_table->setItem(hdrRow, 0, hdr);
    m_table->setSpan(hdrRow, 0, 1, 2);

    // 各コマンド行 (登録順)
    for (ICommand* cmd : group.commands) {
      const int row = m_table->rowCount();
      m_table->insertRow(row);

      const auto keys = KeyBindingManager::instance().keysForCommand(cmd->id());
      auto* keyItem  = new QTableWidgetItem(keysToText(keys));
      auto* nameItem = new QTableWidgetItem(cmd->label());

      keyItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      keyItem->setToolTip(cmd->id());
      if (!cmd->description().isEmpty()) {
        nameItem->setToolTip(cmd->description());
      }

      m_table->setItem(row, 0, keyItem);
      m_table->setItem(row, 1, nameItem);
    }
  }

  m_table->setUpdatesEnabled(true);
}

void ShortcutListDialog::keyPressEvent(QKeyEvent* event) {
  // `?` でトグル閉じ、Esc でも閉じる。
  if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Question) {
    close();
    event->accept();
    return;
  }
  QDialog::keyPressEvent(event);
}

} // namespace Farman

#pragma once

#include "WorkerBase.h"
#include <QStringList>

namespace Farman {

// 指定したアーカイブ内のエントリ群だけを展開するワーカー。
// 通常 FS から「アーカイブ内 → 反対パネル」へのコピー (Phase C) で使う。
//
// 引数の `currentInnerDir` は「ユーザーがいま見ているアーカイブ内ディレクトリ」
// (先頭・末尾の '/' なし、ルートは空文字)。展開先の階層計算でこの prefix を
// 剥がす:
//   user@ "/inner" で hello.txt を選んだ場合
//     selectedFiles = ["inner/hello.txt"], currentInnerDir = "inner"
//     → destDir/hello.txt
//   user@ "/" で inner/ ディレクトリを選んだ場合
//     selectedDirs = ["inner"], currentInnerDir = ""
//     → destDir/inner/hello.txt, destDir/inner/sub/world.txt
//
// これにより、通常 FS のコピー感覚 (「カレントを基準とした相対パスで dest に
// 並ぶ」) と一致する挙動になる。
class ArchiveExtractEntriesWorker : public WorkerBase {
  Q_OBJECT

public:
  // filesTotal: 進捗ダイアログの「N / M files」表示用に、呼び出し側で事前に
  // 数えた展開対象エントリ件数を渡す。<= 0 のときは indeterminate モード。
  // password: 暗号化アーカイブの場合のパスワード。空文字なら無視。
  ArchiveExtractEntriesWorker(const QString&     archivePath,
                              const QStringList& selectedFiles,
                              const QStringList& selectedDirs,
                              const QString&     currentInnerDir,
                              const QString&     destDir,
                              int                filesTotal,
                              const QString&     password = QString(),
                              QObject*           parent = nullptr);

protected:
  void run() override;

private:
  QString     m_archivePath;
  QStringList m_selectedFiles;
  QStringList m_selectedDirs;
  // ユーザーが見ているアーカイブ内のカレントディレクトリ (先頭/末尾 '/' なし)。
  // 展開先の階層計算でこの prefix を剥がす。ルートのときは空。
  QString     m_currentInnerDir;
  QString     m_destDir;
  // 進捗ダイアログの「N / M files」表示用。<= 0 = indeterminate。
  int         m_filesTotal;
  // 暗号化アーカイブの場合のパスワード。空文字 = 暗号化なし or 未設定。
  QString     m_password;
};

} // namespace Farman

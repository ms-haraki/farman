#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include "core/workers/CopyWorker.h"
#include "core/workers/MoveWorker.h"
#include "ui/ProgressDialog.h"

using namespace Farman;

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  qDebug() << "=== Worker Test Program ===";

  // テスト用ディレクトリ準備
  QString testDir = QDir::tempPath() + "/farman_test";
  QString srcDir = testDir + "/src";
  QString dstDir = testDir + "/dst";

  QDir().mkpath(srcDir);
  QDir().mkpath(dstDir);

  qDebug() << "Test directory:" << testDir;
  qDebug() << "Source directory:" << srcDir;
  qDebug() << "Destination directory:" << dstDir;

  // テストファイル作成
  QStringList testFiles;

  // 1. 小さいファイル (1MB)
  QString smallFile = srcDir + "/small_file.dat";
  QFile small(smallFile);
  if (small.open(QIODevice::WriteOnly)) {
    small.write(QByteArray(1 * 1024 * 1024, 'S')); // 1MB
    small.close();
    testFiles << smallFile;
    qDebug() << "Created small file (1MB):" << smallFile;
  }

  // 2. 中サイズファイル (10MB)
  QString mediumFile = srcDir + "/medium_file.dat";
  QFile medium(mediumFile);
  if (medium.open(QIODevice::WriteOnly)) {
    medium.write(QByteArray(10 * 1024 * 1024, 'M')); // 10MB
    medium.close();
    testFiles << mediumFile;
    qDebug() << "Created medium file (10MB):" << mediumFile;
  }

  // 3. 大きいファイル (100MB) - キャンセルテスト用
  QString largeFile = srcDir + "/large_file.dat";
  QFile large(largeFile);
  if (large.open(QIODevice::WriteOnly)) {
    // 10MBずつ書き込んで100MBに
    QByteArray chunk(10 * 1024 * 1024, 'L');
    for (int i = 0; i < 10; i++) {
      large.write(chunk);
    }
    large.close();
    testFiles << largeFile;
    qDebug() << "Created large file (100MB):" << largeFile;
  }

  qDebug() << "Total test files:" << testFiles.size();

  // ユーザーに選択肢を表示
  QMessageBox msgBox;
  msgBox.setWindowTitle("Worker Test");
  msgBox.setText("Which test do you want to run?");
  msgBox.setInformativeText(
    "Test files created in: " + srcDir + "\n\n"
    "Small: 1MB\n"
    "Medium: 10MB\n"
    "Large: 100MB (good for cancel test)"
  );

  QPushButton* copyBtn = msgBox.addButton("Copy Test", QMessageBox::ActionRole);
  QPushButton* moveBtn = msgBox.addButton("Move Test", QMessageBox::ActionRole);
  QPushButton* cancelBtn = msgBox.addButton("Exit", QMessageBox::RejectRole);

  msgBox.exec();

  if (msgBox.clickedButton() == copyBtn) {
    qDebug() << "Starting CopyWorker test...";

    CopyWorker* worker = new CopyWorker(testFiles, dstDir);
    ProgressDialog* dialog = new ProgressDialog("Copying test files", nullptr);
    dialog->setWindowTitle("Copy Test - Try canceling during large file copy!");
    dialog->setWorker(worker);

    worker->start();
    int result = dialog->exec();

    if (result == QDialog::Accepted) {
      qDebug() << "Copy completed successfully!";
      QMessageBox::information(nullptr, "Success",
        "Copy test completed!\n\nCheck destination: " + dstDir);
    } else {
      qDebug() << "Copy was cancelled";
      QMessageBox::information(nullptr, "Cancelled", "Copy operation was cancelled.");
    }

    delete dialog;
    delete worker;

  } else if (msgBox.clickedButton() == moveBtn) {
    qDebug() << "Starting MoveWorker test...";

    // Move用に別のディレクトリを用意
    QString moveDstDir = testDir + "/moved";
    QDir().mkpath(moveDstDir);

    MoveWorker* worker = new MoveWorker(testFiles, moveDstDir);
    ProgressDialog* dialog = new ProgressDialog("Moving test files", nullptr);
    dialog->setWindowTitle("Move Test - Try canceling during large file move!");
    dialog->setWorker(worker);

    worker->start();
    int result = dialog->exec();

    if (result == QDialog::Accepted) {
      qDebug() << "Move completed successfully!";
      QMessageBox::information(nullptr, "Success",
        "Move test completed!\n\nCheck destination: " + moveDstDir);
    } else {
      qDebug() << "Move was cancelled";
      QMessageBox::information(nullptr, "Cancelled", "Move operation was cancelled.");
    }

    delete dialog;
    delete worker;

  } else {
    qDebug() << "Test cancelled by user";
  }

  // クリーンアップ確認
  QMessageBox cleanupBox;
  cleanupBox.setWindowTitle("Cleanup");
  cleanupBox.setText("Delete test directory?");
  cleanupBox.setInformativeText(testDir);
  cleanupBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  cleanupBox.setDefaultButton(QMessageBox::No);

  if (cleanupBox.exec() == QMessageBox::Yes) {
    QDir(testDir).removeRecursively();
    qDebug() << "Test directory removed";
  } else {
    qDebug() << "Test directory preserved for inspection";
  }

  return 0;
}

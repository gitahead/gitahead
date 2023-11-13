//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "ui/FileList.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QTextEdit>
#include <QTextStream>

using namespace Test;
using namespace QTest;

class TestIndex : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void stageAddition();
  void stageDeletion();
  void stageDirectory();
  void cleanupTestCase();

private:
  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestIndex::initTestCase()
{
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));
}

void TestIndex::stageAddition()
{
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex index = model->index(0, 0);
  QVERIFY(!index.data(Qt::CheckStateRole).toBool());

  // Click on the check box.
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Check that it's staged now.
  QVERIFY(index.data(Qt::CheckStateRole).toBool());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("addition");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDeletion()
{
  // Remove file and refresh.
  mRepo->workdir().remove("test");

  RepoView *view = mWindow->currentView();
  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex index = model->index(0, 0);
  QVERIFY(!index.data(Qt::CheckStateRole).toBool());

  // Click on the check box.
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Check that it's staged now.
  QVERIFY(index.data(Qt::CheckStateRole).toBool());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("deletion");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDirectory()
{
  QDir dir = mRepo->workdir();
  dir.mkdir("dir");
  QVERIFY(dir.cd("dir"));

  QFile file1(dir.filePath("test1"));
  QVERIFY(file1.open(QFile::WriteOnly));
  QTextStream(&file1) << "This is a test." << Qt::endl;

  QFile file2(dir.filePath("test2"));
  QVERIFY(file2.open(QFile::WriteOnly));
  QTextStream(&file2) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex index = model->index(0, 0);
  QVERIFY(!index.data(Qt::CheckStateRole).toBool());

  // Setup post refresh trigger.
  bool finished = false;
  auto connection = QObject::connect(view, &RepoView::statusChanged,
  [&finished](bool dirty) {
    QVERIFY(dirty);
    finished = true;
  });

  // Set up timer to dismiss the prompt.
  QTimer::singleShot(0, [] {
    QWidget *prompt = QApplication::activeModalWidget();
    QVERIFY(prompt && qWaitForWindowActive(prompt));

    keyClick(prompt, Qt::Key_Return);
  });

  // Click on the check box.
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Wait for the refresh to finish.
  while (!finished)
    qWait(100);

  QObject::disconnect(connection);

  // Check for two staged files.
  QCOMPARE(model->rowCount(), 2);

  QModelIndex index1 = model->index(0, 0);
  QVERIFY(index1.data(Qt::CheckStateRole).toBool());

  QModelIndex index2 = model->index(1, 0);
  QVERIFY(index2.data(Qt::CheckStateRole).toBool());
}

void TestIndex::cleanupTestCase()
{
  mWindow->close();
}

TEST_MAIN(TestIndex)

#include "index.moc"

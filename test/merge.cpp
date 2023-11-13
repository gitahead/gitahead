//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "ui/FileList.h"
#include "ui/MainWindow.h"
#include "ui/DetailView.h"
#include "ui/DiffView.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestMerge : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void firstCommit();
  void secondCommit();
  void thirdCommit();
  void mergeConflict();
  void resolve();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestMerge::initTestCase()
{
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));
}

void TestMerge::firstCommit()
{
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This will be a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("base commit");
  view->commit();
  refresh(view, false);
}

void TestMerge::secondCommit()
{
  RepoView *view = mWindow->currentView();
  git::Branch branch = mRepo->createBranch("branch2", mRepo->head().target());
  QVERIFY(branch.isValid());

  view->checkout(branch);
  QCOMPARE(mRepo->head().name(), QString("branch2"));

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a conflict." << Qt::endl;

  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicting commit b");
  view->commit();
  refresh(view, false);

}

void TestMerge::thirdCommit()
{
  RepoView *view = mWindow->currentView();
  git::Reference ref = mRepo->lookupRef("refs/heads/master");
  QVERIFY(ref);

  view->checkout(ref);
  QCOMPARE(mRepo->head().name(), QString("master"));

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << Qt::endl;

  refresh(view);

  FileList *files = view->findChild<FileList *>();
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(
    files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
    files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicting commit a");
  view->commit();
  refresh(view, false);
}

void TestMerge::mergeConflict()
{
  RepoView *view = mWindow->currentView();
  git::Reference master = mRepo->lookupRef("refs/heads/master");
  QVERIFY(master);

  git::Reference branch2 = mRepo->lookupRef("refs/heads/branch2");
  QVERIFY(branch2);

  QCOMPARE(mRepo->head().name(), QString("master"));

  view->merge(RepoView::Merge, branch2);

  // Diff is in a conflicted state
  git::Diff diff = mRepo->diffIndexToWorkdir();
  QVERIFY(diff.isConflicted());
}

void TestMerge::resolve()
{
  RepoView *view = mWindow->currentView();
  DiffView *diffView = view->findChild<DiffView *>();

  QToolButton *theirs = nullptr;
  while (theirs == nullptr) {
    theirs = diffView->findChild<QToolButton *>("ConflictTheirs");
    qWait(100);
  }
  mouseClick(theirs, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), inputDelay);

  QToolButton *undo = diffView->widget()->findChild<QToolButton *>("ConflictUndo");
  mouseClick(undo, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), inputDelay);

  QToolButton *ours = diffView->widget()->findChild<QToolButton *>("ConflictOurs");
  mouseClick(ours, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), inputDelay);

  QToolButton *save = diffView->widget()->findChild<QToolButton *>("ConflictSave");
  mouseClick(save, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), inputDelay);

  DetailView *detailView = view->findChild<DetailView *>();
  QPushButton *stageAll = nullptr;
  while (stageAll == nullptr) {
    stageAll = detailView->findChild<QPushButton *>("StageAll");
    qWait(100);
  }
  mouseClick(stageAll, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), inputDelay);

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicts resolved");
  view->commit();
  refresh(view, false);

  // Diff is not in a conflicted state
  git::Diff diff = mRepo->diffIndexToWorkdir();
  QVERIFY(!diff.isConflicted());
}

void TestMerge::cleanupTestCase()
{
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestMerge)

#include "merge.moc"

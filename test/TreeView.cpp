#include "Test.h"
#include "ui/MainWindow.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/TreeView.h"

#include <QTextEdit>

using namespace Test;
using namespace QTest;

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  auto repo = git::Repository::open(path);                                     \
  QVERIFY(repo.isValid());                                                     \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                               \
  RepoView *repoView = window.currentView();

class TestTreeView : public QObject {
  Q_OBJECT

private slots:
  void restoreStagedFileAfterCommit();

private:
};

void TestTreeView::restoreStagedFileAfterCommit() {
  INIT_REPO("TreeViewCollapseCount.zip", true);

  // Check for a single file called "test".
  RepoView *view = window.currentView();
  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  {
    auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(unstagedTree);
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 1)
      qWait(300);

    QCOMPARE(unstagedModel->rowCount(), 2);
    auto folder = unstagedModel->index(0, 0);
    auto subfolder = unstagedModel->index(0, 0, folder);
    auto file_txt = unstagedModel->index(0, 0, subfolder);
    QCOMPARE(unstagedModel->data(file_txt).toString(), QString("file.txt"));
    unstagedTree->selectionModel()->select(file_txt,
                                           QItemSelectionModel::Select);

    // Click on the check box. --> Stage file.txt
    mouseClick(unstagedTree->viewport(), Qt::LeftButton,
               Qt::KeyboardModifiers(),
               unstagedTree->checkRect(file_txt).center());
  }

  refresh(view, true);

  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  stagedTree->expandAll();

  QAbstractItemModel *stagedModel = stagedTree->model();
  QVERIFY(stagedTree);
  {
    QCOMPARE(stagedModel->rowCount(), 1);
    auto folder = stagedModel->index(0, 0);
    auto subfolder = stagedModel->index(0, 0, folder);
    auto file_txt = stagedModel->index(0, 0, subfolder);
    QCOMPARE(stagedModel->data(file_txt).toString(), QString("file.txt"));

    // Select file
    stagedTree->selectionModel()->clearSelection();
    stagedTree->selectionModel()->select(file_txt, QItemSelectionModel::Select);
  }

  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);
  editor->setText("conflicting commit b");
  view->commit();

  // The application should not crash!
}

TEST_MAIN(TestTreeView)

#include "TreeView.moc"

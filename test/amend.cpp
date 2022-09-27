#include "Test.h"

#include "git/Signature.h"
#include "git/Reference.h"
#include "git/Tree.h"
#include "ui/MainWindow.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"

#include <QTextEdit>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());

class TestAmend : public QObject {
  Q_OBJECT

private slots:
  void testAmend();
  void testAmendAddFile();
};

using namespace git;

void TestAmend::testAmend() {
  INIT_REPO("CherryPickAuthorEmail.zip", true);

  git::Reference master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  auto c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "changes");
  QCOMPARE(c.author().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(c.author().name(), "Martin Marmsoler");
  QCOMPARE(c.author().date(), QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "Martin Marmsoler");
  QCOMPARE(c.committer().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(c.committer().date(), QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));

  const QString commitMessage = "New commit message";

  auto authorSignature = repo.signature("New Author", "New Author Email", QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature =
      repo.signature("New Committer", "New Committer Email", QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  Tree tree;
  c.amend(authorSignature, committerSignature, commitMessage, tree);

  master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "New commit message");
  QCOMPARE(c.author().email(), "New Author Email");
  QCOMPARE(c.author().name(), "New Author");
  QCOMPARE(c.author().date(), QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "New Committer");
  QCOMPARE(c.committer().email(), "New Committer Email");
  QCOMPARE(c.committer().date(), QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));
}

void TestAmend::testAmendAddFile() {

  // Create repo
  Test::ScratchRepository mRepo;
  auto mMainBranch = mRepo->unbornHeadName();
  auto mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(QTest::qWaitForWindowExposed(mWindow));
  RepoView *view = mWindow->currentView();

  {
    // Add file and refresh.
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "This will be a test." << endl;

    Test::refresh(view);

    auto doubleTree = view->findChild<DoubleTreeWidget *>();
    QVERIFY(doubleTree);

    auto files = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(files);

    QAbstractItemModel *model = files->model();
    QCOMPARE(model->rowCount(), 1);

    // Click on the check box.
    QModelIndex index = model->index(0, 0);
    QTest::mouseClick(files->viewport(), Qt::LeftButton,
                      Qt::KeyboardModifiers(),
                      files->checkRect(index).center());

    // Commit and refresh.
    QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
    QVERIFY(editor);

    editor->setText("base commit");
    view->commit();
    Test::refresh(view, false);

    // Create branch and stage changes
    git::Branch branch2 =
        mRepo->createBranch("branch2", mRepo->head().target());
    QVERIFY(branch2.isValid());

    view->checkout(branch2);
    QCOMPARE(mRepo->head().name(), QString("branch2"));

    // Check if file has correct content
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    QCOMPARE(c.blob("test").content(), "This will be a test.\n");
  }

  // Stage file with changes
  {
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changes made" << endl;

    Test::refresh(view);

    auto doubleTree = view->findChild<DoubleTreeWidget *>();
    QVERIFY(doubleTree);

    // Staging the file
    auto files = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(files);

    QAbstractItemModel *model = files->model();
    QCOMPARE(model->rowCount(), 1);

    // Click on the check box. to stage file
    QModelIndex index = model->index(0, 0);
    QTest::mouseClick(files->viewport(), Qt::LeftButton,
                      Qt::KeyboardModifiers(),
                      files->checkRect(index).center());
  }

  // Check that changes applied after amending
  {
    // Amend changes
    git::Reference branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    auto authorSignature = mRepo->signature("New Author", "New Author Email");
    auto committerSignature =
        mRepo->signature("New Committer", "New Committer Email");
    QCOMPARE(mRepo->amend(c, authorSignature, committerSignature,
                          "New commit message"),
             true);

    {
      branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
      QVERIFY(branch2.isValid());
      c = branch2.annotatedCommit().commit();
      QCOMPARE(c.message(), "New commit message");
      QCOMPARE(c.author().email(), "New Author Email");
      QCOMPARE(c.author().name(), "New Author");
      QCOMPARE(c.committer().name(), "New Committer");
      QCOMPARE(c.committer().email(), "New Committer Email");

      QCOMPARE(c.blob("test").content(), "Changes made\n");
    }
  }
}

TEST_MAIN(TestAmend)
#include "amend.moc"

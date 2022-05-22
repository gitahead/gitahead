#include "Test.h"

#include "ui/MainWindow.h"
#include "ui/RepoView.h"

#include "conf/Settings.h"

#include "git/Reference.h"
#include "git/Signature.h"

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());

using namespace QTest;

class TestCherryPick : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void testAuthorEmailPreservance();
  void cleanupTestCase();

private:
  bool promptCherryPick;
};

void TestCherryPick::initTestCase() {
  promptCherryPick = Settings::instance()->prompt(Settings::PromptCherryPick);
}

void TestCherryPick::cleanupTestCase() {
  Settings::instance()->setPrompt(Settings::PromptCherryPick, promptCherryPick);
}

/*!
 * \brief TestCherryPick::testAuthorEmailPreservance
 * Check that author and email address are preserved during cherry pick
 */
void TestCherryPick::testAuthorEmailPreservance() {
  INIT_REPO("CherryPickAuthorEmail.zip", true);

  git::Commit commit =
      repo.lookupCommit("710846db7a1fbd583975da0a6c10f9c2964ebd08");
  QVERIFY(commit.isValid());

  auto *window = new MainWindow(repo);
  RepoView *view = window->currentView();

  // Commit directly
  Settings::instance()->setPrompt(Settings::PromptCherryPick, false);
  view->cherryPick(commit);

  git::Reference master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  auto c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "commit");
  QCOMPARE(c.author().email(), "test.author@test.com");
  QCOMPARE(c.author().name(), "TestAuthor");

  git::Signature signature = repo.defaultSignature();

  QCOMPARE(c.committer().email(), signature.email());
  QCOMPARE(c.committer().name(), signature.name());
}

TEST_MAIN(TestCherryPick)
#include "CherryPick.moc"

#include "Test.h"

#include "git/Signature.h"
#include "git/Reference.h"

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());

class TestAmend : public QObject {
  Q_OBJECT

private slots:
  void testAmend();
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
  QCOMPARE(c.committer().name(), "Martin Marmsoler");
  QCOMPARE(c.committer().email(), "martin.marmsoler@gmail.com");

  const QString commitMessage = "New commit message";

  auto authorSignature =
      repo.signature(c.author(), "New Author", "New Author Email");
  auto committerSignature =
      repo.signature(c.committer(), "New Committer", "New Committer Email");

  c.amend(authorSignature, committerSignature, commitMessage);

  master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "New commit message");
  QCOMPARE(c.author().email(), "New Author Email");
  QCOMPARE(c.author().name(), "New Author");
  QCOMPARE(c.committer().name(), "New Committer");
  QCOMPARE(c.committer().email(), "New Committer Email");
}

TEST_MAIN(TestAmend)
#include "amend.moc"

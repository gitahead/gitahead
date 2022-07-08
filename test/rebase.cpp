//
//          Copyright (c) 2022, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"

#include "ui/MainWindow.h"
#include "ui/DiffView/DiffView.h"
#include "ui/RepoView.h"

#include "git/Reference.h"
#include "git/Diff.h"
#include "git/Commit.h"
#include "git/Tree.h"
#include "git/Patch.h"

#include "log/LogEntry.h"

// TODO: move this macro into Test.h is also used in TestEditorLines
#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  mRepo = git::Repository::open(path);                                         \
  QVERIFY(mRepo.isValid());                                                    \
  MainWindow window(mRepo);                                                    \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                               \
  git::Reference head = mRepo.head();                                          \
  git::Commit commit = head.target();                                          \
  git::Diff stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */   \
                                                                               \
  RepoView *repoView = window.currentView();                                   \
  DiffView diffView = DiffView(mRepo, repoView);                               \
  auto diff = mRepo.status(mRepo.index(), nullptr, false);

using namespace git;

class TestRebase : public QObject {
  Q_OBJECT

private slots:
    void withoutConflicts();
    void conflictingRebase();
    void continueExternalStartedRebase(); // must have conflicts otherwise it is not possible to continue
    void startRebaseContinueInCLI();
    void startRebaseContinueInCLIContinueGUI(); // start rebase, continue in the cli and finish in the GUI
    void abortMR();
private:
    git::Repository mRepo;
};

//###################################################################################################
//###################################################################################################
//###################################################################################################
// Tests starting ######################################################################################
//###################################################################################################
//###################################################################################################
//###################################################################################################

void TestRebase::withoutConflicts() {
	INIT_REPO("rebaseConflicts.zip", false);

//    int rebaseFinished = 0;
//    int rebaseAboutToRebase = 0;
//    int rebaseCommitSuccess = 0;
//    int rebaseConflict = 0;

//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError, [=](){QVERIFY(false);});  // Should not be called
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase, [=, &rebaseAboutToRebase](const Rebase rebase, const Commit before, int count, LogEntry* parent){
//        QVERIFY(rebase.isValid());
//        QCOMPARE(count, 1);
//        QCOMPARE(before.message(), "File2.txt added\n");
//        rebaseAboutToRebase++;
//    });
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid, [=](){QVERIFY(false);}); // Should not be called
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished, [=, &rebaseFinished](){rebaseFinished++;});
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess, [=, &rebaseCommitSuccess](const Rebase rebase, const Commit before, const Commit after, int counter, LogEntry* parent) {
//        QVERIFY(rebase.isValid());
//        rebaseCommitSuccess++;
//    });
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict, [=, &rebaseConflict](){
//        rebaseConflict++;
//    });

//    const QString rebaseBranchName = "refs/heads/noConflict";

//    git::Reference branch = mRepo.lookupRef(rebaseBranchName);
//    QVERIFY(branch.isValid());
//    auto c = branch.annotatedCommit().commit();

//    // Checkout correct branch
//    QCOMPARE(mRepo.checkout(c), true);

//    // Rebase on main
//    git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
//    QVERIFY(mainBranch.isValid());
//    auto ac = mainBranch.annotatedCommit();
//    mRepo.rebase(ac);

//    // Check that branch is based on "main" now
//    branch = mRepo.lookupRef(rebaseBranchName);
//    QVERIFY(branch.isValid());
//    QList<Commit> parents = branch.annotatedCommit().commit().parents();
//    QCOMPARE(parents.count(), 1);
//    QCOMPARE(parents.at(0).id(), ac.commit().id());

//    // Check that rebase was really finished
//    QCOMPARE(mRepo.rebaseOngoing(), false);

//    // Check call counters
//    QCOMPARE(rebaseFinished, 1);
//    QCOMPARE(rebaseAboutToRebase, 1);
//    QCOMPARE(rebaseCommitSuccess, 1);
//    QCOMPARE(rebaseConflict, 0);
}

void TestRebase::conflictingRebase() {
//    INIT_REPO("rebaseConflicts.zip", false);

//    int rebaseFinished = 0;
//    int rebaseAboutToRebase = 0;
//    int rebaseCommitSuccess = 0;
//    int rebaseConflict = 0;
//    int refreshTriggered = 0;

//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError, [=](){QVERIFY(false);});  // Should not be called
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase, [=, &rebaseAboutToRebase](const Rebase rebase, const Commit before, int count, LogEntry* parent){
//        QVERIFY(rebase.isValid());
//        QCOMPARE(count, 1);
//        QCOMPARE(before.message(), "File.txt changed by second branch\n");
//        rebaseAboutToRebase++;
//    });
//    // TODO: = needed?
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid, [=](){QVERIFY(false);}); // Should not be called
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished, [=, &rebaseFinished](){rebaseFinished++;});
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess, [=, &rebaseCommitSuccess](const Rebase rebase, const Commit before, const Commit after, int counter, LogEntry* parent) {
//        QVERIFY(rebase.isValid());
//        rebaseCommitSuccess++;
//    });
//    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict, [=, &rebaseConflict, &rebaseCommitSuccess](){
//        QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
//        rebaseConflict++;
//    });

//    connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated, [this, &refreshTriggered](const Reference &ref) {
//        QCOMPARE(ref, mRepo.head());
//        refreshTriggered++;
//    });

//    const QString rebaseBranchName = "refs/heads/singleCommitConflict";

//    git::Reference branch = mRepo.lookupRef(rebaseBranchName);
//    QVERIFY(branch.isValid());
//    auto c = branch.annotatedCommit().commit();

//    // Checkout correct branch
//    QCOMPARE(mRepo.checkout(c), true);

//    // Rebase on main
//    git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
//    QVERIFY(mainBranch.isValid());
//    auto ac = mainBranch.annotatedCommit();
//    refreshTriggered = 0;
//    mRepo.rebase(ac);
//    QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

//    QCOMPARE(mRepo.rebaseOngoing(), true);
//    QCOMPARE(rebaseFinished, 0);
//    QCOMPARE(rebaseConflict, 1);
//    // TODO: check toolbar buttons

//    // Resolve conflicts
//    diff = mRepo.status(mRepo.index(), nullptr, false);
//    QCOMPARE(diff.count(), 1);
//    QCOMPARE(diff.patch(0).isConflicted(), true);
//    QFile f(diff.patch(0).name());
//    QCOMPARE(f.open(QIODevice::WriteOnly), true);
//    f.write("Test123"); // just write something to resolve the conflict

//    refreshTriggered = 0;
//    rebaseConflict = 0;
//    mRepo.rebaseContinue("Test message", nullptr);
//    QCOMPARE(refreshTriggered, 1);
//    // TODO: check toolbar buttons

//    // Check that branch is based on "main" now
//    branch = mRepo.lookupRef(rebaseBranchName);
//    QVERIFY(branch.isValid());
//    QList<Commit> parents = branch.annotatedCommit().commit().parents();
//    QCOMPARE(parents.count(), 1);
//    QCOMPARE(parents.at(0).id(), ac.commit().id());
//    QCOMPARE(branch.annotatedCommit().commit().message(), "Test message"); // custom message was used instead of the original one

//    // Check that rebase was really finished
//    QCOMPARE(mRepo.rebaseOngoing(), false);

//    // Check call counters
//    QCOMPARE(rebaseFinished, 1);
//    QCOMPARE(rebaseAboutToRebase, 1);
//    QCOMPARE(rebaseCommitSuccess, 1);
//    QCOMPARE(rebaseConflict, 0);
}

void TestRebase::continueExternalStartedRebase() {
    // TODO: implement
}

void TestRebase::startRebaseContinueInCLI() {
    // TODO: implement
}

void TestRebase::startRebaseContinueInCLIContinueGUI() {
    // TODO: implement
}

void TestRebase::abortMR() {
    // TODO: implement
}

TEST_MAIN(TestRebase)

#include "rebase.moc"

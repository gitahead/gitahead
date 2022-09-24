//
//          Copyright (c) 2022, Gittyup Team
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"

#include "dialogs/CloneDialog.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/RepoView.h"

#include "git/Submodule.h"

#include <QToolButton>
#include <QMenu>
#include <QWizard>
#include <QLineEdit>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  auto repo = git::Repository::open(path);                                         \
  QVERIFY(repo.isValid());                                                    \
  MainWindow window(repo);                                                    \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                   \
  RepoView *repoView = window.currentView();                                   \
  auto diff = repo.status(repo.index(), nullptr, false);


using namespace Test;
using namespace QTest;

class TestSubmodule : public QObject {
  Q_OBJECT

private slots:
    void updateSubmoduleClone();
    void discardFile();


private:

};

void TestSubmodule::updateSubmoduleClone() {
    QString remote = Test::extractRepository("SubmoduleTest.zip", true);
    // Update submodules after cloning

//    git::Repository repo; // invalid repo
//    MainWindow window(repo);
//    window.show();

//    auto mb = window.menuBar();

    CloneDialog* d = new CloneDialog(CloneDialog::Kind::Clone);

    RepoView* view = nullptr;

    bool cloneFinished = false;
    QObject::connect(d, &CloneDialog::accepted, [d, &view, &cloneFinished] {
       cloneFinished = true;
      if (MainWindow *window = MainWindow::open(d->path())) {
        view = window->currentView();
      }
    });

    d->setField("url", remote);
    d->setField("name", "TestrepoSubmodule");
    d->setField("path", QString(TESTREPOSITORIES_PATH));
    d->setField("bare", "false");
    d->page(2)->initializePage(); // start clone

    {
      auto timeout =
          Timeout(2000000, "Failed to clone");
      while (!cloneFinished)
        qWait(300);
    }

    QVERIFY(view);
    QCOMPARE(view->repo().submodules().count(), 1);
    for (auto& s: view->repo().submodules())
        QVERIFY(s.isValid());


//    MainWindow *mWindow = nullptr;

//    QDir dir = QDir::temp();
//    if (dir.cd("test_init_repo"))
//      QVERIFY(dir.removeRecursively());

//    StartDialog *dialog = StartDialog::openSharedInstance();
//    QVERIFY(qWaitForWindowActive(dialog));

//    // Find the first button in the first footer.
//    Footer *footer = dialog->findChild<Footer *>();
//    QToolButton *plus = footer->findChild<QToolButton *>();

//    // Set up timer to dismiss the popup.
//    QTimer::singleShot(500, [] {
//      QMenu *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
//      QVERIFY(menu);

//      keyClick(menu, Qt::Key_Down);
//      keyClick(menu, Qt::Key_Down);
//      keyClick(menu, Qt::Key_Down);
//      keyClick(menu, Qt::Key_Return);
//    });

//    {
//      auto timeout = Timeout(1000, "Start dialog didn't close in time");

//      // Show popup menu.
//      mouseClick(plus, Qt::LeftButton);
//    }

//    CloneDialog *cloneDialog =
//        qobject_cast<CloneDialog *>(QApplication::activeModalWidget());
//    QVERIFY(cloneDialog);

//    // Set fields.
//    cloneDialog->setField("name", "test_init_repo");
//    cloneDialog->setField("path", QDir::tempPath());

//    // Click return.
//    keyClick(cloneDialog, Qt::Key_Return);

//    // Wait on the new window.
//    mWindow = MainWindow::activeWindow();
//    QVERIFY(mWindow && qWaitForWindowExposed(mWindow));
}

void TestSubmodule::discardFile() {
    // Discarding a file should not reset the submodule
    INIT_REPO("SubmoduleTest.zip", false);

//    {
//        QFile file(repo.workdir().filePath("Readme.md"));
//        QVERIFY(file.open(QFile::WriteOnly));
//        QTextStream(&file) << "This will be a test." << endl;
//        file.close();
//    }

//    {
//        QFile file(repo.workdir().filePath("Gittyup-Test-Module/Readme.d"));
//        QVERIFY(file.open(QFile::WriteOnly));
//        QTextStream(&file) << "Adding Readme to submodule" << endl;
//        file.close();
//    }
}

TEST_MAIN(TestSubmodule)

#include "Submodule.moc"

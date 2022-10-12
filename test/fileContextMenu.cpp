#include "Test.h"

#include "ui/FileContextMenu.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "git/Reference.h"

#include <QMessageBox>
#include <QPushButton>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
  RepoView *repoView = window.currentView();

class TestFileContextMenu : public QObject {
  Q_OBJECT

private slots:
  void testDiscardFile();
  void testDiscardFolder();
};

using namespace git;

void TestFileContextMenu::testDiscardFile() {
  INIT_REPO("TestRepository.zip", false);

  git::Commit commit =
      repo.lookupCommit("51198ba9b2b2b2c25ea6576cf7ca3e9f2a7c3fc7");
  QVERIFY(commit);

  // modifying all files
  QHash<QString, QString> fileContent{
      {"file.txt", "Modified file"},
      {"file2.txt", "Modified file2"},
      {"folder1/file.txt", "Modified file in folder1"},
      {"folder1/file2.txt", "Modified file2 in folder1"},
      {"GittyupTestRepo/README.md", "Modified readme in submodule"},
  };

  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::WriteOnly));
      file.write(i.value().toLatin1());
    }
  }

  // refresh repo
  repo.notifier()->referenceUpdated(repo.head());

  QStringList files = {"file.txt"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto* a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  action->triggered(true);

  auto *msgBox = repoView->findChild<QMessageBox *>();
  QVERIFY(msgBox);
  auto *button = msgBox->findChild<QPushButton *>("DiscardButton");
  QVERIFY(button);
  emit button->clicked(true);

  // original text
  //  {"file.txt", "File.txt"},
  //  {"file2.txt", "file2.txt"},
  //  {"folder1/file.txt", "file in folder1"},
  //  {"folder1/file2.txt", "file2 in folder1"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    fileContentRef.insert("file.txt", "File.txt"); // this file was discarded
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardFolder() {}

TEST_MAIN(TestFileContextMenu)
#include "fileContextMenu.moc"

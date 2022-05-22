//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TEST_H
#define TEST_H

#include "app/Application.h"
#include "git/Repository.h"
#include <iostream>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#define TEST_MAIN(TestClass)                                                   \
  int main(int argc, char *argv[]) {                                           \
    QTEST_SET_MAIN_SOURCE_PATH                                                 \
    return ::Test::runTest<TestClass>(argc, argv);                             \
  }

class RepoView;

namespace Test {

class ScratchRepository {
public:
  ScratchRepository(bool autoRemove = true);

  // Treat this class a wrapper.
  operator git::Repository();
  git::Repository *operator->();

private:
  QTemporaryDir mDir;
  git::Repository mRepo;
};

class Timeout : public QObject {
  // Q_OBJECT

public:
  Timeout(int milliseconds, QString message);
  ~Timeout();
  void dismiss();

private:
  QString message;
  QTimer timer;

  void onTimeout();
};

void refresh(RepoView *repoView, bool expectDirty = true);
void fetch(RepoView *repoView, git::Remote remote);
QString extractRepository(const QString &filename, bool useTempDir);

Application createApp(int &argc, char *argv[]);

template <typename T> int runTest(int argc, char *argv[]) {
  Application::setInTest();

  int orig_argc = argc;
  auto app = createApp(argc, argv);

  T testObj;

  return QTest::qExec(&testObj, orig_argc, argv);
}

} // namespace Test

#endif

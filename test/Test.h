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
#include <QTemporaryDir>
#include <QtTest/QtTest>

#define TEST_MAIN(TestClass) \
int main(int argc, char *argv[]) \
{ \
  Application app(argc, argv, false, "Default"); \
  TestClass test; \
  QTEST_SET_MAIN_SOURCE_PATH \
  return QTest::qExec(&test, app.arguments()); \
}

class RepoView;

namespace Test {

class ScratchRepository
{
public:
  ScratchRepository(bool autoRemove = true);

  // Treat this class a wrapper.
  operator git::Repository();
  git::Repository *operator->();

private:
  QTemporaryDir mDir;
  git::Repository mRepo;
};

void refresh(RepoView *repoView, bool expectDirty = true);
void fetch(RepoView *repoView, git::Remote remote);

} // namespace Test

#endif

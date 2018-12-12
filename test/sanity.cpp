//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"

using namespace QTest;

class TestSanity : public QObject
{
  Q_OBJECT

private slots:
  void sanity();
};

void TestSanity::sanity()
{
  QCOMPARE(QCoreApplication::applicationName(), QString(GITAHEAD_NAME));
  QCOMPARE(QCoreApplication::applicationVersion(), QString(GITAHEAD_VERSION));
}

TEST_MAIN(TestSanity)

#include "sanity.moc"

#include "Test.h"
#include "dialogs/ExternalToolsDialog.h"

#include "ui/DiffView/HunkWidget.h"
#include "ui/DiffView/Line.h"
#include <QString>

using namespace QTest;

class TestHunkWidget : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

private:
  int closeDelay = 0;
  HunkWidget *mHunk{nullptr};
};

void TestHunkWidget::initTestCase() {
  bool lfs = false;
  bool submodule = false;
}

void TestHunkWidget::cleanupTestCase() { qWait(closeDelay); }

TEST_MAIN(TestHunkWidget)
#include "HunkWidget.moc"

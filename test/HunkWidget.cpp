#include "Test.h"
#include "dialogs/ExternalToolsDialog.h"

#include "ui/DiffView/HunkWidget.h"
#include "ui/DiffView/Line.h"
#include <QString>

using namespace QTest;

class TestHunkWidget : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void findStagedLines();
  void determineLineOffset();
  void cleanupTestCase();

private:
  int closeDelay = 0;
  HunkWidget* mHunk{nullptr};
};

void TestHunkWidget::initTestCase()
{
  bool lfs = false;
  bool submodule = false;
}

void TestHunkWidget::cleanupTestCase()
{
  qWait(closeDelay);
}

void TestHunkWidget::findStagedLines()
{

    Line line1(' ', 1, 1);
    Line line2('-', 2, -1);
    Line line3('+', -1, 2);
    Line line4(' ', 3, 3);
    QList<Line> lines({line1, line2, line3, line4});
    int offset = 0;

    //-------------------------------------------------------------------------
    // Line was modified
    // Stage complete hunk
    {
    Line sline1(' ', 1, 1);
    Line sline2('-', 2, -1);
    Line sline3('+', -1, 2);
    Line sline4(' ', 3, 3);
    QList<Line> linesStaged({sline1, sline2, sline3, sline4});


    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines, additions, lidx, offset, linesStaged, stagedAdditions, staged);

        if (lidx == 0 || lidx == 3)
            QVERIFY(staged == false);

        if (lidx == 1 || lidx == 2)
            QVERIFY(staged == true);
    }
    }

    //-------------------------------------------------------------------------
    // Line was modified
    // Stage only the deletion
    {
    Line sline21(' ', 1, 1);
    Line sline22('-', 2, -1);
    Line sline23(' ', 3, 2);
    QList<Line> linesStaged2({sline21, sline22, sline23});

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines, additions, lidx, offset, linesStaged2, stagedAdditions, staged);

        if (lidx == 0 || lidx == 2 || lidx == 3)
            QVERIFY(staged == false);

        if (lidx == 1)
            QVERIFY(staged == true);
    }
    }

    //-------------------------------------------------------------------------
    // Line was modified
    // Stage only the addition
    {
    Line sline31(' ', 1, 1);
    Line sline32(' ', 2, 2);
    Line sline33('+', -1, 3);
    Line sline34(' ', 3, 4);
    QList<Line> linesStaged3({sline31, sline32, sline33, sline34});

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines, additions, lidx, offset, linesStaged3, stagedAdditions, staged);

        if (lidx == 0 || lidx == 1 || lidx == 3)
            QVERIFY(staged == false);

        if (lidx == 2)
            QVERIFY(staged == true);
    }
    }

    //-------------------------------------------------------------------------
    // Line was modified
    // Stage only the addition, offset = 1
    {
    offset = 1;
    Line sline42(' ', 2, 2);
    Line sline43('+', -1, 3);
    Line sline44(' ', 3, 4);
    QList<Line> linesStaged4({sline42, sline43, sline44});

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines, additions, lidx, offset, linesStaged4, stagedAdditions, staged);

        if (lidx == 0 || lidx == 1 || lidx == 3)
            QVERIFY(staged == false);

        if (lidx == 2)
            QVERIFY(staged == true);
    }
    }

    //-------------------------------------------------------------------------
    // Line was modified
    // Nothing is staged
    {
    QList<Line> linesStaged5;

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines, additions, lidx, offset, linesStaged5, stagedAdditions, staged);
        QVERIFY2(staged == false, QString::number(lidx).toUtf8());
    }
    }

    Line line21(' ', 1, 1);
    Line line22('-', 2, -1);
    Line line23('+', -1, 2);
    Line line24(' ', 3, 3);
    Line line25('-', 2, -1);
    Line line26('+', -1, 2);
    Line line27(' ', 3, 3);
    QList<Line> lines2({line21, line22, line23, line24, line25, line26, line27});

    //-------------------------------------------------------------------------
    // 2 Lines were modified
    // Only deletions are staged
    {
    Line line61(' ', 1, 1);
    Line line62('-', 2, -1);
    Line line64(' ', 3, 2);
    Line line65('-', 4, -1);
    Line line67(' ', 5, 3);
    QList<Line> linesStaged6({line61, line62, line64, line65, line67});

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines2, additions, lidx, offset, linesStaged6, stagedAdditions, staged);

        if (lidx == 0 || lidx == 3 || lidx == 4 || lidx == 6 || lidx == 7)
            QVERIFY2(staged == false, QString::number(lidx).toUtf8());
    }
    }

    //-------------------------------------------------------------------------
    // 2 Lines were modified
    // Only additions are staged
    {
    Line line71(' ', 1, 1);
    Line line72(' ', 2, 2);
    Line line73('+', -1, 3);
    Line line74(' ', 3, 4);
    Line line75(' ', 4, 5);
    Line line76('+', -1, 6);
    Line line77(' ', 6, 7);
    QList<Line> linesStaged7({line71, line72, line73, line74, line75, line76, line77});

    int stagedAdditions = 0;
    int additions = 0;
    for (int lidx = 0; lidx < lines.count(); lidx++) {
        bool staged;
        HunkWidget::findStagedLines(lines2, additions, lidx, offset, linesStaged7, stagedAdditions, staged);

        if (lidx == 0 || lidx == 3 || lidx == 4 || lidx == 6 || lidx == 7)
            QVERIFY2(staged == false, QString::number(lidx).toUtf8());
    }
    }
}

void TestHunkWidget::determineLineOffset()
{
    Line line1(' ', 1, 1);
    Line line2(' ', 2, 2);
    Line line3(' ', 3, 3);
    Line line4(' ', 4, 4);
    QList<Line> lines({line1, line2, line3, line4});

    Line sline1(' ', 1, 1);
    Line sline2(' ', 2, 2);
    Line sline3(' ', 3, 3);
    Line sline4(' ', 4, 4);
    QList<Line> stagedLines({sline1, sline2, sline3, sline4});

    int offset = HunkWidget::determineLineOffset(lines, stagedLines);
    QVERIFY(offset == 0);

    //-------------------------------------------------------------------------
    QList<Line> lines2({line1, line2, line3, line4});
    QList<Line> stagedLines2({sline3, sline4});
    int offset2 = HunkWidget::determineLineOffset(lines2, stagedLines2);
    QVERIFY(offset2 == +2);

    //-------------------------------------------------------------------------
    QList<Line> lines3({line4});
    QList<Line> stagedLines3({sline1, sline2, sline3, sline4});
    int offset3 = HunkWidget::determineLineOffset(lines3, stagedLines3);
    QVERIFY(offset2 == -3);

    //-------------------------------------------------------------------------
    // Should not occur!
    QList<Line> lines4;
    QList<Line> stagedLines4({sline1, sline2, sline3, sline4});
    int offset4 = HunkWidget::determineLineOffset(lines4, stagedLines4);
    QVERIFY(offset2 == 4);

}

TEST_MAIN(TestHunkWidget)
#include "HunkWidget.moc"

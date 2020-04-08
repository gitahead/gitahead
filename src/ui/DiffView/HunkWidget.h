#ifndef HUNKWIDGET_H
#define HUNKWIDGET_H

#include <QFrame>
#include <QCheckBox>
#include <QWidget>
#include <QLabel>

#include "../../editor/TextEditor.h"
#include "../../git/Patch.h"

#include "git/Diff.h"

class DiffView;
class Header;
class QToolButton;
class DisclosureButton;

namespace _HunkWidget {
    class Header : public QWidget
    {
    public:
      Header(
        const git::Diff &diff,
        const git::Patch &patch,
        int index,
        bool lfs,
        bool submodule,
        QWidget *parent = nullptr);
      QCheckBox *check() const;

      DisclosureButton* button() const;

      QToolButton *saveButton() const;
      QToolButton *undoButton() const;
      QToolButton *oursButton() const;
      QToolButton *theirsButton() const;

    protected:
      void mouseDoubleClickEvent(QMouseEvent *event) override;

    private:
      QCheckBox *mCheck;
      DisclosureButton *mButton;
      QToolButton *mSave = nullptr;
      QToolButton *mUndo = nullptr;
      QToolButton *mOurs = nullptr;
      QToolButton *mTheirs = nullptr;
    };
}

class HunkWidget : public QFrame
{
  Q_OBJECT

public:
  HunkWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    int index,
    bool lfs,
    bool submodule,
    QWidget *parent = nullptr);
  _HunkWidget::Header *header() const;
  TextEditor *editor(bool ensureLoaded = true);
  void invalidate();

signals:
  void updated(QString name, QByteArray& buffer);

protected:
  void paintEvent(QPaintEvent *event);

private slots:
  void stageSelected(int startLine, int end);
  void unstageSelected(int startLine, int end);
   void setStaged(int lidx, bool staged);
   void marginClicked(int pos, int modifier, int margin);

private:
  struct Token
  {
    int pos;
    QByteArray text;
  };

  int tokenEndPosition(int pos) const;

  QList<Token> tokens(int line) const;
  QByteArray tokenBuffer(const QList<Token> &tokens);
  void load();
  void chooseLines(TextEditor::Marker kind);

  DiffView *mView;
  git::Patch mPatch;
  git::Patch mStaged;
  int mIndex;

  _HunkWidget::Header *mHeader;
  TextEditor *mEditor;
  bool mLoaded = false;
};

#endif // HUNKWIDGET_H

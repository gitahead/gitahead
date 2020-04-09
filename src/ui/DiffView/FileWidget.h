#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include "../../editor/TextEditor.h"

#include <QWidget>
#include <QFrame>

#include "git/Diff.h"
#include "git/Patch.h"

class QCheckBox;
class QToolButton;
class QVBoxLayout;

class DisclosureButton;
class DiscardButton;
class EditButton;
class LineStats;
class Badge;
class FileLabel;
class DiffView;
class HunkWidget;

namespace _FileWidget {
class Header : public QFrame
{
    Q_OBJECT

public:
  Header(
    const git::Diff &diff,
    const git::Patch &patch,
    bool binary,
    bool lfs,
    bool submodule,
    QWidget *parent = nullptr);
  void updatePatch(const git::Patch &patch);
  QCheckBox *check() const;

  DisclosureButton *disclosureButton() const;

  QToolButton *lfsButton() const;
  void setStageState(git::Index::StagedState state);

signals:
  void stageStageChanged(int stageState);
protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  void updateCheckState();

  git::Diff mDiff;
  git::Patch mPatch;
  bool mSubmodule;

  QCheckBox *mCheck{nullptr};
  QToolButton *mLfsButton = nullptr;
  EditButton *mEdit{nullptr};
  DiscardButton *mDiscardButton{nullptr};
  DisclosureButton *mDisclosureButton{nullptr};
  LineStats* mStats{nullptr};
  Badge *mStatusBadge{nullptr};
  FileLabel *mFileLabel{nullptr};
};

}

class FileWidget : public QWidget
{
  Q_OBJECT

public:

  FileWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    QWidget *parent = nullptr);
  bool isEmpty();
  void updatePatch(const git::Patch &patch, const git::Patch &staged);
  _FileWidget::Header *header() const;
  QString name() const;

  QList<HunkWidget *> hunks() const;

  QWidget *addImage(
    DisclosureButton *button,
    const git::Patch patch,
    bool lfs = false);
  HunkWidget *addHunk(
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    int index,
    bool lfs,
    bool submodule);
public slots:
  void headerCheckStateChanged(int state);
  /*!
   * Calculates the stage state of the file.
   * emits signal "stageStateChanged" with the current
   * state as parameter
   * \brief stageStateHunks
   */
  void stageStateHunks();
  /*!
   * Discard specific hunk
   * Emitted by the hunk it self
   * \brief discardHunk
   */
  void discardHunk();
  /*!
   * Discard all changes in the file
   * Emitted by the _FileWidget::Header
   * \brief discard
   */
  void discard();

signals:
  void diagnosticAdded(TextEditor::DiagnosticKind kind);
  void stageStateChanged(git::Index::StagedState state);

private:
  DiffView *mView{nullptr};

  git::Diff mDiff;
  git::Patch mPatch;

  _FileWidget::Header *mHeader{nullptr};
  QList<QWidget *> mImages;
  QList<HunkWidget *> mHunks;
  QVBoxLayout* mHunkLayout{nullptr};
  bool mIgnoreStaging{false};
};

#endif // FILEWIDGET_H


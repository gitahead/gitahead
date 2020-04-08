#ifndef EDITBUTTON_H
#define EDITBUTTON_H

#include "Button.h"
#include "ui/RepoView.h"
#include "git/Patch.h"

#include <QMenu>

class EditButton : public Button
{
public:
  EditButton(
    const git::Patch &patch,
    int index,
    bool binary,
    bool lfs,
    QWidget *parent = nullptr);
  void updatePatch(const git::Patch &patch, int index);
protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QMenu* mEditMenu{nullptr};
  QAction* mEditWorkingCopyAction{nullptr};
  QAction* mEditNewRevisionAction{nullptr};
  QAction* mEditOldRevisionAction{nullptr};
  bool mBinary;
  bool mLfs;
};

#endif // EDITBUTTON_H


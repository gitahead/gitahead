//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DeleteBranchDialog.h"
#include "git/Branch.h"
#include "ui/RepoView.h"
#include <QCheckBox>
#include <QPushButton>

DeleteBranchDialog::DeleteBranchDialog(
  const git::Branch &branch,
  QWidget *parent)
  : QMessageBox(parent)
{
  bool isLocal = branch.isLocalBranch();
  QString text = tr("Are you sure you want to delete %1 branch '%2'?");
  setWindowTitle(tr("Delete Branch?"));
  setText(text.arg(isLocal ? tr("local") : tr("remote"), branch.name()));
  setStandardButtons(QMessageBox::Cancel);

  git::Branch upstream = isLocal ? branch.upstream() : git::Branch();
  if (upstream.isValid()) {
    QString text = tr("Also delete the upstream branch from its remote");
    setCheckBox(new QCheckBox(text, this));
  }

  QPushButton *remove = addButton(tr("Delete"), QMessageBox::AcceptRole);
  connect(remove, &QPushButton::clicked, [this, branch, isLocal, upstream] {
    RepoView *view = RepoView::parentView(this);
    if (isLocal)
      git::Branch(branch).remove();
    else
      view->deleteRemoteBranch(branch);

    if (upstream.isValid() && checkBox()->isChecked())
      view->deleteRemoteBranch(upstream);
  });

  remove->setFocus();

  if (isLocal && !branch.isMerged()) {
    setInformativeText(
      tr("The branch is not fully merged. Deleting "
         "it may cause some commits to be lost."));
    setDefaultButton(QMessageBox::Cancel);
  }
}

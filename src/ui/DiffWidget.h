//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFWIDGET_H
#define DIFFWIDGET_H

#include "DetailView.h"
#include "git/Index.h"


class DiffView;
class FileList;
class FindWidget;
class QSplitter;

namespace git {
class Diff;
class Repository;
}

class DiffWidget : public ContentWidget
{
public:
  DiffWidget(const git::Repository &repo, QWidget *parent = nullptr);

  QString selectedFile() const override;

  void setDiff(
    const git::Diff &diff,
    const QString &file = QString(),
    const QString &pathspec = QString()) override;

  void find() override;
  void findNext() override;
  void findPrevious() override;
  
private:
  void selectFile(const QString &name);
  void setCurrentFile(int value);

  DiffView *mDiffView;
  FileList *mFiles;
  FindWidget *mFind;
  QSplitter *mSplitter;

  git::Diff mDiff;

  QList<QMetaObject::Connection> mConnections;
};

#endif

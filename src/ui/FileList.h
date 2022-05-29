//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef FILELIST_H
#define FILELIST_H

#include "git/Diff.h"
#include <QListView>

class QToolButton;

namespace git {
class Repository;
}

class FileList : public QListView
{
  Q_OBJECT

public:
  FileList(const git::Repository &repo, QWidget *parent = nullptr);

  void setDiff(const git::Diff &diff, const QString &pathspec = QString());

  QSize sizeHint() const override;

  // This is public as a testing artifact.
  QRect checkRect(const QModelIndex &index);

  static void setFileRows(int rows);

signals:
  void sortRequested();

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *) override;

private:
  void updateMenu(const git::Diff &diff);

  QIcon mSpacerIcon;
  QIcon mAscIcon;
  QIcon mDesIcon;

  QMap<int, QByteArray> mSortMap;
  QList<QAction *> mActionList;

  QMenu *mSelectMenu;
  QAction *mIgnoreWs;
  QModelIndex mPressedIndex;
  QToolButton *mButton;

  git::Diff mDiff;
};

#endif

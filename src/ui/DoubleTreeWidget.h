//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef DOUBLETREEWIDGET_H
#define DOUBLETREEWIDGET_H

#include "DetailView.h" // ContentWidget

class QTreeView;
class BlameEditor;

// button in treeview: https://stackoverflow.com/questions/40716138/how-to-add-a-button-to-a-qtreeview-row

/*!
 * \brief The DoubleTreeWidget class
 * TreeView like in GitKraken
 */
class DoubleTreeWidget : public ContentWidget
{
  Q_OBJECT
  
public:
  DoubleTreeWidget(const git::Repository &repo, QWidget *parent = nullptr);
  QString selectedFile() const override;

  void setDiff(
    const git::Diff &diff,
    const QString &file = QString(),
    const QString &pathspec = QString()) override;


protected:

private:

  TreeView* stagedFiles{nullptr};
  TreeView* unstagedFiles{nullptr};
  BlameEditor *mEditor{nullptr};
};
#endif // DOUBLETREEWIDGET_H

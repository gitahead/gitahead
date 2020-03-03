//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef ALTERNATIVETREEWIDGET_H
#define ALTERNATIVETREEWIDGET_H

#include "DetailView.h" // ContentWidget
#include "TreeView.h"

/*!
 * \brief The AlternativeTreeWidget class
 * TreeView like in GitKraken
 */
class AlternativeTreeWidget : public ContentWidget
{
  Q_OBJECT
  
public:
  AlternativeTreeWidget(const git::Repository &repo, QWidget *parent = nullptr);
  QString selectedFile() const override;

  void setDiff(
    const git::Diff &diff,
    const QString &file = QString(),
    const QString &pathspec = QString()) override;


protected:

private:

  TreeView *mView;
};
#endif // ALTERNATIVETREEWIDGET_H

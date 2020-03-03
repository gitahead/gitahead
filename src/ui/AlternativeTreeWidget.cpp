//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "AlternativeTreeWidget.h"

AlternativeTreeWidget::AlternativeTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{

}

QString AlternativeTreeWidget::selectedFile() const {

}

void AlternativeTreeWidget::setDiff(const git::Diff &diff,
									const QString &file,
									const QString &pathspec) {

}

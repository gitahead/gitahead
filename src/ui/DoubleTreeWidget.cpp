//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "DoubleTreeWidget.h"
#include "BlameEditor.h"

#include <QTreeView>
#include <QVBoxLayout>

DoubleTreeWidget::DoubleTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
	stagedFiles = new QTreeView(this);
	unstagedFiles = new QTreeView(this);
	QSplitter *treeViewSplitter = new QSplitter(Qt::Vertical, this);
	treeViewSplitter->setHandleWidth(10);
	treeViewSplitter->addWidget(unstagedFiles);
	treeViewSplitter->addWidget(stagedFiles);
	treeViewSplitter->setStretchFactor(1, 1);

	mEditor = new BlameEditor(repo, this);


	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setHandleWidth(0);
	splitter->addWidget(treeViewSplitter);
	splitter->addWidget(mEditor);
	splitter->setStretchFactor(1, 1);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(splitter);
}

QString DoubleTreeWidget::selectedFile() const {

}

void DoubleTreeWidget::setDiff(const git::Diff &diff,
									const QString &file,
									const QString &pathspec) {

}

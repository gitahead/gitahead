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
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

DoubleTreeWidget::DoubleTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
	QVBoxLayout* vBoxLayout = new QVBoxLayout();
	QHBoxLayout* hBoxLayout = new QHBoxLayout();
	QLabel* label = new QLabel(tr("Staged Files"));
	QPushButton* button = new QPushButton(tr("Stage all changes"));
	hBoxLayout->addWidget(label);
	hBoxLayout->addWidget(button);
	stagedFiles = new QTreeView(this);
	vBoxLayout->addLayout(hBoxLayout);
	vBoxLayout->addWidget(stagedFiles);
	QWidget* stagedWidget = new QWidget();
	stagedWidget->setLayout(vBoxLayout);

	vBoxLayout = new QVBoxLayout();
	hBoxLayout = new QHBoxLayout();
	label = new QLabel(tr("Unstaged Files"));
	button = new QPushButton(tr("Unstage all changes"));
	hBoxLayout->addWidget(label);
	hBoxLayout->addWidget(button);
	unstagedFiles = new QTreeView(this);
	vBoxLayout->addLayout(hBoxLayout);
	vBoxLayout->addWidget(unstagedFiles);
	QWidget* unstagedWidget = new QWidget();
	unstagedWidget->setLayout(vBoxLayout);

	QSplitter *treeViewSplitter = new QSplitter(Qt::Vertical, this);
	treeViewSplitter->setHandleWidth(10);
	treeViewSplitter->addWidget(unstagedWidget);
	treeViewSplitter->addWidget(stagedWidget);
	treeViewSplitter->setStretchFactor(1, 1);

	mEditor = new BlameEditor(repo, this);


	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setHandleWidth(0);
	splitter->addWidget(mEditor);
	splitter->addWidget(treeViewSplitter);
	splitter->setStretchFactor(1, 1);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(splitter);

	setLayout(layout);
}

QString DoubleTreeWidget::selectedFile() const {

}

void DoubleTreeWidget::setDiff(const git::Diff &diff,
									const QString &file,
									const QString &pathspec) {

}

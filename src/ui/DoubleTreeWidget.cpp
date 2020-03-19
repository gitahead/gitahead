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
#include "TreeModel.h"
#include "TreeProxy.h"
#include "TreeView.h"
#include "ViewDelegate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

} // anon. namespace

DoubleTreeWidget::DoubleTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
	QVBoxLayout* vBoxLayout = new QVBoxLayout();
	QHBoxLayout* hBoxLayout = new QHBoxLayout();
	QLabel* label = new QLabel(tr("Staged Files"));
	hBoxLayout->addWidget(label);
	stagedFiles = new TreeView(this);
	TreeModel* treemodel = new TreeModel(repo, this);
	TreeProxy* treewrapperStaged = new TreeProxy(true, this);
	treewrapperStaged->setSourceModel(treemodel);
	stagedFiles->setModel(treewrapperStaged);
	stagedFiles->setHeaderHidden(true);
	stagedFiles->setItemDelegateForColumn(0, new ViewDelegate());
	vBoxLayout->addLayout(hBoxLayout);
	hBoxLayout = new QHBoxLayout();
	collapseButtonStagedFiles = new QPushButton("Collapse all", this);
	hBoxLayout->addWidget(collapseButtonStagedFiles);
	//hBoxLayout->addItem(new QSpacerItem(40,20));
	vBoxLayout->addLayout(hBoxLayout);
	vBoxLayout->addWidget(stagedFiles);
	QWidget* stagedWidget = new QWidget();
	stagedWidget->setLayout(vBoxLayout);

	vBoxLayout = new QVBoxLayout();
	hBoxLayout = new QHBoxLayout();
	label = new QLabel(tr("Unstaged Files"));
	hBoxLayout->addWidget(label);
	unstagedFiles = new TreeView(this);
	TreeProxy* treewrapperUnstaged = new TreeProxy(false, this);
	treewrapperUnstaged->setSourceModel(treemodel);
	unstagedFiles->setModel(treewrapperUnstaged);
	unstagedFiles->setHeaderHidden(true);
	unstagedFiles->setItemDelegateForColumn(0, new ViewDelegate());
	vBoxLayout->addLayout(hBoxLayout);
	hBoxLayout = new QHBoxLayout();
	collapseButtonUnstagedFiles = new QPushButton("Collapse all", this);
	hBoxLayout->addWidget(collapseButtonUnstagedFiles);
	//hBoxLayout->addItem(new QSpacerItem(40,20));
	vBoxLayout->addWidget(collapseButtonUnstagedFiles);
	vBoxLayout->addWidget(unstagedFiles);
	QWidget* unstagedWidget = new QWidget();
	unstagedWidget->setLayout(vBoxLayout);

	QSplitter *treeViewSplitter = new QSplitter(Qt::Vertical, this);
	treeViewSplitter->setHandleWidth(10);
	treeViewSplitter->addWidget(stagedWidget);
	treeViewSplitter->addWidget(unstagedWidget);
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

	connect(stagedFiles, &TreeView::fileSelected,
			this, &DoubleTreeWidget::fileSelected);

	connect(unstagedFiles, &TreeView::fileSelected,
			this, &DoubleTreeWidget::fileSelected);

	connect(collapseButtonStagedFiles, &QPushButton::clicked, [=](){

	});
}

QString DoubleTreeWidget::selectedFile() const {

}

/*!
 * \brief DoubleTreeWidget::setDiff
 * \param diff
 * \param file
 * \param pathspec
 */
void DoubleTreeWidget::setDiff(const git::Diff &diff,
									const QString &file,
									const QString &pathspec) {
	// Remember selection.
	QString name = file;
	if (name.isEmpty()) {
	  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
	  if (!indexes.isEmpty())
		name = indexes.first().data(Qt::EditRole).toString();
	}

	// Reset model.
	git::Tree tree = RepoView::parentView(this)->tree();
	TreeProxy* proxy = static_cast<TreeProxy *>(stagedFiles->model());
	TreeModel* model = static_cast<TreeModel*>(proxy->sourceModel());
	model->setTree(tree, diff);

	// because of this, the content in the view is shown.
	proxy = static_cast<TreeProxy *>(unstagedFiles->model());
	model = static_cast<TreeModel*>(proxy->sourceModel());
	model->setTree(tree, diff);

	// Clear editor.
	mEditor->clear();

	// Restore selection.
	//selectFile(name);

	// Show the tree view.
	stagedFiles->setVisible(true);
}

void DoubleTreeWidget::selectFile(const QString &file)
{
//  if (file.isEmpty())
//	return;

//  QModelIndex index;
//  QStringList path = file.split("/");
//  QAbstractItemModel *model = mView->model();
//  while (!path.isEmpty()) {
//	QString elem = path.takeFirst();
//	for (int row = 0; row < model->rowCount(index); ++row) {
//	  QModelIndex current = model->index(row, 0, index);
//	  if (model->data(current, Qt::DisplayRole).toString() == elem) {
//		mView->selectionModel()->setCurrentIndex(current, kSelectionFlags);
//		index = current;
//		break;
//	  }
//	}
//  }

//  if (index.isValid())
//	loadEditorContent(index);

//  // FIXME: Selection does not draw correctly in the last column.
//  // Scrolling down to an invisible index is also broken.
}

void DoubleTreeWidget::fileSelected(const QModelIndex &index) {

	if (!index.isValid())
		return;

	QObject* obj = QObject::sender();
	if (obj) {
		TreeView* treeview = static_cast<TreeView*>(obj);
		if (treeview == stagedFiles) {
			unstagedFiles->deselectAll();
			stagedFiles->setFocus();
		} else if (treeview == unstagedFiles) {
			stagedFiles->deselectAll();
			unstagedFiles->setFocus();
		}
	}

	loadEditorContent(index);
}

void DoubleTreeWidget::loadEditorContent(const QModelIndex &index)
{
  QString name = index.data(Qt::EditRole).toString();
  git::Blob blob = index.data(TreeModel::BlobRole).value<git::Blob>();

  QList<git::Commit> commits = RepoView::parentView(this)->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  mEditor->load(name, blob, commit);
}

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
#include "StatePushButton.h"
#include "DiffView/DiffView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QButtonGroup>

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

class SegmentedButton : public QWidget
{
public:
  SegmentedButton(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mLayout = new QHBoxLayout(this);
    mLayout->setContentsMargins(0,0,0,0);
    mLayout->setSpacing(0);
  }

  void addButton(
    QAbstractButton *button,
    const QString &text = QString(),
    bool checkable = false)
  {
    button->setToolTip(text);
    button->setCheckable(checkable);

    mLayout->addWidget(button);
    mButtons.addButton(button, mButtons.buttons().size());

    if (mButtons.buttons().size() > 1) {
      mButtons.buttons().first()->setObjectName("first");
      mButtons.buttons().last()->setObjectName("last");
    }

    for (int i = 1; i < mButtons.buttons().size() - 1; ++i)
      mButtons.buttons().at(i)->setObjectName("middle");
  }

  const QButtonGroup *buttonGroup() const
  {
    return &mButtons;
  }

private:
  QHBoxLayout *mLayout;
  QButtonGroup mButtons;
};

} // anon. namespace

DoubleTreeWidget::DoubleTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
	QVBoxLayout* vBoxLayout = new QVBoxLayout();
	QHBoxLayout* hBoxLayout = new QHBoxLayout();
	QLabel* label = new QLabel(tr("Staged Files"));
	hBoxLayout->addWidget(label);
	stagedFiles = new TreeView(this);
    mTreeModel = new TreeModel(repo, this);
	TreeProxy* treewrapperStaged = new TreeProxy(true, this);
    treewrapperStaged->setSourceModel(mTreeModel);
	stagedFiles->setModel(treewrapperStaged);
	stagedFiles->setHeaderHidden(true);
	stagedFiles->setItemDelegateForColumn(0, new ViewDelegate());
	vBoxLayout->addLayout(hBoxLayout);
	hBoxLayout = new QHBoxLayout();
	collapseButtonStagedFiles = new StatePushButton("Collapse all", "Expand all", this);
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
    treewrapperUnstaged->setSourceModel(mTreeModel);
	unstagedFiles->setModel(treewrapperUnstaged);
	unstagedFiles->setHeaderHidden(true);
	unstagedFiles->setItemDelegateForColumn(0, new ViewDelegate());
	vBoxLayout->addLayout(hBoxLayout);
	hBoxLayout = new QHBoxLayout();
	collapseButtonUnstagedFiles = new StatePushButton("Collapse all", "Expand all", this);
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



    QVBoxLayout* fileViewLayout = new QVBoxLayout();
    mFileView = new QStackedWidget(this);
	mEditor = new BlameEditor(repo, this);
    mDiffView = new DiffView(repo, this);
    assert(mFileView->addWidget(mEditor) == DoubleTreeWidget::Blame);
    assert(mFileView->addWidget(mDiffView) == DoubleTreeWidget::Diff);
    SegmentedButton* segmentedButton = new SegmentedButton(this);
    QPushButton* blameView = new QPushButton("Blame", this);
    segmentedButton->addButton(blameView, "Blame", true);
    blameView->setChecked(true);
    QPushButton* diffView = new QPushButton("Diff", this);
    segmentedButton->addButton(diffView, "Diff", true);
    const QButtonGroup* viewGroup = segmentedButton->buttonGroup();
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addItem(new QSpacerItem(279, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    buttonLayout->addWidget(segmentedButton);
    buttonLayout->addItem(new QSpacerItem(279, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    fileViewLayout->addLayout(buttonLayout);
    fileViewLayout->addWidget(mFileView);
    mFileView->setCurrentIndex(0);
    mFileView->show();
    QWidget* fileView = new QWidget(this);
    fileView->setLayout(fileViewLayout);


	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setHandleWidth(0);
    splitter->addWidget(fileView);
	splitter->addWidget(treeViewSplitter);
	splitter->setStretchFactor(1, 1);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(splitter);

	setLayout(layout);

    connect(viewGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), [this] (int idx) {
        mFileView->setCurrentIndex(idx);
    });


	connect(stagedFiles, &TreeView::fileSelected,
			this, &DoubleTreeWidget::fileSelected);

	connect(unstagedFiles, &TreeView::fileSelected,
			this, &DoubleTreeWidget::fileSelected);

	connect(collapseButtonStagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseStagedFiles);
	connect(collapseButtonUnstagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseUnstagedFiles);
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
	stagedFiles->expandAll();

	// because of this, the content in the view is shown.
	proxy = static_cast<TreeProxy *>(unstagedFiles->model());
	model = static_cast<TreeModel*>(proxy->sourceModel());
	model->setTree(tree, diff);
	unstagedFiles->expandAll();

	// Clear editor.
	mEditor->clear();

    mDiffView->setDiff(diff);

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
  mDiffView->setFilter(QStringList(name));
}

void DoubleTreeWidget::toggleCollapseStagedFiles() {

	if (collapseButtonStagedFiles->toggleState())
		stagedFiles->expandAll();
	else
		stagedFiles->collapseAll();
}

void DoubleTreeWidget::toggleCollapseUnstagedFiles() {

	if (collapseButtonUnstagedFiles->toggleState())
		unstagedFiles->expandAll();
	else
		unstagedFiles->collapseAll();
}

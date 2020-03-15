//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeView.h"
#include "ColumnView.h"
#include "ViewDelegate.h"
#include "TreeModel.h"
#include <QFormLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#define ICON_SIZE 48
#define SCROLL_BAR_WIDTH 18
#else
#define ICON_SIZE 64
#define SCROLL_BAR_WIDTH 0
#endif

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

} // anon. namespace

TreeView::TreeView(QWidget *parent)
  : QTreeView(parent), mSharedDelegate(new ViewDelegate(this))
{
}

void TreeView::setModel(QAbstractItemModel *model)
{
  QTreeView::setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged,
		  this, &TreeView::handleSelectionChange);
}

bool TreeView::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
	QWidget *TreeViewport = static_cast<QWidget *>(obj);
    QPoint globalPos = static_cast<QMouseEvent *>(event)->globalPos();
    QModelIndex index = indexAt(viewport()->mapFromGlobal(globalPos));
	if (!TreeViewport->hasFocus() && index.row() < 0) {
	  TreeViewport->setFocus();
      selectionModel()->clearSelection();
    }
  }

  return false;
}

void TreeView::handleSelectionChange(
  const QItemSelection &selected,
  const QItemSelection &deselected)
{
  // FIXME: The argument sent by Qt doesn't contain the whole selection.
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  QModelIndex index = (indexes.size() == 1) ? indexes.first() : QModelIndex();
  emit fileSelected(index);

  // Handle deselection.
  if (indexes.isEmpty() && !deselected.indexes().isEmpty()) {
    QModelIndex parent = deselected.indexes().first().parent();
    setCurrentIndex(parent);
    if (!parent.isValid())
      setRootIndex(QModelIndex());
  }
}

void TreeView::deselectAll() {
//	QModelIndexList indexes = selectionModel()->selectedIndexes();
//	for (auto index : indexes) {
//		index
//	}
}

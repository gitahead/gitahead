//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeWrapper.h"
#include "TreeModel.h"

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";

} // anon. namespace

TreeWrapper::TreeWrapper(TreeModel* model, QObject *parent):
	mModel(model),
	QAbstractItemModel(parent)
{

}

TreeWrapper::~TreeWrapper()
{
}

//void TreeWrapper::setTreeModel(TreeModel* model) {
//	mModel = model;
//}

void TreeWrapper::setTree(const git::Tree &tree, const git::Diff &diff)
{
  beginResetModel();
  mModel->setTree(tree, diff);
  endResetModel();
}

int TreeWrapper::rowCount(const QModelIndex &parent) const
{
	int count = mModel->rowCount(parent);
  return count;
}

int TreeWrapper::columnCount(const QModelIndex &parent) const
{
  return mModel->columnCount(parent);
}

bool TreeWrapper::hasChildren(const QModelIndex &parent) const
{
  return mModel->hasChildren(parent);
}

QModelIndex TreeWrapper::parent(const QModelIndex &index) const
{
  return mModel->parent(index);
}

QModelIndex TreeWrapper::index(
  int row,
  int column,
  const QModelIndex &parent) const
{
	QModelIndex index = mModel->index(row, column, parent);
  return index;
}

QVariant TreeWrapper::data(const QModelIndex &index, int role) const
{
	return mModel->data(index, role);
}

bool TreeWrapper::setData(
  const QModelIndex &index,
  const QVariant &value,
  int role)
{
	mModel->setData(index, value, role);
}

Qt::ItemFlags TreeWrapper::flags(const QModelIndex &index) const
{
	mModel->flags(index);
}

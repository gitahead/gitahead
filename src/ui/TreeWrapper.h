//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TREEWRAPPER
#define TREEWRAPPER

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QAbstractItemModel>
#include <QFileIconProvider>

class TreeModel;

class TreeWrapper : public QAbstractItemModel
{
  Q_OBJECT

public:

  TreeWrapper(TreeModel *model, QObject *parent);
  virtual ~TreeWrapper();

	//void setTreeModel(TreeModel* model);
  void setTree(const git::Tree &tree, const git::Diff &diff = git::Diff());

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

  QModelIndex parent(const QModelIndex &index) const override;
  QModelIndex index(
    int row,
    int column,
    const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(
    const QModelIndex &index,
    int role = Qt::DisplayRole) const override;
  bool setData(
    const QModelIndex &index,
    const QVariant &value,
    int role = Qt::EditRole) override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
  TreeModel* mModel{nullptr};
};

#endif // TREEWRAPPER

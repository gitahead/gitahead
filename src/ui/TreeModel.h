//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TREEMODEL
#define TREEMODEL

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QAbstractItemModel>
#include <QFileIconProvider>

class TreeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum Role
  {
    BlobRole = Qt::UserRole,
    KindRole,
    AddedRole,
    ModifiedRole,
    StatusRole
  };

  TreeModel(
    const git::Repository &repo,
    QObject *parent = nullptr);
  virtual ~TreeModel();

  void setTree(
    const git::Tree &tree,
    const git::Diff &diff,
    const git::Index &index);

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
  class Node
  {
  public:
    Node(const QString &name, const git::Object &obj, Node *parent = nullptr);
    ~Node();

    QString name() const;
    QString path(bool relative = false) const;

    Node *parent() const;
    bool hasChildren() const;
    QList<Node *> children();

    git::Object object() const;

  private:
    QString mName;
    git::Object mObject;

    Node *mParent;
    QList<Node *> mChildren;
  };

  Node *node(const QModelIndex &index) const;

  Node *mRoot = nullptr;
  QFileIconProvider mIconProvider;

  git::Diff mDiff;
  git::Index mIndex;
  git::Repository mRepo;
};

#endif

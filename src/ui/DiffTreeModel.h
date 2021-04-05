//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFTREEMODEL
#define DIFFTREEMODEL

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QAbstractItemModel>
#include <QFileIconProvider>

/*!
 * \brief The DiffTreeModel class
 * This Treemodel is similar to the normal tree model, but handles only the files in the diff it self
 * and not the complete tree
 */
class DiffTreeModel : public QAbstractItemModel
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

  DiffTreeModel(
    const git::Repository &repo,
    QObject *parent = nullptr);
  virtual ~DiffTreeModel();

  void setDiff(const git::Diff &diff = git::Diff());

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

  void createDiffTree();
  /*!
   * Setting the data to the item
   * \brief setData
   * \param index
   * \param value
   * \param role
   * \param ignoreIndexChanges If index changes should be ignored or not.
   * In normal case it is desired that the index is changed when checking an item,
   * but when the data was changed outside of the model (like in the DiffView) the
   * index must not be updated anymore because it is done already.
   * \return
   */
  bool setData(
    const QModelIndex &index,
    const QVariant &value,
    int role, bool ignoreIndexChanges = false);

  Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
  void checkStateChanged(const QModelIndex& index, int state);

private:
  class Node // item of the model
  {
  public:
	  Node(const QString &name, Node *parent = nullptr);
    ~Node();

    QString name() const;
    QString path(bool relative = false) const;

    Node *parent() const;
    bool hasChildren() const;
	QList<Node *> children();
	void addChild(const QStringList& pathPart, int indexFirstDifferent);

  private:
    QString mName;

    Node *mParent;
    QList<Node *> mChildren;
  };

  Node *node(const QModelIndex &index) const;

  QFileIconProvider mIconProvider;

  git::Diff mDiff;
  Node *mRoot{nullptr};
  git::Repository mRepo;
};

#endif /* DIFFTREEMODEL */

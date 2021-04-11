//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffTreeModel.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Diff.h"
#include "git/RevWalk.h"
#include "git/Submodule.h"
#include <QStringBuilder>
#include <QUrl>

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";

} // anon. namespace

DiffTreeModel::DiffTreeModel(const git::Repository &repo, QObject *parent)
  : QAbstractItemModel(parent), mRepo(repo)
{}

DiffTreeModel::~DiffTreeModel()
{
  delete mRoot;
}

void DiffTreeModel::createDiffTree()
{

    for (int patchNum = 0; patchNum < mDiff.count(); ++patchNum) {
        QString path = mDiff.name(patchNum);
        auto pathParts = path.split("/");
        mRoot->addChild(pathParts, patchNum, 0);
    }
}

void DiffTreeModel::setDiff(const git::Diff &diff)
{
  beginResetModel();

  if (diff) {
      delete mRoot;
      mDiff = diff;
      mRoot = new Node(mRepo.workdir().path(), -1);
      createDiffTree();
  }

  endResetModel();
}

int DiffTreeModel::rowCount(const QModelIndex &parent) const
{
  return mDiff ? node(parent)->children().size() : 0;
}

int DiffTreeModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

bool DiffTreeModel::hasChildren(const QModelIndex &parent) const
{
  return mRoot && node(parent)->hasChildren();
}

int DiffTreeModel::fileCount(const QModelIndex& parent) const
{
    if (!mRoot)
        return 0;

    return node(parent)->fileCount();
}

QList<int> DiffTreeModel::patchIndices(const QModelIndex& parent) const
{
    QList<int> list;
    if (!mRoot)
        return list;

    node(parent)->patchIndices(list);
    return list;
}

QModelIndex DiffTreeModel::parent(const QModelIndex &index) const
{
  Node *parent = node(index)->parent();
  if (!parent || parent == mRoot)
    return QModelIndex();

  Q_ASSERT(parent->parent()); // because parent is not root
  return createIndex(parent->parent()->children().indexOf(parent), 0, parent);
}

QModelIndex DiffTreeModel::index(
  int row,
  int column,
  const QModelIndex &parent) const
{
  if (row < 0 || row >= rowCount(parent) ||
      column < 0 || column >= columnCount(parent))
    return QModelIndex();

  auto n = node(parent)->children().at(row);
  return createIndex(row, column, n);
}

QVariant DiffTreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  Node *node = this->node(index);
  switch (role) {
    case Qt::DisplayRole:
      return node->name();

//    case Qt::DecorationRole: {
//      QFileInfo info(node->path());
//      return info.exists() ? mIconProvindexider.icon(info) :
//        mIconProvider.icon(QFileIconProvider::File);
//    }

    case Qt::EditRole:
      return node->path(true);

    case Qt::ToolTipRole:
      return node->path();

    case Qt::CheckStateRole: {
      if (!mDiff.isValid() || !mDiff.isStatusDiff())
        return QVariant();

      git::Index index = mDiff.index();
      return node->stageState(index, Node::ParentStageState::Any);
    }

    case KindRole: {
      Settings *settings = Settings::instance();
      git::Submodule submodule = mRepo.lookupSubmodule(node->path(true));
      return submodule.isValid() ? tr("Submodule") : settings->kind(node->name());
    }

    case AddedRole:
    case ModifiedRole: {
      int sort = GIT_SORT_TIME;
      if (role == AddedRole)
        sort |= GIT_SORT_REVERSE;
      git::RevWalk walker = mRepo.walker(sort);
      git::Commit commit = walker.next(node->path(true));
      if (!commit.isValid())
        return QVariant();

      QUrl url;
      url.setScheme("id");
      url.setPath(commit.id().toString());
      return kLinkFmt.arg(url.toString(), commit.shortId());
    }

    case StatusRole: {
      if (!mDiff.isValid())
        return QString();

      QString status;
      QList<int> patchIndices;
      node->patchIndices(patchIndices);
      for (auto patchIndex: patchIndices) {
          QChar ch = git::Diff::statusChar(mDiff.status(patchIndex));
          if (!status.contains(ch))
            status.append(ch);
      }

      return status;
    }
  }

  return QVariant();
}

bool DiffTreeModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role)
{
    return setData(index, value, role, false);
}

bool DiffTreeModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role,
                        bool ignoreIndexChanges)
{
  switch (role) {
    case Qt::CheckStateRole: {
      QStringList files;
      Node *node = this->node(index);
      QString prefix = node->path(true);

      if (!ignoreIndexChanges) {
          QStringList files;
          node->childFiles(files);
          mDiff.index().setStaged(files, value.toBool());
      }

	  // childs
	  if (hasChildren(index)) {
		  // emit dataChanged() for all files in the folder
		  // all children changed too. TODO: only the tracked files should emit a signal
		  int count = rowCount(index);
		  for (int row = 0; row < count; row++) {
			  QModelIndex child = this->index(row, 0, index);
			  emit dataChanged(child, child, {role});
		  }
	  }
	  // parents
	  // recursive approach to emit signal dataChanged also for the parents.
	  // Because when a file in a folder is staged, the state of the folder changes too
	  QModelIndex parent = this->parent(index);
	  while (parent.isValid()) {
		  emit dataChanged(parent, parent, {role});
		  parent = this->parent(parent);
	  }

	  // file/folder it self
	  // emit dataChanged() for folder or file it self
	  emit dataChanged(index, index, {role});
      emit checkStateChanged(index, value.toInt());

      return true;
    }
  }

  return false;
}

Qt::ItemFlags DiffTreeModel::flags(const QModelIndex &index) const
{
  return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
}

DiffTreeModel::Node *DiffTreeModel::node(const QModelIndex &index) const
{
  return index.isValid() ? static_cast<Node *>(index.internalPointer()) : mRoot;
}

//#############################################################################
//######     DiffTreeModel::Node     ##############################################
//#############################################################################

DiffTreeModel::Node::Node(const QString &name, int patchIndex, Node *parent)
  : mName(name), mPatchIndex(patchIndex), mParent(parent)
{}

DiffTreeModel::Node::~Node()
{
  qDeleteAll(mChildren);
}

QString DiffTreeModel::Node::name() const
{
  return mName;
}

QString DiffTreeModel::Node::path(bool relative) const
{
  bool root = (!mParent || (relative && !mParent->mParent));
  return !root ? mParent->path(relative) % "/" % mName : mName;
}

DiffTreeModel::Node *DiffTreeModel::Node::parent() const
{
  return mParent;
}

bool DiffTreeModel::Node::hasChildren() const
{
  return mChildren.length() > 0;
}

QList<DiffTreeModel::Node *> DiffTreeModel::Node::children()
{
  return mChildren;
}

void DiffTreeModel::Node::addChild(const QStringList& pathPart, int patchIndex, int indexFirstDifferent)
{
    for (auto c: mChildren) {
        if (c->name() == pathPart[indexFirstDifferent]) {
            c->addChild(pathPart, patchIndex, indexFirstDifferent + 1);
            return;
        }
    }

    Node* n;
    if (indexFirstDifferent + 1 < pathPart.length()) {
        // folders cannot have a patch Index!
        n = new Node(pathPart[indexFirstDifferent], -1, this);
        n->addChild(pathPart, patchIndex, indexFirstDifferent + 1);
    } else
        n = new Node(pathPart[indexFirstDifferent], patchIndex, this);
    mChildren.append(n);
}

git::Index::StagedState DiffTreeModel::Node::stageState(const git::Index& idx, ParentStageState searchingState)
{
    if (!hasChildren())
        return idx.isStaged(path(true));

    git::Index::StagedState childState;
    for (auto child: mChildren) {

        childState = child->stageState(idx, searchingState);
        if ((childState == git::Index::StagedState::Staged && searchingState == ParentStageState::Unstaged) ||
            (childState == git::Index::StagedState::Unstaged && searchingState == ParentStageState::Staged) ||
            childState == git::Index::PartiallyStaged)
            return git::Index::PartiallyStaged;

        if (searchingState == ParentStageState::Any) {
            if (childState == git::Index::Unstaged)
                searchingState = ParentStageState::Unstaged;
            else if (childState == git::Index::Staged)
                searchingState = ParentStageState::Staged;
        }
    }

    return childState;
}

void DiffTreeModel::Node::childFiles(QStringList& files)
{
    if (!hasChildren()) {
        files.append(path(true));
        return;
    }

    for (auto child: mChildren)
        child->childFiles(files);
}

int  DiffTreeModel::Node::fileCount() const
{
    if (!hasChildren())
        return 1;

    int count = 0;
    for (auto child: mChildren)
        count += child->fileCount();

    return count;
}

void DiffTreeModel::Node::patchIndices(QList<int>& list)
{
    if (!hasChildren()) {
        assert(patchIndex() >= 0);
        list.append(patchIndex());
        return;
    }

    for (auto child: mChildren)
        child->patchIndices(list);
}

int DiffTreeModel::Node::patchIndex() const
{
    return mPatchIndex;
}

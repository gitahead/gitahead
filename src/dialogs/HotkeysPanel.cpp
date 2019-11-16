//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "HotkeysPanel.h"
#include "ui/HotkeyManager.h"
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QKeySequence>
#include <QMap>
#include <QModelIndex>
#include <QRegularExpression>
#include <QString>
#include <QVector>

namespace
{
class HotkeyGroupData;
class HotkeyKeyData;

class HotkeyData : public QObject
{
public:
  enum class ObjectType
  {
    Key,
    Group
  };

  HotkeyData(QObject *parent, HotkeyGroupData *group, const QString &label)
    : QObject(parent), mGroup(group), mLabel(label)
  {
  }

  QString label() const
  {
    return mLabel;
  }

  HotkeyGroupData *group() const
  {
    return mGroup;
  }

  virtual ObjectType type() const = 0;

private:
  HotkeyGroupData *mGroup;
  QString mLabel;
};

class HotkeyGroupData : public HotkeyData
{
public:
  HotkeyGroupData(
    QObject *parent,
    const QString &label,
    HotkeyGroupData *group = nullptr
  ) : HotkeyData(parent, group, label)
  {
  }

  virtual ObjectType type() const
  {
    return ObjectType::Group;
  }

  QVector<HotkeyData*> childrenData() const
  {
    return mChildren;
  }

  void addChildData(HotkeyData *data)
  {
    mChildren.append(data);
  }

private:
  QVector<HotkeyData*> mChildren;
};

class HotkeyKeyData : public HotkeyData
{
public:
  HotkeyKeyData(HotkeyGroupData *group, const QString &label, Hotkey hotkey, HotkeyManager *manager)
    : HotkeyData(group, group, label), mHotkey(hotkey), mManager(manager)
  {
    mKeys = hotkey.currentKeys(manager);
  }

  virtual ObjectType type() const
  {
    return ObjectType::Key;
  }

  QKeySequence keys() const
  {
    return mKeys;
  }

  void setKeys(QKeySequence keys)
  {
    mHotkey.setKeys(keys, mManager);
    mKeys = keys;
  }

private:
  Hotkey mHotkey;
  HotkeyManager *mManager;
  QKeySequence mKeys;
};

class HotkeyModel : public QAbstractItemModel
{
public:
  enum class ColumnIndex : int
  {
    Min = 0,
    Label = 0,
    Keys = 1,
    Max = 1
  };

  HotkeyModel(QObject *parent, HotkeyGroupData *root): QAbstractItemModel(parent), mRoot(root)
  {
  }

  virtual int columnCount(const QModelIndex &parent) const
  {
      return 2;
  }

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const
  {
    if (!parent.isValid())
      return mRoot->childrenData().length();
    
    HotkeyData *data = (HotkeyData*)parent.internalPointer();
    if(data->type() != HotkeyData::ObjectType::Group)
      return 0;

    return ((HotkeyGroupData*)data)->childrenData().length();
  }

  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
  {
    if (row < 0 || column < (int)ColumnIndex::Min || column > (int)ColumnIndex::Max)
      return QModelIndex();

    HotkeyGroupData *group = nullptr;
    if (parent.isValid()) {
      HotkeyData *data = (HotkeyData*)parent.internalPointer();
      if(data->type() == HotkeyData::ObjectType::Group)
        group = (HotkeyGroupData*)data;

    } else {
      group = mRoot;
    }

    if(!group || row >= group->childrenData().length())
      return QModelIndex();

    return createIndex(row, column, group->childrenData()[row]);
  }

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
  {
    if (
      role != Qt::DisplayRole
      || !index.isValid()
      || index.column() < (int)ColumnIndex::Min
      || index.column() > (int)ColumnIndex::Max
    )
      return QVariant();

    HotkeyData *data = (HotkeyData*)index.internalPointer();

    switch ((ColumnIndex)index.column()) {
      case ColumnIndex::Label:
        return QVariant(data->label());
      
      case ColumnIndex::Keys:
        if(data->type() == HotkeyData::ObjectType::Key)
          return QVariant(((HotkeyKeyData*)data)->keys().toString(QKeySequence::NativeText));
        else
          return QVariant();
    }

    Q_ASSERT(false);
  }

  virtual QModelIndex parent(const QModelIndex &index) const
  {
    if (!index.isValid() || index.column() != 0)
      return QModelIndex();

    HotkeyData *data = (HotkeyData*)index.internalPointer();
    HotkeyGroupData *parent = data->group();

    if(!parent)
      return QModelIndex();
    else
      return createIndex(0, 0, parent);
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const
  {
    if (
      orientation != Qt::Orientation::Horizontal
      || section < (int)ColumnIndex::Min
      || section > (int)ColumnIndex::Max
      || role != Qt::DisplayRole
    )
      return QVariant();

    switch ((ColumnIndex)section) {
      case ColumnIndex::Label:
        return QVariant(tr("Action"));
      
      case ColumnIndex::Keys:
        return QVariant(tr("Keys"));
    }

    Q_ASSERT(false);
  }

private:
  HotkeyGroupData *mRoot;
};
}

HotkeysPanel::HotkeysPanel(QWidget *parent) : QTreeView(parent)
{
  static QRegularExpression slashRegex("/+");

  HotkeyManager *manager = HotkeyManager::instance();

  QMap<QString, HotkeyGroupData*> groups;
  HotkeyGroupData *root = new HotkeyGroupData(this, "<Root>");

  // Build hotkey hierarchy
  for (Hotkey hotkey: manager->knownHotkeys()) {
    QString label = hotkey.label().replace(slashRegex, "/");
    int lastSep = label.lastIndexOf('/');

    HotkeyGroupData *group;

    // Look for existing group along hierarchy
    int pos = lastSep;
    while (pos >= 0) {
      group = groups.value(label.left(pos));
      if (group)
        break;

      pos = label.lastIndexOf('/', pos - 1);
    }

    // Use root if no matching group has been found
    if (!group) {
      Q_ASSERT(pos < 0);
      group = root;
      pos = 0;
    }

    // Build and insert missing groups
    while (pos < lastSep && pos >= 0) {
      pos = label.indexOf('/', pos + 1);
      QString path = label.left(pos);

      int subPos = path.lastIndexOf('/');

      group = new HotkeyGroupData(group, tr(path.mid(subPos + 1).toUtf8()), group);
      group->group()->addChildData(group);

      Q_ASSERT(!groups.contains(path));
      groups.insert(path, group);
    }

    // Add hotkey to group
    group->addChildData(new HotkeyKeyData(group, tr(label.mid(lastSep + 1).toUtf8()), hotkey, manager));
  }

  setModel(new HotkeyModel(this, root));
  setUniformRowHeights(true);
  header()->setSectionsMovable(false);
  header()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
  header()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
}

QSize HotkeysPanel::sizeHint() const
{
  return QSize(600, 320);
}
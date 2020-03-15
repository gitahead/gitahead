//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QTreeView>

class QItemDelegate;

class TreeView : public QTreeView
{
  Q_OBJECT

public:
  TreeView(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

signals:
  void linkActivated(const QString &link);
  void fileSelected(const QModelIndex &index);

private:
  void handleSelectionChange(
    const QItemSelection &selected,
    const QItemSelection &deselected);
  void deselectAll();

  QItemDelegate *mSharedDelegate;
};

#endif // TREEVIEW_H

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFVIEW_H
#define DIFFVIEW_H

#include "FindWidget.h"
#include "editor/TextEditor.h"
#include "git/Commit.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "host/Account.h"
#include "plugins/Plugin.h"
#include <QMap>
#include <QScrollArea>

class QCheckBox;
class QVBoxLayout;

class DiffView : public QScrollArea, public EditorProvider
{
  Q_OBJECT

public:
  DiffView(const git::Repository &repo, QWidget *parent = nullptr);
  virtual ~DiffView();

  QWidget *file(int index);

  void setDiff(const git::Diff &diff);

  bool scrollToFile(int index);
  void setFilter(const QStringList &paths);

  const QList<PluginRef> &plugins() const { return mPlugins; }
  const Account::CommitComments &comments() const { return mComments; }

  QList<TextEditor *> editors() override;
  void ensureVisible(TextEditor *editor, int pos) override;

signals:
  void diagnosticAdded(TextEditor::DiagnosticKind kind);

protected:
  void dropEvent(QDropEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;

private:
  bool canFetchMore();
  void fetchMore();
  void fetchAll(int index = -1);

  git::Diff mDiff;
  QMap<QString,git::Patch> mStagedPatches;

  QList<QWidget *> mFiles;
  QList<QMetaObject::Connection> mConnections;

  QList<PluginRef> mPlugins;
  Account::CommitComments mComments;
};

#endif

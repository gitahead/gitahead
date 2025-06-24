//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REPOVIEW_H
#define REPOVIEW_H

#include "dialogs/ConfigDialog.h"
#include "git/AnnotatedCommit.h"
#include "git/Blob.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Reference.h"
#include "git/Remote.h"
#include "git/Repository.h"
#include "git/Submodule.h"
#include "host/Account.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QProcess>
#include <QSplitter>
#include <QTimer>
#include <functional>

class CommitList;
class DetailView;
class EditorWindow;
class History;
class Index;
class Location;
class LogEntry;
class LogView;
class MainWindow;
class PathspecWidget;
class ReferenceWidget;
class RemoteCallbacks;
class ToolBar;

namespace git {
class Result;
}

class RepoView : public QSplitter
{
  Q_OBJECT

public:
  enum ViewMode
  {
    Diff,
    Tree
  };

  enum MergeFlag
  {
    Default       = 0x0,
    Merge         = 0x1,
    Rebase        = 0x2,
    FastForward   = 0x4,  // equivalent to --ff-only
    NoFastForward = 0x8,  // equivalent to --no-ff
    NoCommit      = 0x10, // equivalent to --no-commit
    Squash        = 0x20
  };

  Q_DECLARE_FLAGS(MergeFlags, MergeFlag);

  RepoView(const git::Repository &repo, MainWindow *parent);
  virtual ~RepoView();

  //clean
  void clean(const QStringList &untracked);

  // Select head reference in ref list.
  void selectHead();
  void selectFirstCommit();

  git::Repository repo() const { return mRepo; }
  History *history() const { return mHistory; }
  Index *index() const { return mIndex; }

  Repository *remoteRepo();

  // LFS
  void lfsInitialize();
  void lfsDeinitialize();
  bool lfsSetLocked(const QStringList &paths, bool locked);

  // commit
  void commit();
  bool isCommitEnabled() const;

  // stage / unstage
  void stage();
  bool isStageEnabled() const;
  void unstage();
  bool isUnstageEnabled() const;

  // mode
  ViewMode viewMode() const;
  void setViewMode(ViewMode mode);

  // workdir
  bool isWorkingDirectoryDirty() const;

  // current reference
  git::Reference reference() const;
  void selectReference(const git::Reference &ref);

  // current selection
  QList<git::Commit> commits() const;
  git::Diff diff() const;
  git::Tree tree() const;

  // background tasks
  void cancelRemoteTransfer();
  void cancelBackgroundTasks();

  // links
  void visitLink(const QString &link);

  // history location
  Location location() const;
  void setLocation(const Location &location);

  // history
  void find();
  void findNext();
  void findPrevious();

  // search index
  void startIndexing();
  void cancelIndexing();

  // log window
  bool isLogVisible() const;
  void setLogVisible(bool visible);

  LogEntry *addLogEntry(
    const QString &text,
    const QString &title,
    LogEntry *parent = nullptr);
  LogEntry *error(
    LogEntry *parent,
    const QString &action,
    const QString &name = QString(),
    const QString &defaultError = QString());

  // automatic fetch
  void startFetchTimer();

  // fetch
  void fetchAll();
  QFuture<git::Result> fetch(
    const git::Remote &remote = git::Remote(),
    bool tags = false,
    bool interactive = true,
    LogEntry *parent = nullptr,
    QStringList *submodules = nullptr);
  QFuture<git::Result> fetch(
    const git::Remote &remote,
    bool tags,
    bool interactive,
    LogEntry *parent,
    QStringList *submodules,
    bool prune);

  // pull
  void pull(
    MergeFlags flags = Default,
    const git::Remote &remote = git::Remote(),
    bool tags = false,
    bool prune = false);
  void merge(
    MergeFlags flags,
    const git::Reference &ref = git::Reference(),
    const git::AnnotatedCommit &commit = git::AnnotatedCommit(),
    LogEntry *parent = nullptr,
    const std::function<void()> &callback = std::function<void()>());

  // fast-forward
  void fastForward(
    const git::Reference &ref,
    const git::AnnotatedCommit &upstream,
    LogEntry *parent,
    const std::function<void()> &callback = std::function<void()>());

  // merge
  void merge(
    MergeFlags flags,
    const git::AnnotatedCommit &upstream,
    LogEntry *parent,
    const std::function<void()> &callback = std::function<void()>());
  void mergeAbort(LogEntry *parent = nullptr);

  // rebase
  void rebase(
    const git::AnnotatedCommit &upstream,
    LogEntry *parent,
    const std::function<void()> &callback = std::function<void()>());

  // squash
  void squash(
    const git::AnnotatedCommit &upstream,
    LogEntry *parent);

  // revert
  void revert(const git::Commit &commit);

  // cherry-pick
  void cherryPick(const git::Commit &commit);

  // diff
  void promptToApplyDiff();

  // push
  void promptToForcePush(
    const git::Remote &remote = git::Remote(),
    const git::Reference &src = git::Reference());

  void push(
    const git::Remote &remote = git::Remote(),
    const git::Reference &src = git::Reference(),
    const QString &dst = QString(),
    bool setUpstream = false,
    bool force = false,
    bool tags = false);

  // commit
  bool commit(
    const QString &message,
    const git::AnnotatedCommit &upstream = git::AnnotatedCommit(),
    LogEntry *parent = nullptr,
    bool force = false);
  void amendCommit();

  // checkout
  void promptToCheckout();
  void checkout(const git::Commit &commit, const QStringList &paths);
  void checkout(const git::Reference &ref, bool detach = false);
  void checkout(
    const git::Commit &commit,
    const git::Reference &ref = git::Reference(),
    bool detach = true);

  // branches
  void promptToCreateBranch(const git::Commit &commit = git::Commit());
  git::Branch createBranch(
    const QString &name,
    const git::Commit &target,
    const git::Branch &upstream = git::Branch(),
    bool checkout = false,
    bool force = false);

  // stash
  void promptToStash();
  void stash(const QString &message = QString());
  void applyStash(int index = 0);
  void dropStash(int index = 0);
  void popStash(int index = 0);
  bool canStash() const;

  // tag
  void promptToAddTag(const git::Commit &commit);

  // reset
  void promptToReset(
    const git::Commit &commit,
    git_reset_t type,
    const git::Commit &commitToAmend = git::Commit());
  bool reset(
    const git::Commit &commit,
    git_reset_t type,
    const git::Commit &commitToAmend = git::Commit());

  // submodule
  void updateSubmodules(
    const QList<git::Submodule> &submodules = QList<git::Submodule>(),
    bool recursive = true,
    bool init = false,
    LogEntry *parent = nullptr);
  bool openSubmodule(const git::Submodule &submodule);

  // config
  ConfigDialog *configureSettings(
    ConfigDialog::Index index = ConfigDialog::General);

  // ignore
  void ignore(const QString &name);

  // editor
  EditorWindow *newEditor();
  bool edit(const QString &path, int line = -1);
  EditorWindow *openEditor(
    const QString &path,
    int line = -1,
    const git::Blob &blob = git::Blob(),
    const git::Commit &commit = git::Commit());

  // refresh
  void refresh();

  // pathspec search filter
  void setPathspec(const QString &path);

  git::Commit nextRevision(const QString &path) const;
  git::Commit previousRevision(const QString &path) const;

  void selectCommit(const git::Commit &commit, const QString &file);

  static RepoView *parentView(const QWidget *widget);

signals:
  void statusChanged(bool dirty);

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  struct SubmoduleInfo
  {
    git::Submodule submodule;
    git::Repository repo;
    LogEntry *entry;
  };

  ToolBar *toolBar() const;
  CommitList *commitList() const;

  void notifyReferenceUpdated(const QString &name);

  void startLogTimer();
  bool suspendLogTimer();
  void resumeLogTimer(bool suspended = true);

  QList<SubmoduleInfo> submoduleInfoList(
    const git::Repository &repo,
    const QList<git::Submodule> &submodules,
    bool init,
    LogEntry *parent);
  void updateSubmodulesAsync(
    const QList<SubmoduleInfo> &submodules,
    bool recursive = true,
    bool init = false);

  bool checkForConflicts(LogEntry *parent, const QString &action);

  void applyDiff(const QString &path);

  git::Repository mRepo;

  Index *mIndex;
  QProcess mIndexer;
  bool mRestartIndexer = false;

  History *mHistory;

  Repository *mRemoteRepo;
  bool mRemoteRepoCached = false;

  ReferenceWidget *mRefs;
  PathspecWidget *mPathspec;
  CommitList *mCommits;
  DetailView *mDetails;

  LogEntry *mLogRoot;
  LogView *mLogView;
  QTimer mLogTimer;
  bool mIsLogVisible = false;

  QTimer mFetchTimer;
  RemoteCallbacks *mCallbacks = nullptr;
  QFutureWatcher<git::Result> *mWatcher = nullptr;

  QList<QWidget *> mTrackedWindows;

  bool mShown = false;

  friend class MenuBar;
};

#endif

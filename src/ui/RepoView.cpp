//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RepoView.h"
#include "BlameEditor.h"
#include "CommitList.h"
#include "CommitToolBar.h"
#include "DetailView.h"
#include "EditorWindow.h"
#include "History.h"
#include "MainWindow.h"
#include "MenuBar.h"
#include "PathspecWidget.h"
#include "ReferenceWidget.h"
#include "RemoteCallbacks.h"
#include "SearchField.h"
#include "ToolBar.h"
#include "conf/Settings.h"
#include "dialogs/CheckoutDialog.h"
#include "dialogs/CommitDialog.h"
#include "dialogs/DeleteBranchDialog.h"
#include "dialogs/DeleteTagDialog.h"
#include "dialogs/DiffFileDialog.h"
#include "dialogs/NewBranchDialog.h"
#include "dialogs/RemoteDialog.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/TagDialog.h"
#include "editor/TextEditor.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Rebase.h"
#include "git/RevWalk.h"
#include "git/TagRef.h"
#include "git/Tree.h"
#include "git2/merge.h"
#include "host/Accounts.h"
#include "index/Index.h"
#include "log/LogEntry.h"
#include "log/LogView.h"
#include "watcher/RepositoryWatcher.h"
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QtNetwork>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QTimeLine>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {

const QString kSplitterKey = "splitter";
const QString kMsgFmt = "%1 - <span style='color: gray'>%2</span>";

QString msg(const git::Commit &commit)
{
  QString summary = commit.summary(git::Commit::SubstituteEmoji);
  return kMsgFmt.arg(commit.link(), summary);
}

class CheckoutCallbacks :
  public QObject,
  public git::Repository::CheckoutCallbacks
{
  Q_OBJECT

public:
  CheckoutCallbacks(LogEntry *log, int flags, QObject *parent = nullptr)
    : QObject(parent), mLog(log), mFlags(flags)
  {
    // Connect with automatic type.
    connect(this, &CheckoutCallbacks::queueNotify,
            this, &CheckoutCallbacks::notifyImpl);
  }

  QStringList conflicts() const
  {
    return mConflicts;
  }

  int flags() const override
  {
    return mFlags | GIT_CHECKOUT_NOTIFY_CONFLICT;
  }

  bool notify(char status, const QString &path) override
  {
    emit queueNotify(status, path);
    return true;
  }

  void progress(const QString &path, int current, int total) override
  {
    // Add entries all at once.
    if (current == total && !mEntries.isEmpty())
      mLog->addEntries(mEntries);
  }

signals:
  void queueNotify(char status, const QString &path);

private:
  void notifyImpl(char status, const QString &path)
  {
    LogEntry *entry = new LogEntry(LogEntry::File, path, QString());
    entry->setStatus(status);
    mEntries.append(entry);

    if (status == '!')
      mConflicts.append(path);
  }

  LogEntry *mLog;
  int mFlags;

  QStringList mConflicts;
  QList<LogEntry *> mEntries;
};

class ScopedCollapse
{
public:
  ScopedCollapse(LogView *view)
    : mView(view)
  {
    mView->setCollapseEnabled(false);
  }

  ~ScopedCollapse()
  {
    mView->setCollapseEnabled(true);
  }

private:
  LogView *mView;
};

} // anon. namespace

RepoView::RepoView(const git::Repository &repo, MainWindow *parent)
  : QSplitter(Qt::Vertical, parent), mRepo(repo)
{
  setHandleWidth(0);
  setAttribute(Qt::WA_DeleteOnClose);

  // Start (or restart) indexing after any reference is updated.
  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated,
          this, &RepoView::startIndexing);

  MenuBar *menuBar = MenuBar::instance(parent);
  connect(this, &RepoView::statusChanged,
          menuBar, &MenuBar::updateStash);
  connect(notifier, &git::RepositoryNotifier::stateChanged,
          menuBar, &MenuBar::updateBranch);

  ToolBar *toolBar = parent->toolBar();
  connect(this, &RepoView::statusChanged,
          toolBar, &ToolBar::updateStash);

  // Initialize index.
  mIndex = new Index(repo, this);
  SearchField *searchField = toolBar->searchField();
  connect(&mIndexer, &QProcess::started, [searchField] {
    searchField->setPlaceholderText(tr("Indexing..."));
  });
  connect(&mIndexer, &QProcess::finished,
  [this, searchField](int code, QProcess::ExitStatus status) {
    searchField->setPlaceholderText(tr("Search"));
    if (status == QProcess::CrashExit) {
      QString text =
        tr("The indexer worker process crashed. If this problem "
           "persists please contact us at support@gitahead.com.");
      addLogEntry(text, tr("Indexer Crashed"));
    }

    if (mRestartIndexer) {
      mRestartIndexer = false;
      startIndexing();
    }
  });

  // Forward indexer stderr. Read from stdout.
  mIndexer.setProcessChannelMode(QProcess::ForwardedErrorChannel);
  connect(&mIndexer, &QProcess::readyReadStandardOutput, [this] {
    mIndexer.readAllStandardOutput();
    mIndex->reset();
  });

  // Initialize history.
  mHistory = new History(this);
  connect(mHistory, &History::changed, toolBar, &ToolBar::updateHistory);
  connect(mHistory, &History::changed, menuBar, &MenuBar::updateHistory);
  connect(this, &RepoView::statusChanged, [this](bool dirty) {
    if (!dirty)
      mHistory->clean();
  });

  QWidget *sidebar = new QWidget(this);
  QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
  sidebarLayout->setContentsMargins(0,0,0,0);
  sidebarLayout->setSpacing(0);

  QWidget *header = new QWidget(sidebar);
  QVBoxLayout *headerLayout = new QVBoxLayout(header);
  headerLayout->setContentsMargins(4,4,4,4);
  headerLayout->setSpacing(4);
  sidebarLayout->addWidget(header);

  // Hide references when commit list is filtered.
  connect(searchField, &QLineEdit::textChanged, header,
  [header](const QString &text) {
    header->setVisible(text.simplified().isEmpty());
  });

  // Create header tool bar.
  CommitToolBar *commitToolBar = new CommitToolBar(header);
  headerLayout->addWidget(commitToolBar);

  // Create reference list.
  mRefs = new ReferenceWidget(repo, ReferenceView::AllRefs, header);
  headerLayout->addWidget(mRefs);

  connect(mRefs, &ReferenceWidget::referenceChanged,
          menuBar, &MenuBar::updateBranch);

  // Select HEAD branch when it changes.
  connect(notifier, &git::RepositoryNotifier::referenceUpdated,
  [this](const git::Reference &ref) {
    if (ref.isValid() && ref.isHead()) {
      mRefs->select(ref);

      // Invalidate submodule cache when the HEAD changes.
      mRepo.invalidateSubmoduleCache();
    }
  });

  // Create pathspec chooser.
  mPathspec = new PathspecWidget(repo, header);
  headerLayout->addWidget(mPathspec);

  // Create commit list.
  mCommits = new CommitList(mIndex, sidebar);
  sidebarLayout->addWidget(mCommits);

  connect(commitToolBar, &CommitToolBar::settingsChanged,
          mCommits, &CommitList::resetSettings);
  connect(mRefs, &ReferenceWidget::referenceChanged,
          mCommits, &CommitList::setReference);
  connect(mRefs, &ReferenceWidget::referenceSelected,
          mCommits, &CommitList::selectReference);
  connect(mCommits, &CommitList::statusChanged,
          this, &RepoView::statusChanged);

  // Respond to pathspec change.
  connect(mPathspec, &PathspecWidget::pathspecChanged,
  [this](const QString &pathspec) {
    git::Config config = mRepo.appConfig();
    mCommits->setPathspec(pathspec, config.value<bool>("index.enable", true));
  });

  // Respond to search query change.
  connect(searchField, &SearchField::textChanged,
          mCommits, &CommitList::setFilter);
  connect(mIndex, &Index::indexReset, [this, searchField] {
    mCommits->setFilter(searchField->text());
  });

  mDetails = new DetailView(repo, this);

  // Respond to diff/tree mode change.
  connect(mDetails, &DetailView::viewModeChanged,
  [this](ViewMode mode, bool spontaneous) {
    // Update interface.
    this->toolBar()->updateView();
    MenuBar::instance(this)->updateView();

    // Fake a commit list selection change.
    mCommits->resetSelection(spontaneous);
  });

  // Respond to commit list selection change.
  connect(mCommits, &CommitList::diffSelected,
  [this](const git::Diff &diff, const QString &file, bool spontaneous) {
    mHistory->update(diff.isValid() ? location() : Location(), spontaneous);
    mDetails->setDiff(diff, file, mPathspec->pathspec());
  });

  // Refresh the diff when a whole directory is added to the index.
  // FIXME: This is a workaround.
  connect(notifier, &git::RepositoryNotifier::directoryStaged,
          this, &RepoView::refresh, Qt::QueuedConnection);
  connect(notifier, &git::RepositoryNotifier::directoryAboutToBeStaged,
  [this](const QString &dir, int count, bool &allow) {
    if (!Settings::instance()->prompt(Settings::PromptDirectories))
      return;

    QString title = tr("Stage Directory?");
    QString text = tr("Are you sure you want to stage '%1'?");
    QString info = tr("This will result in the addition of %1 files.");
    QString arg = (count < 0) ? tr("more than 100") : QString::number(count);
    QMessageBox dialog(
      QMessageBox::Question, title, text.arg(dir), QMessageBox::Cancel, this);
    dialog.setInformativeText(info.arg(arg));
    QPushButton *button =
      dialog.addButton(tr("Stage Directory"), QMessageBox::AcceptRole);

    QString cbText = tr("Stop prompting to stage directories");
    QCheckBox *cb = new QCheckBox(cbText, &dialog);
    dialog.setCheckBox(cb);

    dialog.exec();
    allow = (dialog.clickedButton() == button);
    if (cb->isChecked())
      Settings::instance()->setPrompt(Settings::PromptDirectories, false);
  });

  // large file size warning
  connect(notifier, &git::RepositoryNotifier::largeFileAboutToBeStaged,
  [this](const QString &file, int size, bool &allow) {
    if (!Settings::instance()->prompt(Settings::PromptLargeFiles))
      return;

    QString title = tr("Stage Large File?");
    QString fmt = tr("Are you sure you want to stage '%1' with a size of %2?");
    QString text = fmt.arg(file, locale().formattedDataSize(size));
    QMessageBox dialog(
      QMessageBox::Question, title, text, QMessageBox::Cancel, this);
    QPushButton *stage = dialog.addButton(tr("Stage"), QMessageBox::AcceptRole);

    QPushButton *track = nullptr;
    if (this->repo().lfsIsInitialized()) {
      track = dialog.addButton(tr("Track with LFS"), QMessageBox::RejectRole);
      dialog.setInformativeText(
        tr("This repository has LFS enabled. Do you "
           "want to track the file with LFS instead?"));
    }

    QString cbText = tr("Stop prompting to stage large files");
    QCheckBox *cb = new QCheckBox(cbText, &dialog);
    dialog.setCheckBox(cb);

    dialog.exec();
    allow = (dialog.clickedButton() == stage);
    if (cb->isChecked())
      Settings::instance()->setPrompt(Settings::PromptLargeFiles, false);

    if (dialog.clickedButton() == track)
      configureSettings(ConfigDialog::Lfs);
  });

  // Refresh when the workdir changes.
  RepositoryWatcher *watcher = new RepositoryWatcher(repo, this);
  connect(notifier, &git::RepositoryNotifier::referenceUpdated,
          watcher, &RepositoryWatcher::cancelPendingNotification);
  connect(mCommits, &CommitList::statusChanged,
          watcher, &RepositoryWatcher::cancelPendingNotification);

  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(0);
  splitter->addWidget(sidebar);
  splitter->addWidget(mDetails);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 3);
  connect(splitter, &QSplitter::splitterMoved, [this, splitter] {
    QSettings().setValue(kSplitterKey, splitter->saveState());
  });

  // Create log.
  mLogRoot = new LogEntry(this);
  connect(mLogRoot, &LogEntry::errorInserted,
          this, &RepoView::suspendLogTimer);

  mLogView = new LogView(mLogRoot, this);
  connect(mLogView, &LogView::linkActivated,
          this, &RepoView::visitLink);
  connect(mLogView, &LogView::operationCanceled,
          this, &RepoView::cancelRemoteTransfer);

  mLogTimer.setSingleShot(true);
  connect(&mLogTimer, &QTimer::timeout, [this] {
    setLogVisible(false);
  });

  QShortcut *esc = new QShortcut(tr("Esc"), mLogView);
  esc->setContext(Qt::WidgetWithChildrenShortcut);
  connect(esc, &QShortcut::activated, mLogView, [this] {
    setLogVisible(false);
  });

  connect(notifier, &git::RepositoryNotifier::indexStageError, this, [this] {
    error(mLogRoot, tr("stage"));
  });

  QObject *context = new QObject(this);
  connect(notifier, &git::RepositoryNotifier::lfsNotFound,
  context, [this, context] {
    QString text =
      tr("Git LFS was not found on the PATH. "
         "<a href='https://git-lfs.github.com'>"
         "Install Git LFS</a> to use LFS integration.");
    mLogRoot->addEntry(LogEntry::Error, text);
    delete context; // Disconnect after the first error.
  });

  // Automatically hide the log when the model changes.
  connect(mLogView->model(), &QAbstractItemModel::rowsInserted,
          this, &RepoView::startLogTimer);
  connect(mLogView->model(), &QAbstractItemModel::dataChanged,
          this, &RepoView::startLogTimer);

  addWidget(splitter);
  addWidget(mLogView);
  setCollapsible(0, false);
  setStretchFactor(0, 1);
  setSizes({1, 0});

  connect(this, &QSplitter::splitterMoved, [this] {
    mIsLogVisible = (sizes().last() > 0);
  });

  // Restore splitter state.
  splitter->restoreState(QSettings().value(kSplitterKey).toByteArray());

  // Connect automatic fetch timer.
  connect(&mFetchTimer, &QTimer::timeout, [this] {
    fetch(git::Remote(), false, false);
  });
}

RepoView::~RepoView()
{
  // Work around crash caused by clearing focus from the commit list
  // when it's destroyed. If it gets destroyed after the detail view
  // then the focus change may trigger the menu bar to query the mode
  // index from the already destroyed detail view.
  mCommits->clearFocus();
}

void RepoView::clean(const QStringList &untracked)
{
  QString singular = tr("untracked file");
  QString plural = tr("untracked files");
  QString phrase = (untracked.count() == 1) ? singular : plural;
  QMessageBox mb(QMessageBox::Warning, tr("Remove Untracked Files"),
                 tr("Remove %1 %2?").arg(
                   QString::number(untracked.count()), phrase));
  mb.setInformativeText(tr("This action cannot be undone."));

  QPushButton *remove = mb.addButton(tr("Remove"), QMessageBox::AcceptRole);
  mb.addButton(QMessageBox::Cancel);
  mb.setDefaultButton(remove);
  mb.exec();

  if (mb.clickedButton() == remove) {
    foreach (const QString &name, untracked)
      repo().clean(name);
  }
}

void RepoView::selectHead()
{
  mRefs->select(mRepo.head());
}

void RepoView::selectFirstCommit()
{
  mCommits->selectFirstCommit();
}

void RepoView::commit()
{
  mDetails->commit();
}

bool RepoView::isCommitEnabled() const
{
  return mDetails->isCommitEnabled();
}

void RepoView::stage()
{
  mDetails->stage();
}

bool RepoView::isStageEnabled() const
{
  return mDetails->isStageEnabled();
}

void RepoView::unstage()
{
  mDetails->unstage();
}

bool RepoView::isUnstageEnabled() const
{
  return mDetails->isUnstageEnabled();
}

RepoView::ViewMode RepoView::viewMode() const
{
  return mDetails->viewMode();
}

void RepoView::setViewMode(ViewMode mode)
{
  mDetails->setViewMode(mode, true);
}

bool RepoView::isWorkingDirectoryDirty() const
{
  git::Diff status = mCommits->status();
  if (!status.isValid())
    return false;

  // FIXME: Add option to stash untracked files?
  int count = status.count();
  for (int i = 0; i < count; ++i) {
    if (status.status(i) != GIT_DELTA_UNTRACKED)
      return true;
  }

  return false;
}

git::Reference RepoView::reference() const
{
  return mRefs->currentReference();
}

void RepoView::selectReference(const git::Reference &ref)
{
  mRefs->select(ref);
}

QList<git::Commit> RepoView::commits() const
{
  return mCommits->selectedCommits();
}

git::Diff RepoView::diff() const
{
  return mCommits->selectedDiff();
}

git::Tree RepoView::tree() const
{
  QList<git::Commit> commits = mCommits->selectedCommits();
  if (!commits.isEmpty())
    return commits.first().tree();

  git::Diff diff = mCommits->selectedDiff();
  return diff.isValid() ? mRepo.index().writeTree() : git::Tree();
}

void RepoView::cancelRemoteTransfer()
{
  if (!mCallbacks)
    return;

  mCallbacks->setCanceled(true);
  QCoreApplication::processEvents();
  if (mWatcher && mWatcher->isRunning())
    mWatcher->waitForFinished();
}

void RepoView::cancelBackgroundTasks()
{
  cancelIndexing();
  cancelRemoteTransfer();
  mCommits->cancelStatus();
  mDetails->cancelBackgroundTasks();
}

void RepoView::visitLink(const QString &link)
{
  ScopedCollapse collapse(mLogView);
  (void) collapse;

  QUrl url(link);
  QUrlQuery query(url.query());

  if (url.scheme() == "http" || url.scheme() == "https")
    QDesktopServices::openUrl(url);

  // Lookup reference.
  git::Reference ref;
  QString refName = query.queryItemValue("ref");
  if (!refName.isEmpty())
    ref = mRepo.lookupRef(refName);

  // commit id
  if (url.scheme() == "id") {
    if (ref.isValid())
      mRefs->select(ref);
    mCommits->selectRange(url.path(), query.queryItemValue("file"), true);
    return;
  }

  // submodule
  if (url.scheme() == "submodule") {
    openSubmodule(mRepo.lookupSubmodule(url.path()));
    return;
  }

  // actions
  if (url.scheme() != "action")
    return;

  QString action = url.path();
  if (action == "pull") {
    pull();
    return;
  }

  if (action == "push") {
    git::Remote remote;

    QString value = query.queryItemValue("to");
    if (!value.isEmpty())
      remote = mRepo.lookupRemote(value);

    if (query.queryItemValue("force") == "true") {
      promptToForcePush(remote, ref);
    } else {
      bool setUpstream = query.queryItemValue("set-upstream") == "true";
      push(remote, ref, QString(), setUpstream);
    }

    return;
  }

  if (action == "push-to") {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Push, this);
    dialog->open();
  }

  if (action == "add-remote") {
    ConfigDialog *dialog = configureSettings(ConfigDialog::Remotes);
    dialog->addRemote(query.queryItemValue("name"));
    return;
  }

  if (action == "stash") {
    promptToStash();
    return;
  }

  if (action == "unstash") {
    popStash();
    return;
  }

  if (action == "checkout") {
    if (ref.isValid()) {
      checkout(ref, query.queryItemValue("detach") == "true");
    } else {
      promptToCheckout();
    }

    return;
  }

  if (action == "fast-forward") {
    merge(FastForward, ref);
    return;
  }

  // Check for no-ff flag.
  MergeFlags flags;
  if (query.queryItemValue("no-ff") == "true")
    flags |= NoFastForward;

  if (action == "merge") {
    merge(flags | Merge, ref);
    return;
  }

  if (action == "rebase") {
    merge(flags | Rebase, ref);
    return;
  }

  if (action == "config") {
    if (query.queryItemValue("global") == "true") {
      SettingsDialog::openSharedInstance();
    } else {
      configureSettings(ConfigDialog::General);
    }

    return;
  }

  if (action == "amend") {
    amendCommit();
    return;
  }

  if (action == "revert") {
    revert(mRepo.lookupCommit(query.queryItemValue("id")));
    return;
  }

  if (action == "cherry-pick") {
    cherryPick(mRepo.lookupCommit(query.queryItemValue("id")));
    return;
  }

  if (action == "abort") {
    mergeAbort();
    return;
  }
}

Repository *RepoView::remoteRepo()
{
  if (mRemoteRepoCached)
    return mRemoteRepo;

  // Look up remote account repository.
  git::Remote remote = mRepo.defaultRemote();
  mRemoteRepo = remote ? Accounts::instance()->lookup(remote.url()) : nullptr;
  mRemoteRepoCached = true;

  if (mRemoteRepo) {
    auto err = connect(mRemoteRepo->account(), &Account::pullRequestError,
    [this](const QString &name, const QString &message) {
      LogEntry *parent = addLogEntry(tr("Pull Request"), tr("Create"));
      error(parent, tr("create pull request"), name, message);
    });

    connect(mRemoteRepo, &Repository::destroyed, [this, err] {
      disconnect(err);
      mRemoteRepo = nullptr;
      mRemoteRepoCached = false;
    });
  }

  return mRemoteRepo;
}

void RepoView::lfsInitialize()
{
  LogEntry *entry = addLogEntry(tr("Git LFS"), tr("Initialize"));
  if (!mRepo.lfsInitialize()) {
    error(entry, tr("initialize"));
    return;
  }

  entry->addEntry(LogEntry::File, tr("Git LFS initialized."));
}

void RepoView::lfsDeinitialize()
{
  LogEntry *entry = addLogEntry(tr("Git LFS"), tr("Deinitialize"));
  if (!mRepo.lfsDeinitialize()) {
    error(entry, tr("deinitialize"));
    return;
  }

  entry->addEntry(LogEntry::File, tr("Git LFS Deinitialized."));
}

bool RepoView::lfsSetLocked(const QStringList &paths, bool lock)
{
  QStringList errors;
  QString verb = lock ? tr("Lock") : tr("Unlock");

  foreach (const QString &path, paths) {
    if (!repo().lfsSetLocked(path, lock))
      errors.append(tr("Unable to %1 '%2' - %3").arg(
                      verb.toLower(), path, git::Repository::lastError()));
  }

  if (errors.isEmpty())
    return true;

  LogEntry *entry = addLogEntry(tr("Git LFS"), verb);
  foreach (const QString &error, errors)
    entry->addEntry(LogEntry::Error, error);

  return false;
}

Location RepoView::location() const
{
  git::Reference ref = mRefs->currentReference();
  if (!ref.isValid())
    return Location();

  QString name = ref.qualifiedName();
  RepoView::ViewMode mode = mDetails->viewMode();
  return Location(mode, name, mCommits->selectedRange(), mDetails->file());
}

void RepoView::setLocation(const Location &location)
{
  mDetails->setViewMode(location.mode(), false);
  mCommits->selectRange(location.id(), location.file());

  if (git::Reference ref = mRepo.lookupRef(location.ref())) {
    if (git::Reference current = mRefs->currentReference()) {
      if (ref.qualifiedName() != current.qualifiedName())
        mRefs->select(ref);
    }
  }
}

void RepoView::find()
{
  mDetails->find();
}

void RepoView::findNext()
{
  mDetails->findNext();
}

void RepoView::findPrevious()
{
  mDetails->findPrevious();
}

void RepoView::startIndexing()
{
  if (!mRepo.appConfig().value<bool>("index.enable", true))
    return;

  if (mIndexer.state() != QProcess::NotRunning) {
    mRestartIndexer = true;
    return;
  }

  QStringList args = {"--notify", "--background", mRepo.dir().path()};
  if (Index::isLoggingEnabled())
    args.prepend("--log");

  QDir dir(QCoreApplication::applicationDirPath());
  mIndexer.start(dir.filePath("indexer"), args);
}

void RepoView::cancelIndexing()
{
  if (mIndexer.state() == QProcess::NotRunning)
    return;

  mIndexer.terminate();
  mIndexer.waitForFinished(5000);

  if (mIndexer.state() == QProcess::NotRunning)
    return;

  mIndexer.kill();
  mIndexer.waitForFinished(5000);
}

bool RepoView::isLogVisible() const
{
  return mIsLogVisible;
}

void RepoView::setLogVisible(bool visible)
{
  if (visible == mIsLogVisible)
    return;

  mIsLogVisible = visible;

  // Update interface.
  toolBar()->updateView();
  MenuBar::instance(this)->updateView();

  // Animate log view sliding in or out.
  int pos = visible ? mLogView->sizeHint().height() : sizes().last();

  QTimeLine *timeline = new QTimeLine(250, this);
  timeline->setDirection(visible ? QTimeLine::Forward : QTimeLine::Backward);
  timeline->setEasingCurve(QEasingCurve::Linear);
  timeline->setUpdateInterval(20);

  connect(timeline, &QTimeLine::valueChanged, [this, pos](qreal value) {
    setSizes({1, static_cast<int>(pos * value)});
  });

  connect(timeline, &QTimeLine::finished, [timeline] {
    delete timeline;
  });

  timeline->start();
}

LogEntry *RepoView::addLogEntry(
  const QString &text,
  const QString &title,
  LogEntry *parent)
{
  LogEntry *root = parent ? parent : mLogRoot;
  return root->addEntry(text, title);
}

LogEntry *RepoView::error(
  LogEntry *parent,
  const QString &action,
  const QString &name,
  const QString &defaultError)
{
  QString detail = git::Repository::lastError(defaultError);
  QString text = name.isEmpty() ?
    tr("Unable to %1 - %2").arg(action, detail) :
    tr("Unable to %1 '%2' - %3").arg(action, name, detail);

  QStringList items = text.split("\\n", Qt::KeepEmptyParts);
  if (items.last() == "\n")
    items.removeLast();

  LogEntry *root = parent->addEntry(LogEntry::Error, items.takeFirst());
  foreach (const QString &item, items)
    root->addEntry(LogEntry::File, item);

  return root;
}

void RepoView::startFetchTimer()
{
  mFetchTimer.stop();

  // Read defaults from global settings.
  Settings *settings = Settings::instance();
  settings->beginGroup("global/autofetch");
  bool enable = settings->value("enable").toBool();
  int minutes = settings->value("minutes").toInt();
  settings->endGroup();

  git::Config config = mRepo.appConfig();
  if (!config.value<bool>("autofetch.enable", enable))
    return;

  bool prune = settings->value("global/autoprune/enable").toBool();
  fetch(git::Remote(), false, false, nullptr, nullptr,
    config.value<bool>("autoprune.enable", prune));

  mFetchTimer.start(config.value<int>("autofetch.minutes", minutes) * 60000);
}

void RepoView::fetchAll()
{
  QList<git::Remote> remotes = mRepo.remotes();
  if (remotes.isEmpty())
    return;

  if (remotes.size() == 1) {
    fetch(remotes.at(0));
    return;
  }

  // Queue up all remotes to fetch them serially.
  QString text = tr("%1 remotes").arg(remotes.size());
  LogEntry *entry = addLogEntry(text, tr("Fetch All"));
  foreach (const git::Remote &remote, remotes)
    fetch(remote, false, true, entry);
}

QFuture<git::Result> RepoView::fetch(
  const git::Remote &rmt,
  bool tags,
  bool interactive,
  LogEntry *parent,
  QStringList *submodules)
{
  bool prune = Settings::instance()->value("global/autoprune/enable").toBool();
  return fetch(rmt, tags, interactive, parent, submodules,
    mRepo.appConfig().value<bool>("autoprune.enable", prune));
}

QFuture<git::Result> RepoView::fetch(
  const git::Remote &rmt,
  bool tags,
  bool interactive,
  LogEntry *parent,
  QStringList *submodules,
  bool prune)
{
  if (mWatcher) {
    // Queue fetch.
    connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
    [this, rmt, tags, interactive, parent, submodules, prune] {
      fetch(rmt, tags, interactive, parent, submodules, prune);
    });

    return QFuture<git::Result>();
  }

  // Fetch if there's a valid remote, even if HEAD is detached.
  QString title = tr("Fetch");
  git::Remote remote = rmt.isValid() ? rmt : mRepo.defaultRemote();
  QString text = remote.isValid() ? remote.name() : tr("<i>no remote</i>");
  LogEntry *entry = interactive ? addLogEntry(text, title, parent) :
    new LogEntry(LogEntry::Entry, text, title); // Create unparented.

  if (!remote.isValid()) {
    QString err =
      tr("Unable to fetch. No upstream is configured for the current "
         "branch, and there isn't a remote called 'origin'.");
    entry->addEntry(LogEntry::Error, err);
    return QFuture<git::Result>();
  }

  mWatcher = new QFutureWatcher<git::Result>(this);
  connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
  [this, remote, entry, interactive, parent] {
    entry->setBusy(false);

    git::Result result = mWatcher->result();
    if (mCallbacks->isCanceled()) {
      entry->addEntry(LogEntry::Error, tr("Fetch canceled."));
    } else if (!result) {
      error(entry, tr("fetch from"), remote.name(), result.errorString());
    } else {
      mCallbacks->storeDeferredCredentials();
      if (entry->entries().isEmpty()) {
        entry->addEntry(tr("Everything up-to-date."));
      } else if (!interactive) {
        LogEntry *root = parent ? parent : mLogRoot;
        root->addEntries({entry});
      }
    }

    if (!entry->parent())
      delete entry;

    mWatcher->deleteLater();
    mWatcher = nullptr;
    mCallbacks = nullptr;
  });

  QString url = remote.url();
  mCallbacks = new RemoteCallbacks(
    RemoteCallbacks::Receive, entry, url, remote.name(), mWatcher, mRepo);
  connect(mCallbacks, &RemoteCallbacks::referenceUpdated,
          this, &RepoView::notifyReferenceUpdated);
  connect(mCallbacks, &RemoteCallbacks::credentialsCanceled, [this] {
    git::Config config = mRepo.appConfig();
    bool defaultValue = Settings::instance()->value("global/autofetch/enable").toBool();
    if (config.value<bool>("autofetch.enable", defaultValue)) {
      // Disable autofetch; don't prompt for credentials again.
      config.setValue("autofetch.enable", false);
      mFetchTimer.stop();
    }
  });

  entry->setBusy(true);
  mWatcher->setFuture(QtConcurrent::run([this, remote, tags, submodules, prune] {
    git::Result result = git::Remote(remote).fetch(mCallbacks, tags, prune);

    if (result && submodules) {
      // Scan for unmodified submodules on the fetch thread.
      foreach (const git::Submodule &submodule, mRepo.submodules()) {
        if (GIT_SUBMODULE_STATUS_IS_UNMODIFIED(submodule.status()))
          submodules->append(submodule.name());
      }
    }

    return result;
  }));

  return mWatcher->future();
}

void RepoView::pull(
  MergeFlags flags,
  const git::Remote &rmt,
  bool tags,
  bool prune)
{
  if (mWatcher) {
    // Queue pull.
    connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
    [this, flags, rmt, tags] {
      pull(flags, rmt, tags);
    });

    return;
  }

  // Use default remote for fetch.
  git::Branch head = mRepo.head();
  git::Remote remote = rmt.isValid() ? rmt : mRepo.defaultRemote();

  QString headName = head.isValid() ? head.name() : tr("<i>no branch</i>");
  QString name = remote.isValid() ? remote.name() : tr("<i>no remote</i>");
  QString text = tr("%1 from %2").arg(headName, name);
  LogEntry *entry = addLogEntry(text, tr("Pull"));

  // Read submodule setting.
  QStringList *submodules = nullptr;
  Settings *settings = Settings::instance();
  bool enable = settings->value("global/autoupdate/enable").toBool();
  if (mRepo.appConfig().value<bool>("autoupdate.enable", enable))
    submodules = new QStringList;

  QFutureWatcher<git::Result> *watcher = new QFutureWatcher<git::Result>(this);
  connect(watcher, &QFutureWatcher<git::Result>::finished, watcher,
  [this, flags, entry, watcher, submodules] {
    // Copy submodule names.
    QStringList names;
    if (submodules) {
      names = *submodules;
      delete submodules;
    }

    watcher->deleteLater();
    if (!watcher->result())
      return;

    // Create callback to update submodules.
    std::function<void()> callback;
    if (!names.isEmpty()) {
      callback = [this, entry, names] {
        QList<git::Submodule> modules;
        foreach (const QString &name, names)
          modules.append(mRepo.lookupSubmodule(name));
        updateSubmodules(modules, true, false, entry);
      };
    }

    // Merge the upstream of the HEAD branch.
    MergeFlags mf = flags;
    if (flags == Default) {
      // Read pull.rebase from config.
      git::Config config = mRepo.config();
      bool rebase = config.value<bool>("pull.rebase");

      // Read branch.<name>.rebase from config.
      if (git::Branch head = mRepo.head()) {
        QString key = QString("branch.%1.rebase").arg(head.name());
        rebase = config.value<bool>(key, rebase);
      }

      mf = rebase ? Rebase : Merge;
    }

    merge(mf, git::Reference(), git::AnnotatedCommit(), entry, callback);
  });

  watcher->setFuture(fetch(remote, tags, true, entry, submodules, prune));
  if (watcher->isCanceled()) {
    delete watcher;
    delete submodules;
  }
}

void RepoView::merge(
  MergeFlags flags,
  const git::Reference &ref,
  const git::AnnotatedCommit &commit,
  LogEntry *parent,
  const std::function<void()> &callback)
{
  git::Reference head = mRepo.head();

  git::AnnotatedCommit upstream;
  QString upstreamName = tr("<i>no upstream</i>");
  if (commit.isValid()) {
    upstream = commit;
    upstreamName = commit.commit().link();
  } else if (ref.isValid()) {
    upstream = ref.annotatedCommit();
    upstreamName = ref.name();
  } else if (head.isValid() && head.isBranch()) {
    git::Branch headBranch = head;
    upstream = headBranch.annotatedCommitFromFetchHead();
    git::Branch up = headBranch.upstream();
    if (up.isValid())
      upstreamName = up.name();
  }

  int analysis = upstream.isValid() ?
    upstream.analysis() : GIT_MERGE_ANALYSIS_NONE;
  bool ff = (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD);
  bool noff = (flags & NoFastForward);
  bool ffonly = (flags & FastForward);
  bool squashflag = (flags & Squash);

  // Write log entry.
  QString title = tr("Merge");
  QString textFmt = tr("%1 into %2");
  if ((ffonly || (!noff && ff)) && !squashflag) {
    title = tr("Fast-forward");
    textFmt = tr("%2 to %1");
  } else if (flags & Rebase) {
    title = tr("Rebase");
    textFmt = tr("%2 on %1");
  }

  QString headName = head.isValid() ? head.name() : tr("<i>no branch</i>");
  QString text = textFmt.arg(upstreamName, headName);
  LogEntry *entry = addLogEntry(text, title, parent);

  // Empty repository.
  if (!head.isValid()) {
    entry->addEntry(LogEntry::Error,
      tr("The repository is empty."));
    return;
  }

  // Validate inputs.
  if (!upstream.isValid()) {
    entry->addEntry(LogEntry::Error,
      tr("The current branch '%1' has no upstream branch.").arg(headName));
    return;
  }

  // Choose strategy.
  if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
    entry->addEntry(tr("Already up-to-date."));
    return;
  }

  if (!ff && ffonly && !squashflag) {
    entry->addEntry(LogEntry::Error, tr("Unable to fast-forward."));
    return;
  }

  if (ff && !noff && !squashflag) {
    fastForward(ref, upstream, entry, callback);
    return;
  }

  // FIXME: Handle unborn?
  if (!(analysis & GIT_MERGE_ANALYSIS_NORMAL))
    return;

  if (flags & Rebase) {
    rebase(upstream, entry, callback);
    return;
  }

  if (squashflag) {
    squash(upstream, entry);
    return;
  }

  merge(flags, upstream, entry, callback);
}

void RepoView::fastForward(
  const git::Reference &ref,
  const git::AnnotatedCommit &upstream,
  LogEntry *parent,
  const std::function<void()> &callback)
{
  git::Reference head = mRepo.head();
  Q_ASSERT(head.isValid());

  git::Commit commit = upstream.commit();
  CheckoutCallbacks callbacks(parent, GIT_CHECKOUT_NOTIFY_UPDATED);
  if (!mRepo.checkout(commit, &callbacks)) {
    LogEntry *err = error(parent, tr("fast-forward"), head.name());
    foreach (const QString &path, callbacks.conflicts())
      err->addEntry(LogEntry::File, path)->setStatus('!');

    QUrlQuery query;
    if (ref.isValid())
      query.addQueryItem("ref", ref.qualifiedName());

    QUrl url("action:fast-forward");
    url.setQuery(query);

    // Add stash hint if available.
    if (canStash()) {
      QString stash =
        tr("You may be able to reconcile your changes with the conflicting "
           "files by <a href='action:stash'>stashing</a> before you "
           "<a href='%1'>fast-forward</a>. Then "
           "<a href='action:unstash'>unstash</a> to restore your changes.");
      err->addEntry(LogEntry::Hint, stash.arg(url.toString()));
    }

    query.addQueryItem("no-ff", "true");
    url.setPath("merge");
    url.setQuery(query);

    // Add merge hint.
    QString merge =
      tr("If you want to create a new merge commit instead of fast-"
         "forwarding, you can <a href='%1'>merge without fast-forwarding "
         "</a> instead.");
    err->addEntry(LogEntry::Hint, merge.arg(url.toString()));

    return;
  }

  // Point head branch at the new commit.
  if (head.setTarget(commit, "pull: fast-forward").isValid() && callback)
    callback();
}

void RepoView::merge(
  MergeFlags flags,
  const git::AnnotatedCommit &upstream,
  LogEntry *parent,
  const std::function<void()> &callback)
{
  git::Reference head = mRepo.head();
  Q_ASSERT(head.isValid());

  // Try to merge.
  if (!mRepo.merge(upstream)) {
    LogEntry *err = error(parent, tr("merge"), head.name());

    // Add stash hint if the failure was because of uncommitted changes.
    QString msg = git::Repository::lastError();
    int kind = git::Repository::lastErrorKind();
    if (kind == GIT_ERROR_MERGE && msg.contains("overwritten by merge") && canStash()) {
      QString text =
        tr("You may be able to rebase by <a href='action:stash'>stashing</a> "
           "before trying to <a href='action:merge'>merge</a>. Then "
           "<a href='action:unstash'>unstash</a> to restore your changes.");
      err->addEntry(LogEntry::Hint, text);
    }

    return;
  }

  // Check for conflicts.
  if (checkForConflicts(parent, tr("merge")))
    return;

  if (flags & NoCommit) {
    refresh();
    selectHead();
    return;
  }

  // Read default message.
  QString msg = mRepo.message();
  if (Settings::instance()->prompt(Settings::PromptMerge)) {
    // Prompt to edit message.
    bool suspended = suspendLogTimer();
    CommitDialog *dialog = new CommitDialog(msg, Settings::PromptMerge, this);
    connect(dialog, &QDialog::rejected, this, [this, parent, suspended] {
      resumeLogTimer(suspended);
      mergeAbort(parent);
    });
    connect(dialog, &QDialog::accepted, this,
    [this, dialog, upstream, parent, suspended, callback] {
      resumeLogTimer(suspended);
      if (commit(dialog->message(), upstream, parent) && callback)
        callback();
    });

    dialog->open();
    return;
  }

  // Automatically commit with the default message.
  if (commit(msg, upstream, parent) && callback)
    callback();
}

void RepoView::mergeAbort(LogEntry *parent)
{
  // Make sure that the we're still merging.
  if (mRepo.state() == GIT_REPOSITORY_STATE_NONE)
    return;

  git::Reference head = mRepo.head();
  if (!head.isValid())
    return;

  git::Commit commit = head.target();
  if (!commit.isValid())
    return;

  bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();

  QSet<QString> paths;
  git::Diff index =
    mRepo.diffTreeToIndex(commit.tree(), git::Index(), ignoreWhitespace);
  for (int i = 0; i < index.count(); ++i)
    paths.insert(index.name(i));

  QStringList conflicts;
  git::Diff workdir =
    mRepo.diffIndexToWorkdir(git::Index(), nullptr, ignoreWhitespace);
  for (int i = 0; i < workdir.count(); ++i) {
    QString name = workdir.name(i);
    if (workdir.status(i) != GIT_DELTA_CONFLICTED && paths.contains(name))
      conflicts.append(name);
  }

  if (!conflicts.isEmpty()) {
    LogEntry *parent = addLogEntry(tr("merge"), tr("Abort"));
    QString err = tr("Some merged files have unstaged changes");
    LogEntry *entry = error(parent, tr("abort merge"), QString(), err);
    foreach (const QString &conflict, conflicts)
      entry->addEntry(LogEntry::File, conflict)->setStatus('M');
    return;
  }

  int state = mRepo.state();
  if (!commit.reset(GIT_RESET_HARD, paths.values()))
    return;

  QString text = tr("merge");
  switch (state) {
    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
      text = tr("revert");
      break;

    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
      text = tr("cherry-pick");
      break;

    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
      text = tr("rebase");
      break;
  }

  addLogEntry(text, tr("Abort"), parent);
  mDetails->setCommitMessage(QString());
  refresh();
}

void RepoView::rebase(
  const git::AnnotatedCommit &upstream,
  LogEntry *parent,
  const std::function<void()> &callback)
{
  git::Branch head = mRepo.head();
  Q_ASSERT(head.isValid());

  git::Rebase rebase = mRepo.rebase(upstream);
  if (!rebase.isValid()) {
    LogEntry *err = error(parent, tr("rebase"), head.name());

    // Add stash hint if the failure was because of uncommitted changes.
    QString msg = git::Repository::lastError();
    int kind = git::Repository::lastErrorKind();
    if (kind == GIT_ERROR_REBASE && msg.contains("changes exist") && canStash()) {
      QString text =
        tr("You may be able to rebase by <a href='action:stash'>stashing</a> "
           "before trying to <a href='action:rebase'>rebase</a>. Then "
           "<a href='action:unstash'>unstash</a> to restore your changes.");
      err->addEntry(LogEntry::Hint, text);
    }

    return;
  }

  // Loop over rebase operations.
  int i = 0;
  int count = rebase.count();
  while (rebase.hasNext()) {
    git::Commit before = rebase.next();
    if (!before.isValid()) {
      error(parent, tr("rebase"), head.name());
      rebase.abort();
      return;
    }

    QString beforeText = before.link();
    QString step = tr("%1/%2").arg(++i).arg(count);
    QString text = tr("%1 - %2").arg(step, beforeText);
    LogEntry *entry = parent->addEntry(text, tr("Apply"));

    git::Commit after = rebase.commit();
    if (!after.isValid()) {
      // Rebase conflicted.
      entry->addEntry(LogEntry::Error,
        tr("There was a merge conflict. The rebase has been aborted"));
      rebase.abort();
      return;
    }

    entry->setText((after == before) ?
      tr("%1 - %2 <i>already applied</i>").arg(step, beforeText) :
      tr("%1 - %2 as %3").arg(step, beforeText, msg(after)));

    // Yield to the main event loop.
    QCoreApplication::processEvents();
  }

  // Finalize.
  if (rebase.finish() && callback)
    callback();
}

void RepoView::squash(
  const git::AnnotatedCommit &upstream,
  LogEntry *parent)
{
  git::Branch head = mRepo.head();
  Q_ASSERT(head.isValid());

  // Try to merge.
  if (!mRepo.merge(upstream)) {
    LogEntry *err = error(parent, tr("squash"), head.name());

    // Add stash hint if the failure was because of uncommitted changes.
    QString msg = git::Repository::lastError();
    int kind = git::Repository::lastErrorKind();
    if (kind == GIT_ERROR_MERGE && msg.contains("overwritten by merge") && canStash()) {
      QString text =
        tr("You may be able to rebase by <a href='action:stash'>stashing</a> "
           "before trying to <a href='action:merge'>merge</a>. Then "
           "<a href='action:unstash'>unstash</a> to restore your changes.");
      err->addEntry(LogEntry::Hint, text);
    }

    return;
  }

  // Make squash effect.
  mRepo.cleanupState();

  // Check for conflicts.
  checkForConflicts(parent, tr("squash"));

  refresh();
  selectHead();
}

void RepoView::revert(const git::Commit &commit)
{
  if (!commit.isValid())
    return;

  QString link = commit.link();
  LogEntry *parent = addLogEntry(link, tr("Revert"));

  // FIXME: Report which files conflicted?
  if (!commit.revert()) {
    error(parent, tr("revert"), link);
    return;
  }

  // Check for conflicts.
  if (checkForConflicts(parent, tr("revert")))
    return;

  QString id = commit.id().toString();
  QString summary = commit.summary();
  QString msg = tr("Revert \"%1\"\n\nThis reverts commit %2.").arg(summary, id);
  if (Settings::instance()->prompt(Settings::PromptRevert)) {
    // Prompt to edit message.
    bool suspended = suspendLogTimer();
    CommitDialog *dialog =
      new CommitDialog(msg, Settings::PromptRevert, this);
    connect(dialog, &QDialog::rejected, this, [this, parent, suspended] {
      resumeLogTimer(suspended);
      mergeAbort(parent);
    });
    connect(dialog, &QDialog::accepted, this,
    [this, dialog, parent, suspended] {
      resumeLogTimer(suspended);
      this->commit(dialog->message(), git::AnnotatedCommit(), parent);
    });

    dialog->open();
    return;
  }

  // Automatically commit with the default message.
  this->commit(msg, git::AnnotatedCommit(), parent);
}

void RepoView::cherryPick(const git::Commit &commit)
{
  if (!commit.isValid())
    return;

  QString link = commit.link();
  git::Branch head = mRepo.head();
  QString name = head.isValid() ? head.name() : tr("<i>detached HEAD</i>");
  QString text = tr("%1 on %2").arg(link, name);
  LogEntry *parent = addLogEntry(text, tr("Cherry-pick"));

  // FIXME: Report which files conflicted?
  if (!mRepo.cherryPick(commit)) {
    error(parent, tr("cherry-pick"), link);
    return;
  }

  // Check for conflicts.
  if (checkForConflicts(parent, tr("cherry-pick")))
    return;

  QString msg = commit.message();
  if (Settings::instance()->prompt(Settings::PromptCherryPick)) {
    // Prompt to edit message.
    bool suspended = suspendLogTimer();
    CommitDialog *dialog =
      new CommitDialog(msg, Settings::PromptCherryPick, this);
    connect(dialog, &QDialog::rejected, this, [this, parent, suspended] {
      resumeLogTimer(suspended);
      mergeAbort(parent);
    });
    connect(dialog, &QDialog::accepted, this,
    [this, dialog, parent, suspended] {
      resumeLogTimer(suspended);
      this->commit(dialog->message(), git::AnnotatedCommit(), parent);
    });

    dialog->open();
    return;
  }

  // Automatically commit with the default message.
  this->commit(msg, git::AnnotatedCommit(), parent);
}

void RepoView::promptToApplyDiff()
{
  QString path = DiffFileDialog::getApplyFileName(this);
  if (!path.isEmpty())
    applyDiff(path);
}

void RepoView::promptToForcePush(
  const git::Remote &remote,
  const git::Reference &src)
{
  // FIXME: Check if force is really required?

  QString title = tr("Force Push?");
  QString text = tr("Are you sure you want to force push?");
  QMessageBox *dialog =
    new QMessageBox(QMessageBox::Warning, title, text, QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  dialog->setInformativeText(
    tr("The remote will lose any commits that are reachable only from "
       "the overwritten reference. Dropped commits may be unexpectedly "
       "reintroduced by clones that already contain those commits locally."));

  QPushButton *accept =
    dialog->addButton(tr("Force Push"), QMessageBox::AcceptRole);
  connect(accept, &QPushButton::clicked, [this, remote, src] {
    push(remote, src, QString(), false, true);
  });

  dialog->open();
}

void RepoView::push(
  const git::Remote &rmt,
  const git::Reference &src,
  const QString &dst,
  bool setUpstream,
  bool force,
  bool tags)
{
  if (mWatcher) {
    // Queue push.
    connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
    [this, rmt, src, dst, setUpstream, force, tags] {
      push(rmt, src, dst, setUpstream, force, tags);
    });

    return;
  }

  git::Reference ref = src.isValid() ? src : mRepo.head();
  QString refName = ref.isValid() ? ref.name() : tr("<i>no reference</i>");

  git::Remote remote = rmt.isValid() ? rmt : mRepo.defaultRemote();
  QString name = remote.isValid() ? remote.name() : tr("<i>no remote</i>");

  git::Branch branch = ref;
  git::Branch upstream = branch ? branch.upstream() : git::Branch();
  git::Remote upstreamRemote = upstream ? upstream.remote() : git::Remote();
  if (upstreamRemote && upstreamRemote.name() != remote.name())
    upstream = git::Branch();

  QString title = !force ? tr("Push") : tr("Push (Force)");
  QString text = tr("%1 to %2").arg(refName, name);
  LogEntry *entry = addLogEntry(text, title);

  if (!ref.isValid()) {
    QString text = tr("You are not currently on a branch.");
    LogEntry *err = entry->addEntry(LogEntry::Error, text);
    if (mRepo.isHeadUnborn()) {
      QString hint = tr("Create a commit to add the default '%1' branch.");
      err->addEntry(LogEntry::Hint, hint.arg(mRepo.unbornHeadName()));
    } else {
      QString hint =
        tr("You can <a href='action:checkout'>checkout</a> a branch "
           "then <a href='action:push'>push</a> again, or "
           "<a href='action:push-to'>push to an explicit branch</a>.");
      err->addEntry(LogEntry::Hint, hint);
    }

    return;
  }

  if (!remote.isValid()) {
    QString text = tr("The current branch '%1' has no default remote.");
    LogEntry *err = entry->addEntry(LogEntry::Error, text.arg(refName));
    QString hint1 =
      tr("You may want to <a href='action:add-remote?name=origin'>add a remote "
         "named 'origin'</a>. Then <a href='action:push?set-upstream=true'>"
         "push and set the current branch's upstream</a> to begin tracking a "
         "remote branch called 'origin/%1'.").arg(refName);
    QString hint2 =
      tr("You can also <a href='action:push-to'>push to an explicit URL</a> "
         "if you don't want to track a remote branch.");
    err->addEntry(LogEntry::Hint, hint1);
    err->addEntry(LogEntry::Hint, hint2);
    return;
  }

  QString unqualifiedName = !dst.isEmpty() ? dst : refName;
  QString remoteBranchName = QString("%1/%2").arg(name, unqualifiedName);
  if (!upstream.isValid() && !setUpstream && !src.isValid()) {
    LogEntry *err = entry->addEntry(LogEntry::Error,
      tr("The current branch '%1' has no upstream branch.").arg(refName));
    QString hint1 =
      tr("To begin tracking a remote branch called '%1', "
         "<a href='action:push?set-upstream=true'>push and "
         "set the current branch's upstream</a>.").arg(remoteBranchName);
    QString hint2 =
      tr("To push without setting up tracking information, "
         "<a href='action:push?ref=%1'>push '%2'</a> "
         "explicitly.").arg(ref.qualifiedName(), refName);
    err->addEntry(LogEntry::Hint, hint1);
    err->addEntry(LogEntry::Hint, hint2);
    return;
  }

  if (!tags && upstream.isValid() && ref.target() == upstream.target() &&
      (dst.isEmpty() || dst == ref.qualifiedName())) {
    entry->addEntry(tr("Everything up-to-date."));
    return;
  }

  mWatcher = new QFutureWatcher<git::Result>(this);
  connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
  [this, src, ref, setUpstream, remote, entry, remoteBranchName] {
    entry->setBusy(false);

    git::Result result = mWatcher->result();
    if (mCallbacks->isCanceled()) {
      entry->addEntry(LogEntry::Error, tr("Push canceled."));
    } else if (int err = result.error()) {
      QString name = remote.name();
      QString errString = result.errorString();
      LogEntry *errorEntry = error(entry, tr("push to"), name, errString);
      if (err == GIT_ENONFASTFORWARD) {
        if (ref.isTag()) {
          QString hint1 =
            tr("The tag update may cause the remote to lose commits.");
          QString hint2 =
            tr("If you want to risk the remote losing commits, you can "
            "<a href='action:push?ref=%1&to=%2&force=true'>force push</a>.");

          errorEntry->addEntry(LogEntry::Hint, hint1);
          errorEntry->addEntry(LogEntry::Warning, hint2.arg(
            ref.qualifiedName().toHtmlEscaped(), name.toHtmlEscaped()));

        } else {
          QString hint1 =
            tr("You may want to integrate remote commits first by "
            "<a href='action:pull'>pulling</a>. Then "
            "<a href='action:push'>push</a> again.");
          QString hint2 =
            tr("If you really want the remote to lose commits, you may "
            "be able to <a href='action:push?force=true'>force push</a>.");
          errorEntry->addEntry(LogEntry::Hint, hint1);
          errorEntry->addEntry(LogEntry::Warning, hint2);
        }
      }
    } else {
      mCallbacks->storeDeferredCredentials();
      if (entry->entries().isEmpty())
        entry->addEntry(tr("Everything up-to-date."));

      if (setUpstream) {
        // Reset upstream unconditionally.
        git::Branch upstream =
          mRepo.lookupBranch(remoteBranchName, GIT_BRANCH_REMOTE);
        if (upstream.isValid()) {
          git::Branch head = src.isValid() ? src : mRepo.head();
          head.setUpstream(upstream);
        }
      }
    }

    mWatcher->deleteLater();
    mWatcher = nullptr;
    mCallbacks = nullptr;
  });

  QString url = remote.url();
  mCallbacks = new RemoteCallbacks(
    RemoteCallbacks::Send, entry, url, remote.name(), mWatcher, mRepo);
  connect(mCallbacks, &RemoteCallbacks::referenceUpdated,
          this, &RepoView::notifyReferenceUpdated);

  entry->setBusy(true);
  mWatcher->setFuture(QtConcurrent::run(
    &git::Remote::pushRef, remote, mCallbacks, ref, dst, force, tags));
}

bool RepoView::commit(
  const QString &message,
  const git::AnnotatedCommit &upstream,
  LogEntry *parent,
  bool force)
{
  // Check for detached head.
  git::Reference head = mRepo.head();
  if (!force && head.isValid() && !head.isLocalBranch()) {
    QString title = tr("Commit?");
    QString text = tr("Are you sure you want to commit on a detached HEAD?");
    QMessageBox *dialog = new QMessageBox(
      QMessageBox::Warning, title, text, QMessageBox::Cancel, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    dialog->setInformativeText(
      tr("<p>You are in a detached HEAD state. You can still commit, but the "
         "new commit will not be reachable from any branch. If you want to "
         "commit to an existing branch, checkout the branch first.</p>"));

    QPushButton *accept =
      dialog->addButton(tr("Commit"), QMessageBox::AcceptRole);
    connect(accept, &QPushButton::clicked, [this, message, upstream, parent] {
      this->commit(message, upstream, parent, true);
    });

    dialog->open();
    return false;
  }

  QString text = tr("<i>no commit</i>");
  LogEntry *entry = addLogEntry(text, tr("Commit"), parent);

  bool fakeSignature = false;
  git::Commit commit = mRepo.commit(message, upstream, &fakeSignature);
  if (!commit.isValid()) {
    error(entry, tr("commit"));
    return false;
  }

  // Log commit message.
  entry->setText(msg(commit));
  if (fakeSignature) {
    QString text =
      tr("This commit was signed with a generated user name and email.");
    QString hint1 =
      tr("Consider setting the user name and email in "
         "<a href='action:config?global=true'>global settings</a>.");
    QString hint2 =
      tr("If you want to limit the name and email settings to this repository, "
         "<a href='action:config'>edit repository settings</a> instead.");
    QString hint3 =
      tr("After settings have been updated, <a href='action:amend'> amend "
         "this commit</a> to record the new user name and email.");
    LogEntry *error = entry->addEntry(LogEntry::Error, text);
    error->addEntry(LogEntry::Hint, hint1);
    error->addEntry(LogEntry::Hint, hint2);
    error->addEntry(LogEntry::Hint, hint3);
  }

  // Automatically push if enabled.
  bool enable = Settings::instance()->value("global/autopush/enable").toBool();
  if (mRepo.appConfig().value<bool>("autopush.enable", enable))
    push(); // FIXME: Check for upstream before pushing?

  return true;
}

void RepoView::amendCommit()
{
  // FIXME: Log errors.
  git::Branch head = mRepo.head();
  if (!head.isValid())
    return;

  git::Commit commit = head.target();
  if (!commit.isValid())
    return;

  QList<git::Commit> parents = commit.parents();
  switch (parents.size()) {
    case 0:
      // Return to the unborn HEAD state.
      // FIXME: Prompt to reset?
      head.remove(true);
      mDetails->setCommitMessage(commit.message());
      refresh();
      break;

    case 1:
      // Reset to parent commit.
      promptToReset(parents.first(), GIT_RESET_SOFT, commit);
      break;

    default:
      // FIXME: Return to merging state?
      break;
  }
}

void RepoView::promptToCheckout()
{
  git::Reference ref = reference();
  CheckoutDialog *dialog = new CheckoutDialog(mRepo, ref, this);
  connect(dialog, &QDialog::accepted, [this, dialog] {
    checkout(dialog->reference(), dialog->detach());
  });

  dialog->open();
}

void RepoView::checkout(const git::Commit &commit, const QStringList &paths)
{
  QString count = QString::number(paths.size());
  QString name = (paths.size() == 1) ? tr("file") : tr("files");
  QString text = tr("%1 - %2 %3").arg(commit.link(), count, name);
  LogEntry *entry = addLogEntry(text, tr("Checkout"));

  CheckoutCallbacks callbacks(entry, GIT_CHECKOUT_NOTIFY_ALL);
  int strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_DONT_UPDATE_INDEX;
  mRepo.checkout(commit, &callbacks, paths, strategy);
  mRefs->select(mRepo.head());
}

void RepoView::checkout(
  const git::Reference &ref,
  bool detach)
{
  Q_ASSERT(ref.isValid());

  if (!ref.isRemoteBranch()) {
    checkout(ref.target(), ref, detach);
    return;
  }

  // Prompt to create a new local branch instead
  // of checking out a remote tracking branch.
  QMessageBox *dialog = new QMessageBox(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setIcon(QMessageBox::Question);
  dialog->setStandardButtons(QMessageBox::Cancel);
  dialog->setWindowTitle(tr("Checkout Detached HEAD?"));

  QPushButton *checkoutButton =
    dialog->addButton(tr("Checkout Detached HEAD"), QMessageBox::DestructiveRole);
  connect(checkoutButton, &QPushButton::clicked, [this, ref, detach] {
    checkout(ref.target(), ref, detach);
  });

  QString name = ref.name();
  QString local = name.section('/', 1);
  if (mRepo.lookupBranch(local, GIT_BRANCH_LOCAL)) {
    dialog->setText(
      tr("Checking out remote branch '%1' will result in a detached HEAD "
         "state. Do you want to reset the existing local branch '%2' to "
         "this commit instead?").arg(name, local));

    QPushButton *resetButton =
      dialog->addButton(tr("Reset Local Branch"), QMessageBox::AcceptRole);
    connect(resetButton, &QPushButton::clicked, [this, ref, local] {
      git::Branch head = mRepo.head();
      if (head.isValid() && head.name() == local) {
        // The local branch is currently checked out and cannot be replaced.
        // We'll just reset it instead.
        if (reset(ref.target(), GIT_RESET_HARD))
          head.setUpstream(ref);
      } else {
        // Replace local branch.
        createBranch(local, ref.target(), ref, true, true);
      }
    });
  } else {
    dialog->setText(
      tr("Checking out remote branch '%1' will result in a detached HEAD "
         "state. Do you want to create a new local branch called '%2' to "
         "track it instead?").arg(name, local));
    dialog->setInformativeText(
      tr("Create a local branch to start tracking remote changes and make "
         "new commits. Check out the detached HEAD to temporarily put your "
         "working directory into the state of the remote branch."));

    QPushButton *createButton =
      dialog->addButton(tr("Create Local Branch"), QMessageBox::AcceptRole);
    connect(createButton, &QPushButton::clicked, [this, ref, local] {
      createBranch(local, ref.target(), ref, true);
    });
  }

  dialog->open();
}

void RepoView::checkout(
  const git::Commit &commit,
  const git::Reference &ref,
  bool detach)
{
  Q_ASSERT(detach || ref.isValid());

  QString name = tr("<i>no commit</i>");
  if (!detach && ref.isValid()) {
    name = ref.name();
  } else if (commit.isValid()) {
    name = commit.link();
  }

  LogEntry *entry = addLogEntry(name, tr("Checkout"));
  CheckoutCallbacks callbacks(entry, GIT_CHECKOUT_NOTIFY_DIRTY);
  if (!commit.isValid() || !mRepo.checkout(commit, &callbacks) ||
      (detach && !mRepo.setHeadDetached(commit)) ||
      (!detach && !mRepo.setHead(ref))) {
    LogEntry *err = error(entry, tr("checkout"), name);
    foreach (const QString &path, callbacks.conflicts())
      err->addEntry(LogEntry::File, path)->setStatus('!');

    if (ref.isValid()) {
      QUrlQuery query;
      query.addQueryItem("ref", ref.qualifiedName());
      if (detach)
        query.addQueryItem("detach", "true");

      // Add stash hint if available.
      if (canStash()) {
        QString text =
          tr("You may be able to reconcile your changes with the conflicting "
             "files by <a href='action:stash'>stashing</a> before you "
             "<a href='action:checkout?%1'>checkout '%2'</a>. Then "
             "<a href='action:unstash'>unstash</a> to restore your changes.");
        err->addEntry(LogEntry::Hint, text.arg(query.toString(), ref.name()));
      }
    }

    return;
  }

  mRefs->select(mRepo.head());
}

void RepoView::promptToCreateBranch(const git::Commit &commit)
{
  NewBranchDialog *dialog = new NewBranchDialog(mRepo, commit, this);
  connect(dialog, &QDialog::accepted, [this, dialog] {
    createBranch(
      dialog->name(), dialog->target(),
      dialog->upstream(), dialog->checkout());
  });

  dialog->open();
}

git::Branch RepoView::createBranch(
  const QString &name,
  const git::Commit &target,
  const git::Branch &upstream,
  bool checkout,
  bool force)
{
  LogEntry *entry = addLogEntry(name, tr("New Branch"));
  git::Branch branch = mRepo.createBranch(name, target, force);
  if (!branch.isValid()) {
    error(entry, tr("create new branch"), name);
    return git::Branch();
  }

  // Start tracking.
  branch.setUpstream(upstream);

  // Checkout.
  if (checkout)
    this->checkout(branch);

  return branch;
}

void RepoView::promptToStash()
{
  if (!canStash())
    return;

  // Prompt to edit stash commit message.
  if (!Settings::instance()->prompt(Settings::PromptStash)) {
    stash();
    return;
  }

  // Reproduce default commit message.
  git::Reference head = mRepo.head();
  git::Commit commit = head.target();
  QString id = commit.shortId();
  QString ref = head.isBranch() ? head.name() : tr("(no branch)");
  QString msg = tr("WIP on %1: %2 %3").arg(ref, id, commit.summary());
  CommitDialog *dialog = new CommitDialog(msg, Settings::PromptStash, this);
  connect(dialog, &QDialog::accepted, this, [this, msg, dialog] {
    QString userMsg = dialog->message();
    stash(msg != userMsg ? userMsg : QString());
  });

  dialog->open();
}

void RepoView::stash(const QString &message)
{
  QString text = tr("<i>working directory</i>");
  LogEntry *entry = addLogEntry(text, tr("Stash"));

  git::Commit commit = mRepo.stash(message);
  if (!commit.isValid()) {
    error(entry, tr("stash"), text);
    return;
  }

  entry->setText(msg(commit));
  refresh();
}

void RepoView::applyStash(int index)
{
  QList<git::Commit> stashes = mRepo.stashes();
  Q_ASSERT(index >= 0 && index < stashes.size());

  git::Commit commit = stashes.at(index);
  LogEntry *entry = addLogEntry(msg(commit), tr("Apply Stash"));
  if (!mRepo.applyStash(index)) {
    error(entry, tr("apply stash"), commit.link());
    return;
  }

  refresh();
}

void RepoView::dropStash(int index)
{
  QList<git::Commit> stashes = mRepo.stashes();
  Q_ASSERT(index >= 0 && index < stashes.size());

  git::Commit commit = stashes.at(index);
  LogEntry *entry = addLogEntry(msg(commit), tr("Drop Stash"));
  if (!mRepo.dropStash(index))
    error(entry, tr("drop stash"), commit.link());
}

void RepoView::popStash(int index)
{
  QList<git::Commit> stashes = mRepo.stashes();
  Q_ASSERT(index >= 0 && index < stashes.size());

  git::Commit commit = stashes.at(index);
  LogEntry *entry = addLogEntry(msg(commit), tr("Pop Stash"));
  if (!mRepo.popStash(index)) {
    error(entry, tr("pop stash"), commit.link());
    return;
  }

  refresh();
}

bool RepoView::canStash() const
{
  return !mRepo.isHeadUnborn() && isWorkingDirectoryDirty();
}

void RepoView::promptToAddTag(const git::Commit &commit)
{
  TagDialog *dialog = new TagDialog(mRepo, commit.shortId(),
    mRepo.defaultRemote(), this);

  connect(dialog, &TagDialog::accepted, this, [this, commit, dialog] {
    bool force = dialog->force();
    QString name = dialog->name();
    QString msg = dialog->message();
    git::TagRef tag = mRepo.createTag(commit, name, msg, force);

    git::Remote remote = dialog->remote();

    QString link = commit.link();
    QString text = tag.isValid() ? tr("%1 as %2").arg(link, tag.name()) : link;
    LogEntry *entry = addLogEntry(text, tr("Tag"));
    if (!tag.isValid())
      error(entry, tr("tag"), link);
    else if (remote.isValid())
      push(remote, tag);
  });

  dialog->open();
}

void RepoView::promptToReset(
  const git::Commit &commit,
  git_reset_t type,
  const git::Commit &commitToAmend)
{
  git::Branch head = mRepo.head();
  if (!head.isValid()) {
    QString title = commitToAmend ? tr("Amend") : tr("Reset");
    LogEntry *entry = addLogEntry(tr("<i>no branch</i>"), title);
    entry->addEntry(LogEntry::Error, tr("You are not currently on a branch."));
    return;
  }

  QString id = commitToAmend ? commitToAmend.shortId() : commit.shortId();
  QString title = commitToAmend ? tr("Amend?") : tr("Reset?");
  QString text = commitToAmend ?
    tr("Are you sure you want to amend '%1'?").arg(id) :
    tr("Are you sure you want to reset '%1' to '%2'?").arg(head.name(), id);
  QMessageBox *dialog = new QMessageBox(
    QMessageBox::Warning, title, text, QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  QString info;
  if (head.target() != commit)
    info = tr("<p>Some commits may become unreachable "
              "from the current branch.</p>");

  // Add warnings about destructive changes and resetting past upstream.
  git::Branch upstream = head.upstream();
  if (type == GIT_RESET_HARD && isWorkingDirectoryDirty()) {
    info +=
      tr("<p>Resetting will cause you to lose uncommitted changes. "
         "Untracked and ignored files will not be affected.</p>");
  } else if (upstream.isValid() && !head.difference(upstream)) {
    info +=
      tr("<p>Your branch appears to be up-to-date with its upstream branch. "
         "Resetting may cause your branch history to diverge from the "
         "remote branch history.</p>");
  }

  dialog->setInformativeText(info);

  QString buttonText = commitToAmend ? tr("Amend") : tr("Reset");
  QPushButton *accept = dialog->addButton(buttonText, QMessageBox::AcceptRole);
  connect(accept, &QPushButton::clicked, [this, commit, type, commitToAmend] {
    reset(commit, type, commitToAmend);

    // Pre-populate the commit message editor.
    if (commitToAmend)
      mDetails->setCommitMessage(commitToAmend.message());
  });

  dialog->open();
}

bool RepoView::reset(
  const git::Commit &commit,
  git_reset_t type,
  const git::Commit &commitToAmend)
{
  git::Reference head = mRepo.head();
  Q_ASSERT(head.isValid());

  QString title = commitToAmend ? tr("Amend") : tr("Reset");
  QString text = tr("%1 to %2").arg(head.name(), commit.link());
  LogEntry *entry = addLogEntry(text, title);

  if (!commit.reset(type)) {
    error(entry, commitToAmend ? tr("amend") : tr("reset"), head.name());
    return false;
  }

  return true;
}

void RepoView::updateSubmodules(
  const QList<git::Submodule> &submodules,
  bool recursive,
  bool init,
  LogEntry *parent)
{
  if (mWatcher) {
    // Queue update.
    connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
    [this, submodules, recursive, init, parent] {
      updateSubmodules(submodules, recursive, init, parent);
    });

    return;
  }

  QList<git::Submodule> modules =
    !submodules.isEmpty() ? submodules : mRepo.submodules();
  if (modules.isEmpty())
    return;

  // Start updating asynchronously.
  QList<SubmoduleInfo> infos = submoduleInfoList(mRepo, modules, init, parent);
  updateSubmodulesAsync(infos, recursive, init);
}

QList<RepoView::SubmoduleInfo> RepoView::submoduleInfoList(
  const git::Repository &repo,
  const QList<git::Submodule> &submodules,
  bool init,
  LogEntry *parent)
{
  // Gather list of submodules.
  QList<git::Submodule> modules;
  foreach (const git::Submodule &submodule, submodules) {
    // FIXME: Add hint to init the submodule?
    if (!init && !submodule.isInitialized())
      continue;

    if (submodule.workdirId() == submodule.headId())
      continue;

    modules.append(submodule);
  }

  QString text =
    tr("%1 of %2 submodules").arg(modules.size()).arg(submodules.size());
  LogEntry *entry = addLogEntry(text, tr("Update"), parent);

  if (modules.isEmpty())
    entry->addEntry(tr("Already up-to-date."));

  QList<SubmoduleInfo> list;
  foreach (const git::Submodule &module, modules)
    list.append({module, repo, entry});
  return list;
}

void RepoView::updateSubmodulesAsync(
  const QList<SubmoduleInfo> &submodules,
  bool recursive,
  bool init)
{
  if (submodules.isEmpty()) {
    refresh();
    return;
  }

  // Remove first submodule from the list.
  QList<SubmoduleInfo> tail = submodules;
  SubmoduleInfo info = tail.takeFirst();
  git::Submodule submodule = info.submodule;
  LogEntry *entry = info.entry->addEntry(submodule.name(), tr("Update"));

  mWatcher = new QFutureWatcher<git::Result>(this);
  connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
  [this, init, recursive, tail, info, entry] {
    entry->setBusy(false);

    git::Result result = mWatcher->result();
    if (mCallbacks->isCanceled()) {
      entry->addEntry(LogEntry::Error, tr("Fetch canceled."));
    } else if (!result) {
      QString name = info.submodule.name();
      error(entry, tr("update submodule"), name, result.errorString());
    } else {
      mCallbacks->storeDeferredCredentials();
    }

    mWatcher->deleteLater();
    mWatcher = nullptr;
    mCallbacks = nullptr;

    // Build list of submodules recursively.
    QList<SubmoduleInfo> prefix;
    if (recursive) {
      if (git::Repository repo = info.submodule.open()) {
        QList<git::Submodule> submodules = repo.submodules();
        if (!submodules.isEmpty())
          prefix = submoduleInfoList(repo, submodules, init, entry);
      }
    }

    // Restart with smaller list.
    updateSubmodulesAsync(prefix + tail, recursive, init);
  });

  QString url = submodule.url();
  git::Repository repo = submodule.open();
  mCallbacks = new RemoteCallbacks(
    RemoteCallbacks::Receive, entry, url, QString(), mWatcher, repo);

  entry->setBusy(true);
  mWatcher->setFuture(QtConcurrent::run(
    &git::Submodule::update, submodule, mCallbacks, init));
}

bool RepoView::openSubmodule(const git::Submodule &submodule)
{
  if (!submodule.isValid())
    return false;

  git::Repository repo = submodule.open();
  if (!repo.isValid()) {
    // Warn about trying to open a submodule that hasn't been inited.
    QString title = tr("Invalid Submodule Repository");
    QString text =
      tr("The submodule '%1' doesn't have a valid repository. You may need "
         "to init and/or update the submodule to check out a repository.");
    QMessageBox::warning(nullptr, title, text.arg(submodule.name()));
    return false;
  }

  if (Settings::instance()->value("window/tabs/submodule").toBool())
    return static_cast<MainWindow *>(window())->addTab(repo);

  return MainWindow::open(repo);
}

ConfigDialog *RepoView::configureSettings(ConfigDialog::Index index)
{
  ConfigDialog *dialog = new ConfigDialog(this, index);
  dialog->open();
  return dialog;
}

void RepoView::ignore(const QString &name)
{
  QFile file(mRepo.workdir().filePath(".gitignore"));
  if (!file.open(QFile::Append | QFile::Text))
    return;

  QTextStream(&file) << name << Qt::endl;
  file.close();

  refresh();
}

EditorWindow *RepoView::newEditor()
{
  EditorWindow *editor = new EditorWindow(repo(), window());
  editor->show();
  return editor;
}

bool RepoView::edit(const QString &path, int line)
{
  return openSubmodule(mRepo.lookupSubmodule(path)) || openEditor(path, line);
}

EditorWindow *RepoView::openEditor(
  const QString &path,
  int line,
  const git::Blob &blob,
  const git::Commit &commit)
{
  EditorWindow *window = EditorWindow::open(path, blob, commit, mRepo);
  if (!window)
    return nullptr;

  // Scroll line into view.
  BlameEditor *widget = window->widget();
  if (line >= 0) {
    TextEditor *editor = widget->editor();
    editor->ensureVisibleEnforcePolicy(line - 1);
    editor->gotoLine(line - 1);
  }

  connect(widget, &BlameEditor::linkActivated, this, &RepoView::visitLink);
  connect(widget, &BlameEditor::saved, [this] {
    // Notify window that the head branch is changed.
    emit mRepo.notifier()->referenceUpdated(mRepo.head());
  });

  // Track this window.
  mTrackedWindows.append(window);
  connect(window, &QObject::destroyed, [this, window] {
    mTrackedWindows.removeAll(window);
  });

  return window;
}

void RepoView::refresh()
{
  // Fake head update.
  emit mRepo.notifier()->referenceUpdated(mRepo.head());
}

void RepoView::setPathspec(const QString &path)
{
  mPathspec->setPathspec(path);
}

git::Commit RepoView::nextRevision(const QString &path) const
{
  QList<git::Commit> commits = mCommits->selectedCommits();
  if (commits.isEmpty())
    return git::Commit();

  git::Reference ref = mRefs->currentReference();
  git::RevWalk walker =
    ref.walker(GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME | GIT_SORT_REVERSE);
  walker.hide(commits.first());
  return walker.next(path);
}

git::Commit RepoView::previousRevision(const QString &path) const
{
  //Special case: Working tree to commit
  QList<git::Commit> commits = mCommits->selectedCommits();
  if (commits.isEmpty()) {
    git::Reference ref = mRefs->currentReference();
    if (!ref.isValid())
      return git::Commit();

    git::RevWalk walker = ref.walker(GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
    return walker.next(path);
  }

  git::Commit commit = commits.last();
  git::RevWalk walker = commit.walker(GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
  walker.next();
  return walker.next(path);
}

void RepoView::selectCommit(const git::Commit &commit, const QString &file)
{
  mCommits->selectRange(commit.id().toString(), file, true);
}

RepoView *RepoView::parentView(const QWidget *widget)
{
  QWidget *parent = widget->parentWidget();
  if (!parent)
    return nullptr;

  if (RepoView *view = qobject_cast<RepoView *>(parent))
    return view;

  return parentView(parent);
}

void RepoView::showEvent(QShowEvent *event)
{
  QSplitter::showEvent(event);

  if (mShown)
    return;

  // Start background tasks after showing for the first time.
  mShown = true;
  startIndexing();
  startFetchTimer();
}

void RepoView::closeEvent(QCloseEvent *event)
{
  // Try to close tracked windows.
  foreach (QWidget *window, mTrackedWindows) {
    if (!window->close()) {
      event->ignore();
      return;
    }
  }

  cancelBackgroundTasks();
  QSplitter::closeEvent(event);
}

ToolBar *RepoView::toolBar() const
{
  return static_cast<MainWindow *>(window())->toolBar();
}

CommitList *RepoView::commitList() const
{
  return mCommits;
}

void RepoView::notifyReferenceUpdated(const QString &name)
{
  emit mRepo.notifier()->referenceUpdated(mRepo.lookupRef(name));
}

void RepoView::startLogTimer()
{
  // Don't start the timer if the log is already visible and the
  // timer isn't running. The log was opened manually by the user.
  if (isLogVisible() && !mLogTimer.isActive())
    return;

  setLogVisible(true);

  // Check the hide setting.
  if (!Settings::instance()->value("window/log/hide").toBool())
    return;

  resumeLogTimer();
}

bool RepoView::suspendLogTimer()
{
  bool active = mLogTimer.isActive();
  mLogTimer.stop();
  return active;
}

void RepoView::resumeLogTimer(bool suspended)
{
  if (suspended)
    mLogTimer.start(2000);
}

bool RepoView::checkForConflicts(LogEntry *parent, const QString &action)
{
  // Check for conflicts.
  if (!mRepo.index().hasConflicts())
    return false;

  QString error = tr("There was a merge conflict.");
  LogEntry *entry = parent->addEntry(LogEntry::Error, error);

  QString help =
    tr("Resolve conflicts, then commit to conclude "
       "the %1. See <a href='expand'>details</a>.");
  QString conflicts =
    tr("Resolve conflicts in each conflicted (!) file in "
       "one of the following ways:");
  QString hint1 =
    tr("1. Click the 'Ours' or 'Theirs' button to choose the "
       "correct change. Then click the 'Save' button to apply.");
  QString hint2 =
    tr("2. Edit the file in the editor to make a different "
       "change. Remember to remove conflict markers.");
  QString hint3 =
    tr("3. Use an external merge tool. Right-click on the "
       "files in the list and choose 'External Merge'.");
  QString mark =
    tr("After all conflicts in the file are resolved, "
       "click the check box to mark it as resolved.");
  QString commit =
    tr("After all conflicted files are staged, "
       "commit to conclude the %1.");
  LogEntry *details = entry->addEntry(LogEntry::Hint, help.arg(action));
  LogEntry *resolve = details->addEntry(LogEntry::Entry, conflicts);
  resolve->addEntry(LogEntry::Entry, hint1);
  resolve->addEntry(LogEntry::Entry, hint2);
  resolve->addEntry(LogEntry::Entry, hint3);
  details->addEntry(LogEntry::Entry, mark);
  details->addEntry(LogEntry::Entry, commit.arg(action));
  mLogView->setEntryExpanded(details, false);

  if (action != tr("squash")) {
    QString abort =
      tr("You can <a href='action:abort'>abort</a> the %1 "
         "to return the repository to its previous state.");
    entry->addEntry(LogEntry::Hint, abort.arg(action));
  }

  refresh();
  return true;
}

void RepoView::applyDiff(const QString &path)
{
  LogEntry *entry = addLogEntry(path, tr("Apply Diff"));
  auto error = [this, entry, &path](const QString &message) {
    this->error(entry, tr("apply diff"), QFileInfo(path).fileName(), message);
  };

  QFile file(path);
  if (!file.open(QFile::ReadOnly)) {
    error(file.errorString());
    return;
  }

  QByteArray buffer = file.readAll();
  git::Diff diff = git::Diff::fromBuffer(buffer);
  if (!diff.isValid()) {
    error(tr("The diff file is invalid"));
    return;
  }

  if (!mRepo.applyDiff(diff)) {
    error(git::Repository::lastError());
    return;
  }
}

#include "RepoView.moc"

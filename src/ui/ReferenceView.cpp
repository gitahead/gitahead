//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ReferenceView.h"
#include "RepoView.h"
#include "TabBar.h"
#include "dialogs/DeleteBranchDialog.h"
#include "dialogs/DeleteTagDialog.h"
#include "dialogs/MergeDialog.h"
#include "git/Branch.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include "git/Tag.h"
#include "git/TagRef.h"
#include "log/LogEntry.h"
#include <QAbstractItemModel>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

const int kHeight = 200;
const QString kNowrapFmt = "<span style='white-space: nowrap'>%1</span>";

bool refComparator(const git::Reference &lhs, const git::Reference &rhs)
{
  git::Commit lhsCommit = lhs.target();
  git::Commit rhsCommit = rhs.target();
  return (lhsCommit.isValid() && rhsCommit.isValid() &&
          lhsCommit.committer().date() > rhsCommit.committer().date());
}

class ReferenceModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  struct ReferenceList
  {
    QString name;
    QList<git::Reference> refs;
  };

  ReferenceModel(
    const git::Repository &repo,
    ReferenceView::Kinds kinds,
    QObject *parent = nullptr)
    : QAbstractItemModel(parent), mRepo(repo), mKinds(kinds)
  {
    git::RepositoryNotifier *notifier = repo.notifier();
    connect(notifier, &git::RepositoryNotifier::referenceAdded,
            this, &ReferenceModel::update);
    connect(notifier, &git::RepositoryNotifier::referenceRemoved,
            this, &ReferenceModel::update);
    connect(notifier, &git::RepositoryNotifier::referenceUpdated,
            this, &ReferenceModel::update);
  }

  void update()
  {
    beginResetModel();

    mRefs.clear();

    // Add detached head.
    git::Reference detachedHead;
    if (mKinds & ReferenceView::DetachedHead) {
      git::Reference head = mRepo.head();
      if (head.isValid() && !head.isBranch())
        detachedHead = head;
    }

    // Add local branches.
    if (mKinds & ReferenceView::LocalBranches) {
      QList<git::Reference> branches;
      foreach (const git::Branch &branch, mRepo.branches(GIT_BRANCH_LOCAL)) {
        if (!(mKinds & ReferenceView::ExcludeHead) || !branch.isHead())
          branches.append(branch);
      }

      std::sort(branches.begin(), branches.end(), refComparator);

      // Add top references.
      if (detachedHead.isValid())
        branches.prepend(detachedHead);
      if (mKinds & ReferenceView::InvalidRef)
        branches.prepend(git::Reference());

      // Add bottom references.
      if (mKinds & ReferenceView::Stash) {
        if (git::Reference stash = mRepo.stashRef())
          branches.append(stash);
      }

      mRefs.append({tr("Branches"), branches});
    }

    // Add remote branches.
    if (mKinds & ReferenceView::RemoteBranches) {
      QList<git::Reference> remotes;
      foreach (const git::Branch &branch, mRepo.branches(GIT_BRANCH_REMOTE)) {
        // Filter remote HEAD branches.
        if (!branch.name().endsWith("HEAD"))
          remotes.append(branch);
      }

      std::sort(remotes.begin(), remotes.end(), refComparator);
      if (mKinds & ReferenceView::InvalidRef)
        remotes.prepend(git::Reference());
      mRefs.append({tr("Remotes"), remotes});
    }

    // Add tags.
    if (mKinds & ReferenceView::Tags) {
      QList<git::Reference> tags;
      foreach (const git::TagRef &tag, mRepo.tags())
        tags.append(tag);

      std::sort(tags.begin(), tags.end(), refComparator);
      mRefs.append({tr("Tags"), tags});
    }

    endResetModel();
  }

  QModelIndex index(
    int row,
    int column,
    const QModelIndex &parent = QModelIndex()) const override
  {
    if (!hasIndex(row, column, parent))
      return QModelIndex();

    bool id = (!parent.isValid() || parent.internalId());
    return createIndex(row, column, !id ? parent.row() + 1 : 0);
  }

  QModelIndex parent(const QModelIndex &index) const override
  {
    if (!index.isValid())
      return QModelIndex();

    quintptr id = index.internalId();
    return !id ? QModelIndex() : createIndex(id - 1, 0);
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override
  {
    if (!parent.isValid())
      return mRefs.size();

    if (parent.internalId())
      return 0;

    return mRefs.at(parent.row()).refs.size();
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override
  {
    return 1;
  }

  QVariant data(
    const QModelIndex &index,
    int role = Qt::DisplayRole) const override
  {
    // kinds
    int row = index.row();
    quintptr id = index.internalId();
    if (!id)
      return (role == Qt::DisplayRole) ? mRefs.at(row).name : QVariant();

    // refs
    git::Reference ref = mRefs.at(id - 1).refs.at(row);
    switch (role) {
      case Qt::DisplayRole:
        return ref.isValid() ? ref.name() : QString();

      case Qt::ToolTipRole: {
        if (!ref.isValid() || !ref.isTag())
          return QVariant();

        git::Tag tag = git::TagRef(ref).tag();
        if (!tag.isValid())
          return QVariant();

        QStringList lines;
        if (git::Signature signature = tag.tagger()) {
          QString name = QString("<b>%1</b>").arg(signature.name());
          QString email = QString("&lt;%1&gt;").arg(signature.email());
          lines.append(kNowrapFmt.arg(QString("%1 %2").arg(name, email)));

          QDateTime date = signature.date();
          QString dateStr = QLocale().toString(date, QLocale::LongFormat);
          lines.append(kNowrapFmt.arg(dateStr));
        }

        QString msg = tag.message();
        if (!msg.isEmpty())
          lines.append(QString("<p>%1</p>").arg(msg));

        return lines.join('\n');
      }

      case Qt::FontRole: {
        QFont font = static_cast<QWidget *>(QObject::parent())->font();
        font.setBold(ref.isValid() && ref.isHead());
        return font;
      }

      case Qt::UserRole:
        return QVariant::fromValue(ref);

      default:
        return QVariant();
    }
  }

  QVariant headerData(
    int section,
    Qt::Orientation orientation,
    int role = Qt::DisplayRole) const override
  {
    return QVariant();
  }

private:
  git::Repository mRepo;
  ReferenceView::Kinds mKinds;
  QList<ReferenceList> mRefs;
};

class FilterProxyModel : public QSortFilterProxyModel
{
public:
  FilterProxyModel(QObject *parent = nullptr)
    : QSortFilterProxyModel(parent)
  {}

  void setFilter(const QString &filter)
  {
    QString pattern = QRegularExpression::escape(filter);
    setFilterRegularExpression(
      QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption));
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override
  {
    if (!parent.isValid())
      return true;

    return QSortFilterProxyModel::filterAcceptsRow(row, parent);
  }
};

class Delegate : public QStyledItemDelegate
{
public:
  Delegate(QObject *parent = nullptr)
    : QStyledItemDelegate(parent)
  {}

protected:
  void initStyleOption(
    QStyleOptionViewItem *option,
    const QModelIndex &index) const
  {
    QStyledItemDelegate::initStyleOption(option, index);
    option->state |= QStyle::State_Active;
  }
};

class Header : public QHeaderView
{
  Q_OBJECT

public:
  Header(ReferenceView *view = nullptr)
    : QHeaderView(Qt::Horizontal, view)
  {
    setStretchLastSection(true);

    mTabs = new TabBar(this);
    connect(mTabs, &TabBar::currentChanged, [this, view](int tab) {
      QAbstractItemModel *model = view->model();
      QModelIndex index = model->index(tab, 0);
      view->setRootIndex(index);

      QString name = index.data().toString();
      mField->setPlaceholderText(tr("Filter %1").arg(name));

      QModelIndex child = model->index(0, 0, index);
      view->setRootIsDecorated(model->rowCount(child) > 0);

      // Select the first index.
      QModelIndex parent = view->currentIndex().parent();
      if (parent != view->rootIndex() && view->isPopup())
        view->setCurrentIndex(child);
    });

    // Forward up/down key presses from field to view.
    mField = new QLineEdit(this);
    mField->installEventFilter(view);
    mField->setClearButtonEnabled(true);

    QHBoxLayout *fieldLayout = new QHBoxLayout;
    fieldLayout->setContentsMargins(2,2,2,2);
    fieldLayout->addWidget(mField);

    mWidget = new QWidget(this);
    mWidget->setAutoFillBackground(true);

    QVBoxLayout *layout = new QVBoxLayout(mWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    layout->addWidget(mTabs);
    layout->addLayout(fieldLayout);
  }

  QLineEdit *field() const { return mField; }
  QTabBar *tabs() const { return mTabs; }

  QSize sizeHint() const override
  {
    return mWidget->sizeHint();
  }

protected:
  void hideEvent(QHideEvent *event) override
  {
    mField->clear();
  }

  void resizeEvent(QResizeEvent *event) override
  {
    QHeaderView::resizeEvent(event);
    mWidget->resize(size());
  }

private:
  QLineEdit *mField;
  QTabBar *mTabs;
  QWidget *mWidget;
};

} // anon. namespace

ReferenceView::ReferenceView(
  const git::Repository &repo,
  Kinds kinds,
  bool popup,
  QWidget *parent)
  : QTreeView(parent), mPopup(popup)
{
  // Constrain height.
  setMinimumHeight(kHeight);
  setMaximumHeight(kHeight);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  if (popup)
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // Set delegate.
  setItemDelegate(new Delegate(this));

  // Set header.
  Header *header = new Header(this);
  setHeader(header);

  QTabBar *tabs = header->tabs();
  QLineEdit *field = header->field();

  // Set model.
  FilterProxyModel *model = new FilterProxyModel(this);
  ReferenceModel *source = new ReferenceModel(repo, kinds, this);
  model->setSourceModel(source);
  setModel(model);

  connect(field, &QLineEdit::textChanged, [this, model](const QString &text) {
    model->setFilter(text);
    QModelIndex root = rootIndex();
    if (!currentIndex().isValid() && model->rowCount(root) > 0)
      setCurrentIndex(model->index(0, 0, root));
  });

  connect(model, &QAbstractItemModel::modelReset, [this, tabs] {
    // Reset tabs.
    while (tabs->count() > 0)
      tabs->removeTab(0);

    QAbstractItemModel *model = this->model();
    for (int i = 0; i < model->rowCount(); ++i)
      tabs->addTab(model->index(i, 0).data().toString());
  });

  if (!popup) {
    // Checkout on double-click.
    connect(this, &ReferenceView::doubleClicked,
    [this, repo](const QModelIndex &index) {
      if (repo.isBare())
        return;

      git::Reference ref = index.data(Qt::UserRole).value<git::Reference>();
      if (ref.isValid() && !ref.isHead())
        RepoView::parentView(this)->checkout(ref);
    });
  }

  // Keep focus on the field.
  setFocusProxy(field);

  // Update model last.
  source->update();
}

void ReferenceView::resetTabIndex()
{
  QModelIndex root = currentIndex();
  while (root.parent().isValid())
    root = root.parent();

  static_cast<Header *>(header())->tabs()->setCurrentIndex(root.row());
}

git::Reference ReferenceView::currentReference() const
{
  return currentIndex().data(Qt::UserRole).value<git::Reference>();
}

bool ReferenceView::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() != QEvent::KeyPress)
    return false;

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
  switch (keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
      keyPressEvent(keyEvent);
      return true;
  }

  return false;
}

QString ReferenceView::kindString(const git::Reference &ref)
{
  if (ref.isLocalBranch())
    return tr("Branch");

  if (ref.isRemoteBranch())
    return tr("Remote");

  if (ref.isTag())
    return tr("Tag");

  return QString();
}

void ReferenceView::showEvent(QShowEvent *event)
{
  resetTabIndex();
  setFocus();

  QTreeView::showEvent(event);
}

void ReferenceView::contextMenuEvent(QContextMenuEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  git::Reference ref = index.data(Qt::UserRole).value<git::Reference>();
  if (!ref.isValid())
    return;

  QMenu menu;
  QAction *checkout = menu.addAction(tr("Checkout"), [this, ref] {
    RepoView::parentView(this)->checkout(ref);
  });

  RepoView *view = RepoView::parentView(this);
  checkout->setEnabled(!ref.isHead() && !view->repo().isBare());

  menu.addSeparator();

  if (ref.isLocalBranch()) {
    QAction *rename = menu.addAction(tr("Rename"), [this, ref] {
      RepoView *view = RepoView::parentView(this);
      ConfigDialog *dialog = view->configureSettings(ConfigDialog::Branches);
      dialog->editBranch(ref.name());
    });

    rename->setEnabled(!ref.isHead());
  }

  if (ref.isTag() || ref.isLocalBranch()) {
    QAction *remove = menu.addAction(tr("Delete"), [this, ref] {
      if (ref.isTag()) {
        DeleteTagDialog::open(ref, this);
      } else {
        DeleteBranchDialog::open(ref, this);
      }
    });

    remove->setEnabled(ref.isTag() || !ref.isHead());
  }

  if (ref.isTag()) {
    git::Remote remote = ref.repo().defaultRemote();
    if (remote.isValid()) {
      menu.addAction(tr("Push Tag to %1").arg(remote.name()),
        [this, ref, view, remote] {
          view->push(remote, ref);
        });
    }
  }

  if (ref.isRemoteBranch()) {
    QString local = ref.name().section('/', 1);
    QAction *newLocalBranch = menu.addAction(tr("New Local Branch %1").arg(local),
    [this, ref, local] {
      RepoView::parentView(this)->createBranch(local, ref.target(), ref, true);
    });

    newLocalBranch->setEnabled(!view->repo().lookupBranch(local, GIT_BRANCH_LOCAL));
  }

  menu.addSeparator();

  QAction *merge = menu.addAction(tr("Merge..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog =
      new MergeDialog(RepoView::Merge, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  QAction *rebase = menu.addAction(tr("Rebase..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog =
      new MergeDialog(RepoView::Rebase, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  QAction *squash = menu.addAction(tr("Squash..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog =
      new MergeDialog(RepoView::Squash, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  bool stash = ref.isStash();
  merge->setEnabled(!stash);
  rebase->setEnabled(!stash);
  squash->setEnabled(!stash);

  menu.exec(event->globalPos());
}

#include "ReferenceView.moc"

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "FileList.h"
#include "Badge.h"
#include "ContextMenuButton.h"
#include "FileContextMenu.h"
#include "RepoView.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "git/Diff.h"
#include "git/Patch.h"
#include "git/Repository.h"
#include "git/Config.h"
#include "tools/ExternalTool.h"
#include <QAbstractListModel>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QWindow>

namespace {

const QString kFileRowsKey = "file/rows";

class FileModel : public QAbstractListModel
{
  Q_OBJECT

public:
  FileModel(const git::Repository &repo, QObject *parent = nullptr)
    : QAbstractListModel(parent)
  {
    connect(repo.notifier(), &git::RepositoryNotifier::indexChanged,
            this, &FileModel::updateCheckState);
  }

  void setDiff(const git::Diff &diff)
  {
    beginResetModel();
    mDiff = diff;
    endResetModel();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override
  {
    return mDiff.isValid() ? mDiff.count() : 0;
  }

  QVariant data(
    const QModelIndex &index,
    int role = Qt::DisplayRole) const override
  {
    switch (role) {
      case Qt::DisplayRole:
        return mDiff.name(index.row());

      case Qt::DecorationRole:
        return git::Diff::statusChar(mDiff.status(index.row()));

      case Qt::CheckStateRole: {
        if (!mDiff.isStatusDiff())
          return QVariant();

        QString name = mDiff.name(index.row());
        switch (mDiff.index().isStaged(name)) {
          case git::Index::Disabled:
          case git::Index::Unstaged:
          case git::Index::Conflicted:
            return Qt::Unchecked;

          case git::Index::PartiallyStaged:
            return Qt::PartiallyChecked;

          case git::Index::Staged:
            return Qt::Checked;
        }
      }
    }

    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override
  {
    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
  }

  bool setData(
    const QModelIndex &index,
    const QVariant &value,
    int role = Qt::EditRole) override
  {
    switch (role) {
      case Qt::CheckStateRole: {
        QString name = mDiff.name(index.row());
        if (mDiff.index().isStaged(name) == git::Index::Conflicted &&
            mDiff.patch(index.row()).count() > 0)
          return false;

        mDiff.index().setStaged({name}, value.toBool(), mYieldFocus);
        emit dataChanged(index, index, {role});
        return true;
      }
    }

    return false;
  }

  void setYieldFocus(bool enabled)
  {
    mYieldFocus = enabled;
  }

private:
  void updateCheckState(const QStringList &paths)
  {
    foreach (const QString &path, paths) {
      QModelIndexList indexes = match(index(0, 0), Qt::DisplayRole, path);
      if (!indexes.isEmpty()) {
        QModelIndex index = indexes.first();
        emit dataChanged(index, index, {Qt::CheckStateRole});
      }
    }
  }

  git::Diff mDiff;

  bool mYieldFocus = true;
};

class FileDelegate : public QStyledItemDelegate
{
public:
  FileDelegate(QObject *parent = nullptr)
    : QStyledItemDelegate(parent)
  {}

  void paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override
  {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QString text = index.data(Qt::DecorationRole).toChar();
    QStyle::SubElement se = QStyle::SE_ItemViewItemDecoration;
    QRect rect = style->subElementRect(se, &opt, opt.widget);
    Badge::paint(painter, {text}, rect, &opt);
  }

  QSize sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override
  {
    // Increase spacing.
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(Badge::size(option.font).height() + 4);
    return size;
  }

  QRect checkRect(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
  {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (!(opt.features & QStyleOptionViewItem::HasCheckIndicator))
      return QRect();

    QStyle::SubElement se = QStyle::SE_ItemViewItemCheckIndicator;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    return style->subElementRect(se, &opt, opt.widget);
  }

protected:
  void initStyleOption(
    QStyleOptionViewItem *option,
    const QModelIndex &index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);
    option->decorationSize = Badge::size(option->font);
  }

  bool editorEvent(
    QEvent *event,
    QAbstractItemModel *model,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) override
  {
    FileModel *fm = static_cast<FileModel *>(model);
    fm->setYieldFocus(event->type() != QEvent::KeyPress);
    bool result = QStyledItemDelegate::editorEvent(event, model, option, index);
    fm->setYieldFocus(true);
    return result;
  }
};

} // anon. namespace

FileList::FileList(const git::Repository &repo, QWidget *parent)
  : QListView(parent)
{
  Theme *theme = Application::theme();
  setPalette(theme->fileList());
  
  setAlternatingRowColors(true);
  setFrameShape(QFrame::NoFrame);
  setAttribute(Qt::WA_MacShowFocusRect, false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setModel(new FileModel(repo, this));
  setItemDelegate(new FileDelegate(this));

  mButton = new ContextMenuButton(this);
  QMenu *menu = new QMenu(this);
  mButton->setMenu(menu);

  mSortMenu = menu->addMenu(tr("Sort By"));
  mSelectMenu = menu->addMenu(tr("Select"));

  mSortName = mSortMenu->addAction(tr("Name"), [this] {
    Settings *settings = Settings::instance();
    Qt::SortOrder order = Qt::AscendingOrder;
    if (settings->value("sort/role").toInt() == git::Diff::NameRole &&
        settings->value("sort/order").toInt() == Qt::AscendingOrder)
      order = Qt::DescendingOrder;
    settings->setValue("sort/order", order);
    settings->setValue("sort/role", git::Diff::NameRole);
    emit sortRequested();
  });

  mSortStatus = mSortMenu->addAction(tr("Status"), [this] {
    Settings *settings = Settings::instance();
    Qt::SortOrder order = Qt::AscendingOrder;
    if (settings->value("sort/role").toInt() == git::Diff::StatusRole &&
        settings->value("sort/order").toInt() == Qt::AscendingOrder)
      order = Qt::DescendingOrder;
    settings->setValue("sort/order", order);
    settings->setValue("sort/role", git::Diff::StatusRole);
    emit sortRequested();
  });

  menu->addSeparator();

  mIgnoreWs = menu->addAction(tr("Ignore Whitespace (-w)"));
  mIgnoreWs->setCheckable(true);
  connect(mIgnoreWs, &QAction::triggered, [](bool checked) {
    Settings::instance()->setWhitespaceIgnored(checked);
  });

  mHideUt = menu->addAction(tr("Hide untracked files"));
  mHideUt->setCheckable(true);
  mHideUt->setChecked(RepoView::parentView(parent)->repo().appConfig().value<bool>("untracked.hide", false));
  connect(mHideUt, &QAction::triggered, [parent, this](bool checked) {
    RepoView *view = RepoView::parentView(this);
    view->repo().appConfig().setValue("untracked.hide", checked);
    view->refresh();
});

  connect(this, &FileList::doubleClicked, [this](const QModelIndex &index) {
    if (!index.isValid())
      return;

    QString file = index.data().toString();
    RepoView *view = RepoView::parentView(this);

    // Launch external tool.
    git::Diff diff = view->diff();
    git::Repository repo = view->repo();
    if (ExternalTool *tool = ExternalTool::create(file, diff, repo, this)) {
      // Diff and merge tool both detach from parent after start.
      if (tool->isValid() && tool->start())
        return;

      delete tool;
    }

    // Open editor.
    view->edit(file);
  });
}

void FileList::resizeEvent(QResizeEvent *event)
{
  QListView::resizeEvent(event);
  mButton->move(viewport()->width() - mButton->width() - 2, y() + 2);
}

void FileList::setDiff(const git::Diff &diff, const QString &pathspec)
{
  mDiff = diff;

  FileModel *model = static_cast<FileModel *>(this->model());
  model->setDiff(diff);

  int rows = model->rowCount();
  setVisible(rows > 0);
  if (rows > 0) {
    int rowHeight = sizeHintForRow(0);
    setMinimumHeight(rowHeight + 1);
    setMaximumHeight(rowHeight * rows + 1);
  }

  updateMenu(diff);

  if (pathspec.isEmpty())
    return;

  QItemSelection selection;
  for (int i = 0; i < rows; ++i) {
    QModelIndex index = model->index(i, 0);
    if (index.data().toString().startsWith(pathspec))
      selection.select(index, index);
  }

  selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

QSize FileList::sizeHint() const
{
  QSettings settings;
  QSize size = QListView::sizeHint();
  int rows = qMin(model()->rowCount(), settings.value(kFileRowsKey, 5).toInt());
  return (rows > 0) ? QSize(size.width(), sizeHintForRow(0) * rows + 1) : size;
}

QRect FileList::checkRect(const QModelIndex &index)
{
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);

  FileDelegate *delegate = static_cast<FileDelegate *>(itemDelegate());
  return delegate->checkRect(options, index);
}

void FileList::setFileRows(int rows)
{
  QSettings settings;
  settings.setValue(kFileRowsKey, rows);
}

void FileList::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);
  if (checkRect(index).contains(pos))
    return;

  QListView::mouseMoveEvent(event);
}

void FileList::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  if (event->button() == Qt::LeftButton && selectionModel()->isSelected(index))
    mPressedIndex = index;

  QListView::mousePressEvent(event);
}

void FileList::mouseReleaseEvent(QMouseEvent *event)
{
  // Make sure the check state isn't changing.
  QModelIndex index = indexAt(event->pos());
  QVariant checked = index.data(Qt::CheckStateRole);

  QListView::mouseReleaseEvent(event);

  if (event->button() == Qt::LeftButton &&
      index.data(Qt::CheckStateRole) == checked && index == mPressedIndex) {
    if (event->modifiers() & Qt::ControlModifier) {
      selectionModel()->select(index, QItemSelectionModel::Deselect);
    } else {
      clearSelection();
    }
  }

  mPressedIndex = QModelIndex();
}

void FileList::contextMenuEvent(QContextMenuEvent *event)
{
  QStringList files;
  foreach (const QModelIndex &index, selectionModel()->selectedIndexes())
    files.append(index.data(Qt::DisplayRole).toString());

  RepoView *view = RepoView::parentView(this);
  FileContextMenu menu(view, files, mDiff.index());
  menu.exec(event->globalPos());
}

void FileList::updateMenu(const git::Diff &diff)
{
  if (!diff.isValid())
    return;

  // sort
  Settings *settings = Settings::instance();
  int role = settings->value("sort/role").toInt();
  int order = settings->value("sort/order").toInt();

  qreal dpr = window()->windowHandle()->devicePixelRatio();
  QPixmap pixmap(16 * dpr, 16 * dpr);
  pixmap.setDevicePixelRatio(dpr);
  pixmap.fill(Qt::transparent);

  QIcon spacer;
  spacer.addPixmap(pixmap);

  qreal x = pixmap.width() / (2 * dpr);
  qreal y = pixmap.height() / (2 * dpr);
  qreal f = (order == Qt::AscendingOrder) ? 1 : -1;

  QPainterPath path;
  path.moveTo(x - (3 * f), y - (1.5 * f));
  path.lineTo(x, y + (1.5 * f));
  path.lineTo(x + (3 * f), y - (1.5 * f));

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(palette().color(QPalette::Text), 1.5));
  painter.drawPath(path);

  QIcon icon;
  icon.addPixmap(pixmap);

  mSortName->setIcon(role == git::Diff::NameRole ? icon : spacer);
  mSortStatus->setIcon(role == git::Diff::StatusRole ? icon : spacer);

  // select
  mSelectMenu->clear();
  QSet<git_delta_t> kinds;
  for (int i = 0; i < diff.count(); ++i)
    kinds.insert(diff.status(i));
  foreach (git_delta_t kind, kinds) {    
    QString name;
    switch (kind) {
      case GIT_DELTA_ADDED:      name = tr("Added");       break;
      case GIT_DELTA_DELETED:    name = tr("Deleted");     break;
      case GIT_DELTA_MODIFIED:   name = tr("Modified");    break;
      case GIT_DELTA_RENAMED:    name = tr("Renamed");     break;
      case GIT_DELTA_COPIED:     name = tr("Copied");      break;
      case GIT_DELTA_IGNORED:    name = tr("Ignored");     break;
      case GIT_DELTA_UNTRACKED:  name = tr("Untracked");   break;
      case GIT_DELTA_UNREADABLE: name = tr("Unreadable");  break;
      case GIT_DELTA_CONFLICTED: name = tr("Conflicted");  break;
      case GIT_DELTA_UNMODIFIED: name = tr("Unmodified");  break;
      case GIT_DELTA_TYPECHANGE: name = tr("Type Change"); break;
    }

    mSelectMenu->addAction(name, [this, kind] {
      QItemSelection selection;
      QString status(git::Diff::statusChar(kind));
      for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex index = model()->index(i, 0);
        if (index.data(Qt::DecorationRole).toChar() == status)
          selection.select(index, index);
      }

      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    }); 
  }

  // ignore whitespace
  mIgnoreWs->setChecked(settings->isWhitespaceIgnored());
}

#include "FileList.moc"

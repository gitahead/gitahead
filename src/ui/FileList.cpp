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
#include "tools/ExternalTool.h"
#include <QAbstractListModel>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
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

class Menu : public QMenu
{
  Q_OBJECT

public:
  Menu(QMenu *parent = nullptr)
    : QMenu(parent)
  {}

signals:
  void keyPressed(QAction *action, int key, ulong msDiff);
  void mouseWheel(QAction *action, int wheelX, int wheelY);

private:
  void actionEvent(QActionEvent *e) override
  {
    QMenu::actionEvent(e);

    if (e->type() == QEvent::ActionAdded) {
      QAction *action = e->action();
      connect(action, &QAction::hovered, [this, action] {
        mAction = action;
      });
    }
  }

  void keyPressEvent(QKeyEvent *e) override
  {
    if (mAction != nullptr) {
      ulong timestamp = e->timestamp();
      emit keyPressed(mAction, e->key(), timestamp - mTimeStamp);
      mTimeStamp = timestamp;
    }
    QMenu::keyPressEvent(e);
  }

  void mouseMoveEvent(QMouseEvent *e) override
  {
    mAction = actionAt(e->pos());
    QMenu::mouseMoveEvent(e);
  }

  void wheelEvent(QWheelEvent *e) override
  {
    QPoint degrees = e->angleDelta();
    if (!degrees.isNull()) {

      // Degrees in 1/8 of a degree
      degrees /= 8;

      // Default wheel step = 15 degrees
      QPoint numSteps = degrees / 15;
      if ((numSteps.x() != 0) || (numSteps.y() != 0)) {
        if (mAction != nullptr)
          emit mouseWheel(mAction, numSteps.x(), numSteps.y());
      }
    }
    QMenu::wheelEvent(e);
  }

  QAction *mAction = nullptr;
  ulong mTimeStamp;
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

  Settings *settings = Settings::instance();

  // Load sort role and order map.
  QByteArray inArray;
  inArray = settings->value("sort/map").toByteArray();
  QDataStream inStream(&inArray, QIODevice::ReadOnly);
  inStream >> mSortMap;

  // Map sort role validation.
  bool invalid = mSortMap.count() != 5;
  QList<git::Diff::SortRole> roleList;
  for (int i = 0; i < mSortMap.count(); i++) {
    QByteArray ba = mSortMap.value(i, QByteArray(2, -1));
    git::Diff::SortRole role = static_cast<git::Diff::SortRole>(ba.at(0));
    if ((role >= git::Diff::NameRole) &&
        (role <= git::Diff::ExtensionRole))
      roleList.append(role);
    else {
      invalid = true;
      break;
    }
  }
  if (!roleList.contains(git::Diff::NameRole))       invalid = true;
  if (!roleList.contains(git::Diff::PathRole))       invalid = true;
  if (!roleList.contains(git::Diff::StatusRole))     invalid = true;
  if (!roleList.contains(git::Diff::BinaryRole))     invalid = true;
  if (!roleList.contains(git::Diff::ExtensionRole))  invalid = true;

  // Default sort role and order map.
  if (invalid) {
    QByteArray ba(2, -1);
    mSortMap.clear();
    ba[0] = git::Diff::PathRole;
    ba[1] = Qt::AscendingOrder;
    mSortMap.insert(0, ba);
    ba[0] = git::Diff::NameRole;
    ba[1] = Qt::AscendingOrder;
    mSortMap.insert(1, ba);
    ba[0] = git::Diff::ExtensionRole;
    ba[1] = Qt::AscendingOrder;
    mSortMap.insert(2, ba);
    ba[0] = git::Diff::StatusRole;
    ba[1] = -1;
    mSortMap.insert(3, ba);
    ba[0] = git::Diff::BinaryRole;
    ba[1] = -1;
    mSortMap.insert(4, ba);
  }

  // Setup sort actions.
  for (int i = 0; i < mSortMap.count(); i++) {
    QByteArray ba = mSortMap.value(i, QByteArray(2, -1));
    QAction *action = nullptr;

    // Sort role.
    switch (ba[0]) {
      case git::Diff::NameRole:
        action = new QAction(tr("File Name"));
        break;
      case git::Diff::PathRole:
        action = new QAction(tr("File Path"));
        break;
      case git::Diff::StatusRole:
        action = new QAction(tr("Status"));
        break;
      case git::Diff::BinaryRole:
        action = new QAction(tr("Text/Binary"));
        break;
      case git::Diff::ExtensionRole:
        action = new QAction(tr("File Extension"));
        break;
      default:
        action = new QAction("---");
        break;
    }

    action->setToolTip(tr("Use mouse wheel and '+', '-', space key"));
    action->setData(i);
    mActionList.append(action);
  }

  mButton = new ContextMenuButton(this);
  QMenu *menu = new QMenu(this);
  mButton->setMenu(menu);

  Menu *sortMenu = new Menu();
  sortMenu->setTitle(tr("Sort By"));
  menu->addMenu(sortMenu);

  sortMenu->addActions(mActionList);
  connect(sortMenu, &Menu::mouseWheel, [this, sortMenu](QAction *action, int wheelX, int wheelY) {
    sortMenu->setToolTipsVisible(false);

    int i = action->data().toInt();
    bool save = true;
    QByteArray ba = mSortMap.take(i);

    if (wheelY > 0) {
      switch (ba[1]) {
        case Qt::AscendingOrder:
          ba[1] = -1;
          break;
        case Qt::DescendingOrder:
          save = false;
          break;
        default:
          ba[1] = Qt::DescendingOrder;
          break;
      }
    }
    if (wheelY < 0) {
      switch (ba[1]) {
        case Qt::AscendingOrder:
          save = false;
          break;
        case Qt::DescendingOrder:
          ba[1] = -1;
          break;
        default:
          ba[1] = Qt::AscendingOrder;
          break;
      }
    }

    mSortMap.insert(i, ba);

    if (save) {
      // Sort order.
      switch (ba[1]) {
        case Qt::AscendingOrder:
          action->setIcon(mAscIcon);
          break;
        case Qt::DescendingOrder:
          action->setIcon(mDesIcon);
          break;
        default:
          action->setIcon(mSpacerIcon);
          break;
      }

      // Save sort settings.
      Settings *settings = Settings::instance();
      QByteArray outArray;
      QDataStream outStream(&outArray, QIODevice::WriteOnly);
      outStream << mSortMap;
      settings->setValue("sort/map", outArray);

      emit sortRequested();
    }
  });

  connect(sortMenu, &Menu::keyPressed, [this, sortMenu](QAction *action, int key, ulong msDiff) {
    sortMenu->setToolTipsVisible(false);

    int i = action->data().toInt();
    bool save = false;

    // Next sort order.
    if (key == Qt::Key_Space) {
      QByteArray ba = mSortMap.take(i);

      // Set next sort order.
      switch (ba[1]) {
        case Qt::AscendingOrder:
          ba[1] = Qt::DescendingOrder;
          action->setIcon(mDesIcon);
          break;
        case Qt::DescendingOrder:
          ba[1] = -1;
          action->setIcon(mSpacerIcon);
          break;
        default:
          ba[1] = Qt::AscendingOrder;
          action->setIcon(mAscIcon);
          break;
      }

      mSortMap.insert(i, ba);
      save = true;
    }

    // Sort role.
    int j = -1;
    if (key == Qt::Key_Minus)
      j = i + 1;
    if (key == Qt::Key_Plus)
      j = i - 1;

    // Swap sort actions.
    if ((j >= 0) && (j < mSortMap.count())) {
      QByteArray baj = mSortMap.take(i);
      QByteArray bai = mSortMap.take(j);
      mSortMap.insert(i, bai);
      mSortMap.insert(j, baj);

      for (i = 0; i < mSortMap.count(); i++) {
        QByteArray ba = mSortMap.value(i, QByteArray(2, -1));

        // Sort role.
        switch (ba[0]) {
          case git::Diff::NameRole:
            mActionList[i]->setText(tr("File Name"));
            break;
          case git::Diff::PathRole:
            mActionList[i]->setText(tr("File Path"));
            break;
          case git::Diff::StatusRole:
            mActionList[i]->setText(tr("Status"));
            break;
          case git::Diff::BinaryRole:
            mActionList[i]->setText(tr("Text/Binary"));
            break;
          case git::Diff::ExtensionRole:
            mActionList[i]->setText(tr("File Extension"));
            break;
          default:
            mActionList[i]->setText("---");
            break;
        }

        // Sort order.
        switch (ba[1]) {
          case Qt::AscendingOrder:
            mActionList[i]->setIcon(mAscIcon);
            break;
          case Qt::DescendingOrder:
            mActionList[i]->setIcon(mDesIcon);
            break;
          default:
            mActionList[i]->setIcon(mSpacerIcon);
            break;
        }
      }
      save = true;
    }

    // Save sort settings.
    if (save) {
      Settings *settings = Settings::instance();
      QByteArray outArray;
      QDataStream outStream(&outArray, QIODevice::WriteOnly);
      outStream << mSortMap;
      settings->setValue("sort/map", outArray);

      emit sortRequested();
    }
  });

  connect(sortMenu, &QMenu::aboutToShow, [sortMenu] {
    sortMenu->setToolTipsVisible(true);
  });

  mSelectMenu = menu->addMenu(tr("Select"));

  menu->addSeparator();

  mIgnoreWs = menu->addAction(tr("Ignore Whitespace (-w)"));
  mIgnoreWs->setCheckable(true);
  connect(mIgnoreWs, &QAction::triggered, [](bool checked) {
    Settings::instance()->setWhitespaceIgnored(checked);
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
  mButton->move(viewport()->width() - mButton->width() - 2, y() + 3);
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

  //Setup icons.
  qreal dpr = window()->windowHandle()->devicePixelRatio();

  if (mSpacerIcon.isNull()) {
    QPixmap spacermap(16 * dpr, 16 * dpr);
    spacermap.setDevicePixelRatio(dpr);
    spacermap.fill(Qt::transparent);
    mSpacerIcon.addPixmap(spacermap);
  }

  if (mAscIcon.isNull()) {
    QPixmap ascmap(16 * dpr, 16 * dpr);
    ascmap.setDevicePixelRatio(dpr);
    ascmap.fill(Qt::transparent);

    qreal x = ascmap.width() / (2 * dpr);
    qreal y = ascmap.height() / (2 * dpr);
    qreal f = 1;

    QPainterPath ascpath;
    ascpath.moveTo(x - (3 * f), y - (1.5 * f));
    ascpath.lineTo(x, y + (1.5 * f));
    ascpath.lineTo(x + (3 * f), y - (1.5 * f));

    QPainter ascpainter(&ascmap);
    ascpainter.setRenderHint(QPainter::Antialiasing);
    ascpainter.setPen(QPen(palette().color(QPalette::Text), 1.5));
    ascpainter.drawPath(ascpath);
    mAscIcon.addPixmap(ascmap);
  }

  if (mDesIcon.isNull()) {
    QPixmap desmap(16 * dpr, 16 * dpr);
    desmap.setDevicePixelRatio(dpr);
    desmap.fill(Qt::transparent);
    qreal x = desmap.width() / (2 * dpr);
    qreal y = desmap.height() / (2 * dpr);
    qreal f = -1;

    QPainterPath despath;
    despath.moveTo(x - (3 * f), y - (1.5 * f));
    despath.lineTo(x, y + (1.5 * f));
    despath.lineTo(x + (3 * f), y - (1.5 * f));

    QPainter despainter(&desmap);
    despainter.setRenderHint(QPainter::Antialiasing);
    despainter.setPen(QPen(palette().color(QPalette::Text), 1.5));
    despainter.drawPath(despath);
    mDesIcon.addPixmap(desmap);
  }

  // Sort order icons.
  for (int i = 0; i < mSortMap.count(); i++) {
    QByteArray ba = mSortMap.value(i, QByteArray(2, -1));

    // Sort order icon.
    switch (ba[1]) {
      case Qt::AscendingOrder:
        mActionList[i]->setIcon(mAscIcon);
        break;
      case Qt::DescendingOrder:
        mActionList[i]->setIcon(mDesIcon);
        break;
      default:
        mActionList[i]->setIcon(mSpacerIcon);
        break;
    }
  }

  // Select.
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

  // Ignore whitespace.
  mIgnoreWs->setChecked(Settings::instance()->isWhitespaceIgnored());
}

#include "FileList.moc"

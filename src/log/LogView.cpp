//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "LogView.h"
#include "LogDelegate.h"
#include "LogEntry.h"
#include "LogModel.h"
#include "git/Config.h"
#include <QAction>
#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMimeData>
#include <QMouseEvent>

const int kIndentSpaces = 4;
const int kSaveDelay = 1000;

const int kDefaultIndent = 1;
const int kDefaultKind = 0;
const int kDefaultStatus = 0;

namespace {

QModelIndexList collectSelectedIndexes(
  QTreeView *view,
  const QModelIndex &parent)
{
  QModelIndexList selection;
  QAbstractItemModel *model = view->model();
  for (int i = 0; i < model->rowCount(parent); ++i) {
    QModelIndex index = model->index(i, 0, parent);
    if (view->selectionModel()->isSelected(index))
      selection.append(index);
    if (view->isExpanded(index))
      selection.append(collectSelectedIndexes(view, index));
  }
  return selection;
}

QModelIndexList collectIndexes(
  QTreeView *view,
  const QModelIndex &parent)
{
  QModelIndexList selection;
  QAbstractItemModel *model = view->model();
  for (int i = 0; i < model->rowCount(parent); ++i) {
    QModelIndex index = model->index(i, 0, parent);
    selection.append(index);
    selection.append(collectIndexes(view, index));
  }
  return selection;
}

QModelIndex lastRootIndex(
  QTreeView *view,
  const QModelIndex &parent)
{
  QAbstractItemModel *model = view->model();
  int i = model->rowCount(parent) - 1;
  return model->index(i, 0, parent);
}

} // anon. namespace

LogView::LogView(LogEntry *root,
                 const git::Repository *repo,
                 QWidget *parent)
  : QTreeView(parent), mRoot(root)
{
  setMouseTracking(true);
  setHeaderHidden(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setContextMenuPolicy(Qt::ActionsContextMenu);

  QHeaderView *header = this->header();
  header->setStretchLastSection(false);
  header->setSectionResizeMode(QHeaderView::ResizeToContents);

  mCopyAction = new QAction(tr("Copy Selection to Clipboard"), this);
  mCopyAction->setEnabled(false);
  addAction(mCopyAction);
  connect(mCopyAction, &QAction::triggered, [this] {
    copy(true);
  });

  QAction *copyAllAction = new QAction(tr("Copy Log to Clipboard"), this);
  copyAllAction->setEnabled(false);
  addAction(copyAllAction);
  connect(copyAllAction, &QAction::triggered, [this] {
    copy(false);
  });

  mClearAction = new QAction(tr("Clear Selection"), this);
  mClearAction->setEnabled(false);
  addAction(mClearAction);
  connect(mClearAction, &QAction::triggered, [this] {
    clear(true);
  });

  QAction *clearAllAction = new QAction(tr("Clear Log"), this);
  clearAllAction->setEnabled(false);
  addAction(clearAllAction);
  connect(clearAllAction, &QAction::triggered, [this] {
    clear(false);
  });

  QAction *saveAction = new QAction(tr("Save Log as..."), this);
  saveAction->setEnabled(false);
  addAction(saveAction);
  connect(saveAction, &QAction::triggered, [this] {
    save(true);
  });

  LogModel *model = new LogModel(root, style(), this);
  LogDelegate *delegate = new LogDelegate(this);
  connect(model, &LogModel::dataChanged,
          delegate, &LogDelegate::invalidateCache);

  setModel(model);
  setItemDelegate(delegate);

  connect(model, &QAbstractItemModel::rowsInserted,
  [this, copyAllAction, clearAllAction, saveAction](const QModelIndex &parent, int first, int last) {
    if (!parent.isValid() && mCollapse)
      collapse(this->model()->index(last - 1, 0, parent));
    scrollTo(this->model()->index(last, 0, parent));

    copyAllAction->setEnabled(true);
    clearAllAction->setEnabled(true);
    saveAction->setEnabled(true);

    if (mRepo.isValid()) {
      // Max log entrys.
      while (this->model()->rowCount() > mRepo.appConfig().value<int>("log.maxentrys", 300))
        mRoot->removeEntry(0);

      mLogTimer.start(kSaveDelay);
    }
  });

  connect(model, &QAbstractItemModel::rowsRemoved,
  [this, copyAllAction, clearAllAction, saveAction](const QModelIndex &parent, int first, int last) {
    if (this->model()->rowCount() == 0) {
      copyAllAction->setEnabled(false);
      clearAllAction->setEnabled(false);
      saveAction->setEnabled(false);
    }

    if (mRepo.isValid())
      mLogTimer.start(kSaveDelay);
  });

  // Repository is valid.
  if (repo != nullptr) {
    mRepo = *repo;

    // Load log.
    load();

    // Save log on data change.
    connect(model, &QAbstractItemModel::dataChanged,
    [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
      mLogTimer.start(kSaveDelay);
    });
    connect(&mLogTimer, &QTimer::timeout, [this] {
      mLogTimer.stop();
      save();
    });
  }
}

void LogView::setCollapseEnabled(bool collapse) 
{
  mCollapse = collapse;
}

void LogView::setEntryExpanded(LogEntry *entry, bool expanded)
{
  setExpanded(static_cast<LogModel *>(model())->index(entry), expanded);
}

void LogView::copy(bool selection)
{
  QString plainText;
  QString richText;
  QModelIndexList indexes;

  if (selection)
    indexes = collectSelectedIndexes(this, QModelIndex());
  else
    indexes = collectIndexes(this, QModelIndex());

  foreach (const QModelIndex &index, indexes) {
    QString prefix;

    // Indent child indices.
    QModelIndex currentParent = index.parent();
    while (currentParent.isValid()){
      prefix += QString(kIndentSpaces, ' ');
      currentParent = currentParent.parent();
    }

    // Add expansion indicator.
    if (this->model()->hasChildren(index)) {
      if (selection)
        prefix += isExpanded(index) ? "[-]" : "[+]";
      else
        prefix += "[-]";
    }

    QString text = index.data().toString();
    plainText += QString("%1 %2\n").arg(prefix, text.remove(QRegExp("<[^>]*>")));
    richText += QString("%1 %2<br>").arg(prefix, text);
  }

  richText = QString("<pre>%1</pre>").arg(richText);

  QMimeData *mimeDate = new QMimeData;
  mimeDate->setText(plainText);
  mimeDate->setHtml(richText);
  QApplication::clipboard()->setMimeData(mimeDate);
}

void LogView::clear(bool selection)
{
  QModelIndexList indexes;
  if (selection)
    indexes = collectSelectedIndexes(this, QModelIndex());
  else
    indexes = collectIndexes(this, QModelIndex());

  for (int i = indexes.count() - 1; i >= 0; i--)
    mRoot->removeEntry(indexes[i].row());
}

void LogView::load()
{
  if (!mRepo.isValid())
    return;

  // Load log.
  QJsonDocument jsondoc;
  QFile log(mRepo.dir().filePath("gitahead/log.json"));
  if (log.open(QIODevice::ReadOnly)) {
    jsondoc = QJsonDocument::fromJson(log.readAll());
    log.close();
  }
  if (jsondoc.isEmpty())
    return;

  // Disable save() timer and clear log.
  mLogTimer.blockSignals(true);
  clear(false);

  LogEntry *entry = nullptr;
  int indent = 0;
  QJsonArray jsonarr = jsondoc.array();
  foreach (const QJsonValue &jsonval, jsonarr) {
    QJsonObject json = jsonval.toObject();

    if (json.contains("timestamp")) {
      QDateTime timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
      QString title = json["title"].toString();
      QString text = json["text"].toString();
      entry = mRoot->addEntry(text, title, timestamp);
      indent = 1;

    } else if (indent > 0) {
      int childindent = json["indent"].toInt(kDefaultIndent);
      LogEntry::Kind kind = static_cast<LogEntry::Kind>(json["kind"].toInt(kDefaultKind));
      char status = json["status"].toInt(kDefaultStatus);
      QString title = json["title"].toString();
      QString text = json["text"].toString();

      while ((indent > childindent) && (entry->parentEntry() != mRoot)) {
        entry = entry->parentEntry();
        indent--;
      }

      entry = entry->addEntry(kind, text, title);
      entry->setStatus(status);
      indent += 1;
    }

    // Collapse entry.
    if (indent > 0)
      setEntryExpanded(entry->parentEntry(), false);
  }

  // Max log entrys.
  while (this->model()->rowCount() > mRepo.appConfig().value<int>("log.maxentrys", 300))
    mRoot->removeEntry(0);

  // Reset and restart save() timer.
  mLogTimer.stop();
  mLogTimer.blockSignals(false);
}

void LogView::save(bool as)
{
  QString filename;

  if (as) {
    QStringList filter = { tr("Text Log (*.json)"), tr("Binary Log (*.dat)") };
    filename = QFileDialog::getSaveFileName(nullptr,
                                            tr("Save Log"),
                                            QDir::homePath() + "/log.json",
                                            filter.join(";;"),
                                            &filter.first());
  } else if (mRepo.isValid()) {
    filename = mRepo.dir().filePath("gitahead/log.json");
  }
  if (filename.isEmpty())
    return;

  // Last entry is busy.
  QModelIndex last = lastRootIndex(this, QModelIndex());
  if (last.isValid()) {
    LogEntry *entry = static_cast<LogEntry *>(last.internalPointer());
    if (entry->progress() >= 0)
      return;
  }

  QJsonArray jsonarr;
  QModelIndexList indexes = collectIndexes(this, QModelIndex());
  foreach (const QModelIndex &index, indexes) {
    LogEntry *entry = static_cast<LogEntry *>(index.internalPointer());
    QModelIndex currentParent = index.parent();
    QJsonObject json;

    if (!currentParent.isValid()) {
      json.insert("timestamp", entry->timestamp().toString(Qt::ISODate));
    } else {
      int indent = 0;
      while (currentParent.isValid()) {
        indent += 1;
        currentParent = currentParent.parent();
      }
      if (indent != kDefaultIndent)
        json.insert("indent", indent);
      if (entry->kind() != kDefaultKind)
        json.insert("kind", entry->kind());
      if (entry->status() != kDefaultStatus)
        json.insert("status", entry->status());
    }
    if (!entry->title().isEmpty())
      json.insert("title", entry->title());
    if (!entry->text().isEmpty())
      json.insert("text", entry->text());

    jsonarr.append(json);
  }

  // Save log.
  if (!jsonarr.isEmpty()) {
    QJsonDocument jsondoc(jsonarr);
    QFile log(filename);
    if (log.open(QIODevice::WriteOnly)) {
      if (filename.endsWith(".dat"))
        log.write(jsondoc.toBinaryData());
      else
        log.write(jsondoc.toJson());
      log.close();
    }
  }
}

QSize LogView::minimumSizeHint() const
{
  return QSize(QTreeView::minimumSizeHint().width(), 0);
}

void LogView::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  QString link = linkAt(index, pos);
  setCursor(link.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);

  if (!mLink.isEmpty() || mCancel.isValid())
    return;

  QTreeView::mouseMoveEvent(event);
}

void LogView::mousePressEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  mLink = linkAt(index, pos);

  mCancel = QModelIndex();
  if (isDecoration(index, pos) &&
      index.data(LogModel::EntryRole).value<LogEntry *>()->progress() >= 0)
    mCancel = index;

  if (!mLink.isEmpty() || mCancel.isValid())
    return;

  QTreeView::mousePressEvent(event);
}

void LogView::mouseReleaseEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  if (!mLink.isEmpty() && mLink == linkAt(index, pos)) {
    if (mLink == "expand") {
      setExpanded(index, !isExpanded(index));
    } else {
      emit linkActivated(mLink);
    }
  }

  if (mCancel == index && isDecoration(index, pos))
    emit operationCanceled(index);

  if (!mLink.isEmpty() || mCancel.isValid()) {
    mLink = QString();
    mCancel = QModelIndex();
    return;
  }

  QTreeView::mouseReleaseEvent(event);
}

void LogView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  bool enable = !selected.isEmpty();
  mCopyAction->setEnabled(enable);
  mClearAction->setEnabled(enable);

  QTreeView::selectionChanged(selected, deselected);
}

QString LogView::linkAt(const QModelIndex &index, const QPoint &pos)
{
  if (!index.isValid())
    return QString();

  LogDelegate *delegate = static_cast<LogDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  QPoint docPos = pos - delegate->documentPosition(options, index);
  return delegate->document(index)->documentLayout()->anchorAt(docPos);
}

bool LogView::isDecoration(const QModelIndex &index, const QPoint &pos)
{
  if (!index.isValid())
    return false;

  LogDelegate *delegate = static_cast<LogDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  return delegate->decorationRect(options, index).contains(pos);
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "LogEntry.h"

LogEntry::LogEntry(QObject *parent)
  : QObject(parent)
{}

LogEntry::LogEntry(
  Kind kind,
  const QString &text,
  const QString &title,
  const QDateTime &timestamp,
  LogEntry *parent)
  : QObject(parent), mKind(kind), mText(text), mTitle(title), mTimestamp(timestamp)
{
  // Connect progress timer.
  connect(&mTimer, &QTimer::timeout, [this] {
    ++mProgress;
    if (LogEntry *root = rootEntry())
      emit root->dataChanged(this);
  });
}

void LogEntry::setText(const QString &text)
{
  mText = text;
  if (LogEntry *root = rootEntry())
    emit root->dataChanged(this);
}

LogEntry *LogEntry::rootEntry() const
{
  LogEntry *parent = parentEntry();
  return parent ? parent->rootEntry() : const_cast<LogEntry *>(this);
}

LogEntry *LogEntry::parentEntry() const
{
  return qobject_cast<LogEntry *>(parent());
}

void LogEntry::addEntries(const QList<LogEntry *> &entries)
{
  insertEntries(mEntries.size(), entries);
}

LogEntry *LogEntry::addEntry(Kind kind,
                             const QString &text,
                             const QString &title)
{
  return insertEntry(mEntries.size(), kind, text, title);
}

LogEntry *LogEntry::addEntry(const QString &text,
                             const QString &title,
                             const QDateTime &timestamp)
{
  return insertEntry(mEntries.size(), Entry, text, title, timestamp);
}

void LogEntry::insertEntries(int row, const QList<LogEntry *> &entries)
{
  LogEntry *root = rootEntry();
  emit root->entriesAboutToBeInserted(this, row, entries.size());
  for (int i = 0; i < entries.size(); ++i) {
    LogEntry *entry = entries.at(i);
    entry->setParent(this);
    mEntries.insert(row + i, entry);
    if (entry->kind() == Error)
      emit root->errorInserted();
  }
  emit root->entriesInserted();
}

LogEntry *LogEntry::insertEntry(
  int row,
  Kind kind,
  const QString &text,
  const QString &title,
  const QDateTime &timestamp)
{
  LogEntry *entry = new LogEntry(kind, text, title, timestamp, this);
  insertEntries(row, {entry});
  return entry;
}

void LogEntry::delEntry(const LogEntry *entry)
{
  for (int i = 0; i < mEntries.size(); ++i) {
    if (entry == mEntries.at(i)) {
      removeEntry(i);
      break;
    }
  }
}

void LogEntry::removeEntry(int row)
{
  LogEntry *root = rootEntry();
  emit root->entriesAboutToBeRemoved(this, row, mEntries.size());
  mEntries.removeAt(row);
  emit root->entriesRemoved();
}

void LogEntry::setBusy(bool busy)
{
  if (!busy) {
    mTimer.stop();
    mProgress = -1;
  } else {
    mProgress = 0;
    mTimer.start(50);
  }

  if (LogEntry *root = rootEntry())
    emit root->dataChanged(this);
}

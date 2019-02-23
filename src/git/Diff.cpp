//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Diff.h"
#include "Patch.h"
#include "conf/Settings.h"
#include "git2/patch.h"
#include <algorithm>

namespace git {

namespace {

bool dirCompare(const QString &lhsName, const QString &rhsName,
                bool ascending, Qt::CaseSensitivity cs) {
  const int lLength = lhsName.length();
  const int rLength = rhsName.length();

  for (int start = 0; ;) {
    int lIdx = start == lLength ? -1 : lhsName.indexOf('/', start);
    int rIdx = start == rLength ? -1 : rhsName.indexOf('/', start);

    if (lIdx == -1) {
      if (rIdx == -1) {
        int i = QStringRef::compare(
          QStringRef(&lhsName, start, lLength - start),
          QStringRef(&rhsName, start, rLength - start),
          cs
        );

        if (ascending) {
          return i < 0;

        } else {
          return i > 0;
        }
      } else {
        return false;
      }
    } else if (rIdx == -1) {
      return true;

    } else {
      int i = QStringRef::compare(
        QStringRef(&lhsName, start, lIdx - start),
        QStringRef(&rhsName, start, rIdx - start),
        cs
      );

      if (i < 0) {
        return ascending;

      } else if (i > 0) {
        return !ascending;
      }

      start = lIdx + 1;
    }
  }
}

} // anon. namespace

int Diff::Callbacks::progress(
  const git_diff *diff,
  const char *oldPath,
  const char *newPath,
  void *payload)
{
  Diff::Callbacks *cbs = reinterpret_cast<Diff::Callbacks *>(payload);
  return cbs->progress(oldPath, newPath) ? 0 : -1;
}

Diff::Data::Data(git_diff *diff)
  : diff(diff)
{
  resetMap();
}

Diff::Data::~Data()
{
  git_diff_free(diff);
}

void Diff::Data::resetMap()
{
  map.clear();
  int count = git_diff_num_deltas(diff);
  for (int i = 0; i < count; ++i)
    map.append(i);
}

const git_diff_delta *Diff::Data::delta(int index) const
{
  return git_diff_get_delta(diff, map.at(index));
}

Diff::Diff() {}

Diff::Diff(git_diff *diff)
  : d(diff ? new Data(diff) : nullptr)
{}

Diff::operator git_diff *() const
{
  return d->diff;
}

bool Diff::isConflicted() const
{
  int count = this->count();
  for (int i = 0; i < count; ++i) {
    if (status(i) == GIT_DELTA_CONFLICTED)
      return true;
  }

  return false;
}

int Diff::count() const
{
  return git_diff_num_deltas(d->diff);
}

Patch Diff::patch(int index) const
{
  git_patch *patch = nullptr;
  git_patch_from_diff(&patch, d->diff, d->map.at(index));
  return Patch(patch);
}

QString Diff::name(int index) const
{
  return d->delta(index)->new_file.path;
}

bool Diff::isBinary(int index) const
{
  return d->delta(index)->flags & GIT_DIFF_FLAG_BINARY;
}

git_delta_t Diff::status(int index) const
{
  return d->delta(index)->status;
}

Id Diff::id(int index, File file) const
{
  const git_diff_delta *delta = d->delta(index);
  return (file == NewFile) ? delta->new_file.id : delta->old_file.id;
}

int Diff::indexOf(const QString &name) const
{
  int count = this->count();
  for (int i = 0; i < count; ++i) {
    if (name == this->name(i))
      return i;
  }

  return -1;
}

void Diff::merge(const Diff &diff)
{
  git_diff_merge(d->diff, diff);
  d->resetMap();
}

void Diff::findSimilar()
{
  git_diff_find_similar(d->diff, nullptr);
  d->resetMap();
}

void Diff::sort(SortRole role, Qt::SortOrder order, const git::Index &index)
{
  Settings *settings = Settings::instance();
  bool stagedFirst = settings->value(settings->SORT_STAGED).toBool();
  bool directoryFirst = settings->value(settings->SORT_NAME_DIR).toBool();
  Qt::CaseSensitivity cs = settings->value(settings->SORT_NAME_CASE).toBool() ?
                           Qt::CaseInsensitive : Qt::CaseSensitive;
  bool ascending = (order == Qt::AscendingOrder);
  std::sort(d->map.begin(), d->map.end(),
  [this, stagedFirst, directoryFirst, cs, role, ascending, &index](int lhs, int rhs) {
    QString lhsName = git_diff_get_delta(d->diff, lhs)->new_file.path;
    QString rhsName = git_diff_get_delta(d->diff, rhs)->new_file.path;
    if (stagedFirst && index.isValid()) {
      git::Index::StagedState lhsStaged = index.isStaged(lhsName);
      git::Index::StagedState rhsStaged = index.isStaged(rhsName);
      if (lhsStaged != rhsStaged) {
        if (lhsStaged == git::Index::Staged) {
          return true;
        }
        if (rhsStaged == git::Index::Staged) {
          return false;
        }
        if (lhsStaged == git::Index::PartiallyStaged) {
          return true;
        }
        if (rhsStaged == git::Index::PartiallyStaged) {
          return false;
        }
      }
    }

    switch (role) {
      case NameRole: {
        if (directoryFirst) {
          return dirCompare(lhsName, rhsName, ascending, cs);

        } else {
          return ascending ? QString::compare(lhsName, rhsName, cs) < 0 :
                             QString::compare(rhsName, lhsName, cs) < 0;
        }
      }

      case StatusRole: {
        git_delta_t lhsStatus = git_diff_get_delta(d->diff, lhs)->status;
        git_delta_t rhsStatus = git_diff_get_delta(d->diff, rhs)->status;
        return ascending ? (lhsStatus < rhsStatus) : (rhsStatus < lhsStatus);
      }
    }
  });
}

char Diff::statusChar(git_delta_t status)
{
  if (status == GIT_DELTA_CONFLICTED)
    return '!';

  return git_diff_status_char(status);
}

} // namespace git

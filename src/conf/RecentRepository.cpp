//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RecentRepository.h"
#include "git/Repository.h"
#include "git/Config.h"

RecentRepository::RecentRepository(const QString &path, QObject *parent)
  : QObject(parent), mPath(path)
{
  git::Repository repo = git::Repository::open(mPath, false);
  // Lookup [repository] alias =
  if (repo.isValid()) {
    mAlias = repo.config().value<QString>("repository.alias");
  } else {
    mAlias.clear();
  }
}

QString RecentRepository::path() const
{
  return mPath;
}

QString RecentRepository::name() const
{
  if (!mAlias.isEmpty())
    return mAlias;

  return mPath.section('/', -mSections);
}

void RecentRepository::increment()
{
  if (mAlias.isEmpty()) {
    ++mSections;
  } else {
    mAlias.clear();
  }
}

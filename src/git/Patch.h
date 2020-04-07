//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef PATCH_H
#define PATCH_H

#include "Diff.h"
#include "FilterList.h"
#include "git2/patch.h"
#include <QBitArray>
#include <QSharedPointer>

namespace git {

class Blob;
class Id;
class Repository;

class Patch
{
public:
  enum ConflictResolution
  {
    Unresolved,
    Ours,
    Theirs
  };

  struct LineStats
  {
    int additions;
    int deletions;
  };

  Patch();

  bool isValid() const { return !d.isNull(); }

  Repository repo() const;

  QString name(Diff::File file = Diff::NewFile) const;
  git_delta_t status() const;
  bool isUntracked() const;
  bool isConflicted() const;
  bool isBinary() const;
  bool isLfsPointer() const;

  Blob blob(Diff::File file) const;

  LineStats lineStats() const;
  /*!
   * Returns the complete patch as string.
   * Used for debugging
   * \brief print
   * \return
   */
  QByteArray print() const;

  int count() const;
  QByteArray header(int index) const;

  int lineCount(int index) const;
  char lineOrigin(int index, int line) const;
  int lineNumber(int index, int line, Diff::File file = Diff::NewFile) const;
  QByteArray lineContent(int index, int line) const;

  ConflictResolution conflictResolution(int index);
  void setConflictResolution(int index, ConflictResolution resolution);

  // Apply the given hunk indexes to the old buffer.
  QByteArray apply(
    const QBitArray &hunks,
    const FilterList &filters = FilterList()) const;
  QByteArray apply(int hidx,
                   int start_line,
                   int end_line,
                   const FilterList &filters = FilterList()) const;

  static Patch fromBuffers(
    const QByteArray &oldBuffer,
    const QByteArray &newBuffer,
    const QString &oldPath = QString(),
    const QString &newPath = QString());

  static void clearConflictResolutions(const Repository &repo);

private:
  struct ConflictHunk
  {
    int line; // start line
    int min; // <<<<<<< line
    int mid; // ======= line
    int max; // >>>>>>> line
    QList<QByteArray> lines;
  };

  Patch(git_patch *patch);

  QSharedPointer<git_patch> d;
  QList<ConflictHunk> mConflicts;

  friend class Diff;
};

} // namespace git

#endif

//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MergeTool.h"
#include "git/Command.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Repository.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>

MergeTool::MergeTool(
  const QString &file,
  const git::Blob &localBlob,
  const git::Blob &remoteBlob,
  const git::Blob &baseBlob,
  QObject *parent)
  : ExternalTool(file, parent), mLocalBlob(localBlob), mRemoteBlob(remoteBlob),
    mBaseBlob(baseBlob)
{}

bool MergeTool::isValid() const
{
  return (ExternalTool::isValid() && mLocalBlob.isValid() &&
          mRemoteBlob.isValid());
}

ExternalTool::Kind MergeTool::kind() const
{
  return Merge;
}

QString MergeTool::name() const
{
  return tr("External Merge");
}

bool MergeTool::start()
{
  Q_ASSERT(isValid());

  bool shell = false;
  QString command = lookupCommand("merge", shell);
  if (command.isEmpty())
    return false;

  // Write temporary files.
  QString templatePath = QDir::temp().filePath(QFileInfo(mFile).fileName());
  QTemporaryFile *local = new QTemporaryFile(templatePath, this);
  if (!local->open())
    return false;

  local->write(mLocalBlob.content());
  local->flush();

  QTemporaryFile *remote = new QTemporaryFile(templatePath, this);
  if (!remote->open())
    return false;

  remote->write(mRemoteBlob.content());
  remote->flush();

  QString basePath;
  if (mBaseBlob.isValid()) {
    QTemporaryFile *base = new QTemporaryFile(templatePath, this);
    if (!base->open())
      return false;

    base->write(mBaseBlob.content());
    base->flush();

    basePath = base->fileName();
  }

  // Make the backup copy.
  QString backupPath = QString("%1.orig").arg(mFile);
  if (!QFile::copy(mFile, backupPath)) {
    // FIXME: What should happen if the backup already exists?
  }

  // Destroy this after process finishes.
  QProcess *process = new QProcess(this);
  git::Repository repo = mLocalBlob.repo();
  auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
  QObject::connect(process, signal, [this, repo, backupPath] {
    // FIXME: Trust exit code?
    QFileInfo merged(mFile);
    QFileInfo backup(backupPath);
    git::Config config = git::Config::global();
    bool modified = (merged.lastModified() > backup.lastModified());
    if (!modified || !config.value<bool>("mergetool.keepBackup"))
      QFile::remove(backupPath);

    if (modified) {
      int length = repo.workdir().path().length();
      repo.index().setStaged({mFile.mid(length + 1)}, true);
    }

    deleteLater();
  });

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("LOCAL", local->fileName());
  env.insert("REMOTE", remote->fileName());
  env.insert("MERGED", mFile);
  env.insert("BASE", basePath);
  process->setProcessEnvironment(env);

  QString bash = git::Command::bashPath();
  if (!bash.isEmpty()) {
    process->start(bash, {"-c", command});
  } else if (!shell) {
    process->start(git::Command::substitute(env, command));
  } else {
    emit error(BashNotFound);
    return false;
  }

  process->start(bash, {"-c", command});
  if (!process->waitForStarted())
    return false;

  // Detach from parent.
  setParent(nullptr);

  return true;
}

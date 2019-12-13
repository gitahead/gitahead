//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "EditTool.h"
#include "git/Config.h"
#include "git/Repository.h"
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

EditTool::EditTool(const QString &file, QObject *parent)
  : ExternalTool(file, parent)
{}

bool EditTool::isValid() const
{
  return (ExternalTool::isValid() && QFileInfo(mFile).isFile());
}

ExternalTool::Kind EditTool::kind() const
{
  return Edit;
}

QString EditTool::name() const
{
  return tr("Edit in External Editor");
}

bool EditTool::start()
{
  git::Config config = git::Config::global();
  QString editor = config.value<QString>("gui.editor");

  if (editor.isEmpty())
    editor = qgetenv("GIT_EDITOR");

  if (editor.isEmpty())
    editor = config.value<QString>("core.editor");

  if (editor.isEmpty())
    editor = qgetenv("VISUAL");

  if (editor.isEmpty())
    editor = qgetenv("EDITOR");

  if (editor.isEmpty())
    return QDesktopServices::openUrl(QUrl::fromLocalFile(mFile));

  // Destroy this after process finishes.
  QProcess *process = new QProcess(this);
  auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
  QObject::connect(process, signal, this, &ExternalTool::deleteLater);

  process->start(editor, {mFile});
  if (!process->waitForStarted())
    return false;

  // Detach from parent.
  setParent(nullptr);

  return true;
}

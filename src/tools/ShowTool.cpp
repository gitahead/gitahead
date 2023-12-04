//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ShowTool.h"
#include "git/Repository.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

#if defined(Q_OS_MACOS)
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Finder")
#elif defined(Q_OS_WIN)
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Explorer")
#else
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Default File Browser")
#endif

ShowTool::ShowTool(const QString &file, QObject *parent)
  : ExternalTool(file, parent)
{}

ExternalTool::Kind ShowTool::kind() const
{
  return Show;
}

QString ShowTool::name() const
{
  return tr("Show in %1").arg(tr(NAME));
}

bool ShowTool::start()
{
#if defined(Q_OS_MACOS)
  return QProcess::startDetached("/usr/bin/osascript", {
    "-e", "tell application \"Finder\"",
    "-e", QString("reveal POSIX file \"%1\"").arg(mFile),
    "-e", "activate",
    "-e", "end tell"
  });
#elif defined(Q_OS_WIN)
  return QProcess::startDetached("explorer.exe", {
    "/select,", QDir::toNativeSeparators(mFile)
  });
#else
  QFileInfo info(mFile);
  QString path = info.isDir() ? info.filePath() : info.path();
  return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

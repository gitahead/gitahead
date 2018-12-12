//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Installer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <Security/Authorization.h>

namespace {

class Auth
{
public:
  Auth(const Auth &) = delete;
  Auth &operator=(const Auth &) = delete;

  Auth()
  {
    OSStatus status = AuthorizationCreate(
      nullptr, kAuthorizationEmptyEnvironment,
      kAuthorizationFlagDefaults, &mAuth);
    mValid = (status == errAuthorizationSuccess);
  }

  ~Auth()
  {
    AuthorizationFree(mAuth, kAuthorizationFlagDefaults);
  }

  bool isValid() const { return mValid; }

  operator AuthorizationRef() const { return mAuth; }

private:
  bool mValid = false;
  AuthorizationRef mAuth;
};

bool execute(AuthorizationRef auth, const char *tool, char *const *argv)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  OSStatus status = AuthorizationExecuteWithPrivileges(
    auth, tool, kAuthorizationFlagDefaults, argv, nullptr);
#pragma clang diagnostic pop

  int stat;
  wait(&stat);

  return (status == errAuthorizationSuccess);
}

bool mkpath(AuthorizationRef auth, const QString &path)
{
  // Check if the path already exists.
  QDir dir(path);
  if (dir.exists())
    return true;

  // First try without elevating privileges.
  if (dir.mkpath("."))
    return true;

  // Shell out to mkdir -p.
  QList<QByteArray> args = {"-p", path.toUtf8()};
  char *const argv[3] = {args[0].data(), args[1].data(), nullptr};
  return execute(auth, "/bin/mkdir", argv);
}

QString text()
{
  QString path = QCoreApplication::applicationFilePath();
  return QString("#!/usr/bin/env sh\n%1 $@\n").arg(path);
}

bool write(AuthorizationRef auth, const QString &path, const QString &name)
{
  AuthorizationExternalForm extauth;
  OSStatus status = AuthorizationMakeExternalForm(auth, &extauth);
  if (status != errAuthorizationSuccess)
    return false;

  QProcess process;
  process.start("/usr/libexec/authopen",
    {"-extauth", "-c", "-w", "-m", "0755", QDir(path).filePath(name)});

  process.write(extauth.bytes, kAuthorizationExternalFormLength);
  process.write(text().toUtf8());
  process.closeWriteChannel();
  process.waitForFinished();

  return (process.exitStatus() == QProcess::NormalExit && !process.exitCode());
}

} // anon. namespace

bool Installer::isInstalled() const
{
  QFile file(QDir(mPath).filePath(mName));
  return (file.open(QFile::ReadOnly) && file.readAll() == text());
}

bool Installer::install()
{
  // Create authorization.
  Auth auth;
  if (!auth.isValid())
    return false;

  // Ensure path exists.
  if (!mkpath(auth, mPath))
    return false;

  // Write the script.
  return write(auth, mPath, mName);
}

bool Installer::uninstall()
{
  // First try without elevating privileges.
  QDir dir(mPath);
  if (dir.remove(mName))
    return true;

  Auth auth;
  if (!auth.isValid())
    return false;

  // Shell out to rm.
  QByteArray arg = dir.filePath(mName).toUtf8();
  char *const argv[2] = {arg.data(), nullptr};
  return execute(auth, "/bin/rm", argv);
}

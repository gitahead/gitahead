//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "WinCred.h"
#include <QStringList>
#include <QUrl>
#include <QVarLengthArray>
#include <windows.h>
#include <wincred.h>

// Attempt to be compatible with the git wincred helper by matching
// the target name and encoding the password in UTF-16.

namespace {

const QString kNameFmt = "%1@%2";
const QString kTargetFmt = "git:https://%1";

QString buildTarget(const QString &host, const QString &name)
{
  return kTargetFmt.arg(name.isEmpty() ? host : kNameFmt.arg(name, host));
}

QString host(const QString &url)
{
  QString host = QUrl(url).host();
  if (!host.isEmpty())
    return host;

  // Extract hostname from SSH URL.
  int end = url.indexOf(':');
  int begin = url.indexOf('@') + 1;
  return url.mid(begin, end - begin);
}

} // anon. namespace

WinCred::WinCred() {}

bool WinCred::get(
  const QString &url,
  QString &username,
  QString &password)
{
  log(QString("get: %1 %2").arg(url, username));

  PCREDENTIALW cred;
  QString target = buildTarget(host(url), username);
  QVarLengthArray<wchar_t,1024> targetBuffer(target.length() + 1);
  targetBuffer[target.toWCharArray(targetBuffer.data())] = L'\0';
  if (!CredReadW(targetBuffer.data(), CRED_TYPE_GENERIC, 0, &cred)) {
    switch (DWORD error = GetLastError()) {
      case ERROR_NOT_FOUND:
        log(QString("get: credential not found for '%1'").arg(target));
        break;

      case ERROR_NO_SUCH_LOGON_SESSION:
        log("get: no such logon session");
        break;

      case ERROR_INVALID_FLAGS:
        log("get: invalid flags");
        break;

      default:
        log(QString("get: unknown error '%1'").arg(error));
        break;
    }

    return false;
  }

  username = QString::fromWCharArray(cred->UserName);
  int size = cred->CredentialBlobSize / sizeof(ushort);
  password = QString::fromUtf16((ushort *) cred->CredentialBlob, size);

  CredFree(cred);

  return true;
}

bool WinCred::store(
  const QString &url,
  const QString &username,
  const QString &password)
{
  log(QString("store: %1 %2").arg(url, username));

  bool result = false;
  QVarLengthArray<wchar_t,1024> nameBuffer(username.length() + 1);
  nameBuffer[username.toWCharArray(nameBuffer.data())] = L'\0';
  foreach (const QString &name, QStringList({QString(), username})) {
    QString target = buildTarget(host(url), name);
    QVarLengthArray<wchar_t,1024> targetBuffer(target.length() + 1);
    targetBuffer[target.toWCharArray(targetBuffer.data())] = L'\0';

    CREDENTIALW cred;
    cred.Flags = 0;
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = targetBuffer.data();
    cred.Comment = const_cast<LPWSTR>(L"Written by GitAhead");
    cred.CredentialBlobSize = password.length() * sizeof(ushort);
    cred.CredentialBlob = (LPBYTE) password.utf16();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.AttributeCount = 0;
    cred.Attributes = nullptr;
    cred.TargetAlias = nullptr;
    cred.UserName = nameBuffer.data();
    if (CredWriteW(&cred, 0)) {
      result = true;
    } else {
      switch (DWORD error = GetLastError()) {
        case ERROR_NO_SUCH_LOGON_SESSION:
          log("store: no such logon session");
          break;

        case ERROR_INVALID_PARAMETER:
          log("store: invalid parameter");

        case ERROR_INVALID_FLAGS:
          log("store: invalid flags");
          break;

        case ERROR_BAD_USERNAME:
          log(QString("store: bad username '%1'").arg(username));
          break;

        default:
          log(QString("store: unknown error '%1'").arg(error));
          break;
      }
    }
  }

  return result;
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include "Theme.h"
#include <QApplication>

class QNetworkAccessManager;
class QNetworkReply;
class QSslError;
class QUrlQuery;

class Application : public QApplication
{
  Q_OBJECT

public:
  Application(
    int &argc,
    char **argv,
    bool haltOnParseError = false,
    const QString &defaultTheme = QString());

  void autoUpdate();
  bool restoreWindows();

  static Theme *theme();

protected:
  bool event(QEvent *event) override;

private:
  void registerService();
  void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

  QString mPathspec = QString();
  QScopedPointer<Theme> mTheme;
  QStringList mPositionalArguments;
};

#endif

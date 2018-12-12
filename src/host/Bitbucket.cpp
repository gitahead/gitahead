//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Bitbucket.h"
#include "Repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

const QString kSshFmt = "git@bitbucket.org:%1";

} // anon. namespace

Bitbucket::Bitbucket(const QString &username)
  : Account(username)
{
  QObject::connect(&mMgr, &QNetworkAccessManager::finished, this,
  [this](QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
      QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
      QJsonArray array = doc.array();
      for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array.at(i).toObject();
        if (obj.value("scm").toString() != "git")
          continue;

        QString name = obj.value("name").toString();
        QString owner = obj.value("owner").toString();
        QString fullName = QString("%1/%2").arg(owner, name);

        QUrl httpsUrl;
        httpsUrl.setHost(host());
        httpsUrl.setScheme("https");
        httpsUrl.setUserName(this->username());
        httpsUrl.setPath(QString("/%1").arg(fullName));

        Repository *repo = addRepository(name, fullName);
        repo->setUrl(Repository::Https, httpsUrl.toString());
        repo->setUrl(Repository::Ssh, kSshFmt.arg(fullName));
      }
    } else {
      mError->setText(tr("Connection failed"), reply->errorString());
    }

    mProgress->finish();
  });
}

Account::Kind Bitbucket::kind() const
{
  return Account::Bitbucket;
}

QString Bitbucket::name() const
{
  return QStringLiteral("Bitbucket");
}

QString Bitbucket::host() const
{
  return QStringLiteral("bitbucket.org");
}

void Bitbucket::connect(const QString &password)
{
  clearRepos();

  QNetworkRequest request(url() + "/user/repositories");
  if (setHeaders(request, password)) {
    mMgr.get(request);
    startProgress();
  }
}

QString Bitbucket::defaultUrl()
{
  return QStringLiteral("https://api.bitbucket.org/1.0");
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GitHub.h"
#include "Repository.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace {

const quint16 kPort = 40080;
const char *kPasswordProperty = "password";

const QString kScope = "repo";

const QString kAuthUrl =
  QStringLiteral("https://github.com/login/oauth/authorize");
const QString kAccessUrl =
  QStringLiteral("https://github.com/login/oauth/access_token");
const QString kGraphQlUrl =
  QStringLiteral("https://api.github.com/graphql");

} // anon. namespace

GitHub::GitHub(const QString &username)
  : Account(username)
{
  QObject::connect(&mMgr, &QNetworkAccessManager::finished, this,
  [this](QNetworkReply *reply) {
    QString password = reply->property(kPasswordProperty).toString();
    if (password.isEmpty())
      return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      mError->setText(tr("Connection failed"), reply->errorString());
      mProgress->finish();
      return;
    }

    // Handle repositories.
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();
    for (int i = 0; i < array.size(); ++i) {
      QJsonObject obj = array.at(i).toObject();

      // Add username to HTTPS URL.
      QUrl httpsUrl(obj.value("clone_url").toString());
      httpsUrl.setUserName(this->username());

      QString name = obj.value("name").toString();
      QString fullName = obj.value("full_name").toString();
      Repository *repo = addRepository(name, fullName);
      repo->setUrl(Repository::Https, httpsUrl.toString());
      repo->setUrl(Repository::Ssh, obj.value("ssh_url").toString());
    }

    // Check for additional pages.
    QString link = reply->rawHeader("Link");
    if (link.isEmpty()) {
      mProgress->finish();
      return;
    }

    QMap<QString,QString> map;
    QRegularExpression re("<(.*)>; rel=\"(\\w+)\"");
    foreach (const QString &record, link.split(", ")) {
      QRegularExpressionMatch match = re.match(record);
      if (match.isValid() && match.hasMatch())
        map.insert(match.captured(2), match.captured(1));
    }

    QString next = map.value("next");
    if (next.isEmpty()) {
      mProgress->finish();
      return;
    }

    // Request next page.
    QNetworkRequest request(next);
    if (setHeaders(request, password)) {
      QNetworkReply *reply = mMgr.get(request);
      reply->setProperty(kPasswordProperty, password);
      startProgress();
    }
  });

  QObject::connect(&mRedirectServer, &QTcpServer::newConnection, [this] {
    QTcpSocket *socket = mRedirectServer.nextPendingConnection();
    QObject::connect(socket, &QTcpSocket::readyRead, [this, socket] {
      // Read URL.
      QString code;
      QString status = tr("Failed to authenticate with GitHub!");
      QList<QByteArray> tokens = socket->readAll().split(' ');
      if (!tokens.isEmpty() && tokens.takeFirst() == "GET") {
        if (!tokens.isEmpty() && tokens.first().startsWith('/')) {
          QUrlQuery query(tokens.first().mid(2));
          if (mState == query.queryItemValue("state")) {
            code = query.queryItemValue("code");
            status = tr("Successfully authenticated with GitHub!");
          }
        }
      }

      // Read images.
      QString icon;
      QFile iconFile(":/Gittyup.iconset/icon_64x64.png");
      if (iconFile.open(QFile::ReadOnly))
        icon = iconFile.readAll().toBase64();

      QString name;
	  QFile nameFile(":/logo-type_light@2x.png");
      if (nameFile.open(QFile::ReadOnly))
        name = nameFile.readAll().toBase64();

      // Write HTML.
      QString content = QString(
        "<html><head><style>img { display: inline }</style></head>"
        "<body style=\"text-align: center\">"
        "<div style=\"display: inline-block; padding: 20px\">"
        "<img src=\"data:image/png;base64,%1\" />"
        "<img src=\"data:image/png;base64,%2\" style=\"padding: 10px\" />"
        "<p>%3</p><p>Close this window to continue.</p></div>"
        "</body></html>").arg(icon, name, status);

      QString message = QString(
        "HTTP/1.0 200 OK \r\n"
        "Content-Type: text/html; "
        "charset=\"utf-8\"\r\n"
        "Content-Length: %1\r\n\r\n%2").arg(content.size()).arg(content);

      socket->write(message.toUtf8());
      socket->disconnectFromHost();

      if (code.isEmpty())
        return;

      // Request access token.
      QNetworkRequest request(kAccessUrl);
      request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/x-www-form-urlencoded");

      QUrlQuery query;
      query.addQueryItem("client_id", GITHUB_CLIENT_ID);
      query.addQueryItem("client_secret", GITHUB_CLIENT_SECRET);
      query.addQueryItem("code", code);
      query.addQueryItem("state", mState);

      QNetworkReply *reply = mMgr.post(request, query.toString().toUtf8());
      QObject::connect(reply, &QNetworkReply::finished, [this, reply] {
        QUrlQuery query(reply->readAll());
        mAccessToken = query.queryItemValue("access_token");
        reply->deleteLater();
      });
    });

    QObject::connect(
      socket, &QTcpSocket::disconnected,
      socket, &QTcpSocket::deleteLater);
  });

  // Start listening for redirect.
  // FIXME: Close server after some timeout?
  mRedirectServer.listen(QHostAddress::Any, kPort);
}

Account::Kind GitHub::kind() const
{
  return Account::GitHub;
}

QString GitHub::name() const
{
  return QStringLiteral("GitHub");
}

QString GitHub::host() const
{
  return QStringLiteral("github.com");
}

void GitHub::connect(const QString &password)
{
  clearRepos();

  QString suffix = hasCustomUrl() ? "/api/v3" : QString();
  QNetworkRequest request(url() + suffix + "/user/repos");
  if (setHeaders(request, password)) {
    QNetworkReply *reply = mMgr.get(request);
    reply->setProperty(kPasswordProperty,
      !password.isEmpty() ? password : this->password());
    startProgress();
  }
}

void GitHub::requestForkParents(Repository *repo)
{
  QString query = QString(
    "query {"
    "  repository(owner:\"%1\", name:\"%2\") {"
    "    isFork"
    "    parent {"
    "      isFork"
    "      nameWithOwner"
    "      defaultBranchRef {"
    "        name"
    "      }"
    "      parent {"
    "        isFork"
    "        nameWithOwner"
    "        defaultBranchRef {"
    "          name"
    "        }"
    "        parent{"
    "          isFork"
    "          nameWithOwner"
    "          defaultBranchRef {"
    "            name"
    "          }"
    "        }"
    "      }"
    "    }"
    "  }"
    "}").arg(repo->owner(), repo->name());

  graphql(query, [this](const QJsonObject &data) {
    QMap<QString,QString> map;
    QJsonObject repository = data.value("repository").toObject();
    while (repository.value("isFork").toBool()) {
      repository = repository.value("parent").toObject();

      QString nameWithOwner = repository.value("nameWithOwner").toString();
      QString branch = repository
                       .value("defaultBranchRef").toObject()
                       .value("name").toString();

      map.insert(nameWithOwner, branch);
    }

    emit forkParentsReady(map);
  });
}

void GitHub::createPullRequest(
  Repository *repo,
  const QString &ownerRepo,
  const QString &title,
  const QString &body,
  const QString &head,
  const QString &base,
  bool canModify)
{
  QJsonDocument doc;
  doc.setObject({
    {"title", title},
    {"body", body},
    {"head", QString("%1:%2").arg(repo->owner(), head)},
    {"base", base},
    {"maintainer_can_modify", canModify}
  });

  QUrl url(QString("https://api.github.com/repos/%1/pulls").arg(ownerRepo));
  rest(url, doc, [this, title](const QJsonObject &obj) {
    foreach (const QJsonValue &error, obj.value("errors").toArray())
      emit pullRequestError(title, error.toObject().value("message").toString());
  });
}

void GitHub::requestComments(Repository *repo, const QString &oid)
{
  QString query = QString(
    "query {"
    "  repository(owner: \"%1\", name: \"%2\") {"
    "    object(oid: \"%3\") {"
    "      ... on Commit {"
    "        comments(first: 50) {"
    "          nodes {"
    "            path"
    "            position"
    "            publishedAt"
    "            body"
    "            author {"
    "              login"
    "            }"
    "          }"
    "        }"
    "      }"
    "    }"
    "  }"
    "}").arg(repo->owner(), repo->name(), oid);

  graphql(query, [this, repo, oid](const QJsonObject &data) {
    QJsonArray nodes = data
    .value("repository").toObject()
    .value("object").toObject()
    .value("comments").toObject()
    .value("nodes").toArray();

    if (nodes.isEmpty())
      return;

    CommitComments comments;
    foreach(const QJsonValue &value, nodes) {
      QJsonObject obj = value.toObject();
      QString path = obj.value("path").toString();
      int position = obj.value("position").toInt() - 1;

      QString raw = obj.value("body").toString();
      QString body = raw.trimmed().replace("\r\n", "\n");

      QJsonObject author = obj.value("author").toObject();
      QString login = author.value("login").toString();

      QString published = obj["publishedAt"].toString();
      QDateTime date = QDateTime::fromString(published, Qt::ISODate);

      Comments &map = path.isEmpty() ?
      comments.comments : comments.files[path][position];
      map.insert(date, {body, login});
    }

    emit commentsReady(repo, oid, comments);
  });
}

void GitHub::authorize()
{
  mState = QString();
  for (int i = 0; i < 32; i++) {
    int value = QRandomGenerator::global()->bounded('a', 'z' + 1);
    mState.append(QChar::fromLatin1(value));
  }

  QUrlQuery query;
  query.addQueryItem("client_id", GITHUB_CLIENT_ID);
  query.addQueryItem("scope", kScope);
  query.addQueryItem("state", mState);

  QUrl url(kAuthUrl);
  url.setQuery(query);

  // Open in default browser.
  QDesktopServices::openUrl(url);
}

bool GitHub::isAuthorizeSupported()
{
  QByteArray id(GITHUB_CLIENT_ID);
  QByteArray secret(GITHUB_CLIENT_SECRET);
  QByteArray env = qgetenv("GITTYUP_OAUTH");
  return (!id.isEmpty() && !secret.isEmpty() && !env.isEmpty());
}

QString GitHub::defaultUrl()
{
  return QStringLiteral("https://api.github.com");
}

void GitHub::graphql(
  const QString &query,
  const Callback &callback)
{
  if (mAccessToken.isEmpty())
    return;

  QJsonDocument doc;
  doc.setObject({{"query", query}});

  QNetworkRequest request(kGraphQlUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader(
    "Authorization", QString("bearer %1").arg(mAccessToken).toUtf8());

  QNetworkReply *reply = mMgr.post(request, doc.toJson());
  QObject::connect(reply, &QNetworkReply::finished, [reply, callback] {
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    callback(doc.object().value("data").toObject());
    reply->deleteLater();
  });
}

void GitHub::rest(
  const QUrl &url,
  const QJsonDocument &doc,
  const Callback &callback)
{
  if (mAccessToken.isEmpty())
    return;

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Accept", "application/vnd.github.v3+json");
  request.setRawHeader(
    "Authorization", QString("token %1").arg(mAccessToken).toUtf8());

  QNetworkReply *reply;
  if (doc.isEmpty()) {
    reply = mMgr.get(request);
  } else {
    reply = mMgr.post(request, doc.toJson());
  }

  QObject::connect(reply, &QNetworkReply::finished, [reply, callback] {
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    callback(doc.object());
    reply->deleteLater();
  });
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTE_H
#define REMOTE_H

#include "Repository.h"
#include "Result.h"
#include "git2/net.h"
#include <QSet>
#include <QSharedPointer>

struct git_cred;
struct git_oid;
struct git_remote;
struct git_transfer_progress;

namespace git {

class Id;
class Reference;

class Remote
{
public:
  struct PushUpdate {
    QByteArray srcName;
    QByteArray dstName;
    Id srcId;
    Id dstId;
  };

  class Callbacks {
  public:
    enum State {
      Transfer,
      Resolve,
      Update
    };

    Callbacks(const Repository &repo = Repository())
      : mRepo(repo)
    {}

    State state() const
    {
      return mState;
    }

    Repository repo() const
    {
      return mRepo;
    }

    virtual void sideband(
      const QString &text)
    {}

    virtual bool credentials(
      const QString &url,
      QString &username,
      QString &password)
    {
      return true;
    }

    virtual bool transfer(
      int total,
      int current,
      int bytes)
    {
      return true;
    }

    virtual bool resolve(
      int total,
      int current)
    {
      return true;
    }

    virtual void update(
      const QString &name,
      const Id &a,
      const Id &b)
    {}

    virtual void rejected(
      const QString &name,
      const QString &status)
    {}

    virtual void add(
      int total,
      int current)
    {}

    virtual void delta(
      int total,
      int current)
    {}

    // pre-push hook
    virtual bool negotiation(
      const QList<PushUpdate> &updates)
    {
      return true;
    }

    virtual bool url(QString &url)
    {
      return true;
    }

    // static callback wrappers
    static int sideband(
      const char *str,
      int len,
      void *payload);

    static int credentials(
      git_cred **out,
      const char *url,
      const char *name,
      unsigned int types,
      void *payload);

    static int certificate(
      git_cert *cert,
      int valid,
      const char *host,
      void *payload);

    static int transfer(
      const git_transfer_progress *stats,
      void *payload);

    static int update(
      const char *name,
      const git_oid *a,
      const git_oid *b,
      void *payload);

    static int url(
      git_buf *out,
      git_remote *remote,
      git_direction direction,
      void *payload);

  protected:
    Repository mRepo;
    State mState = Transfer;
    QSet<QString> mAgentNames;
  };

  Remote();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  QString name() const;
  void setName(const QString &name);

  QString url() const;
  void setUrl(const QString &url);

  Result fetch(Callbacks *callbacks, bool tags = false);
  Result push(Callbacks *callbacks, const QStringList &refspecs);
  Result push(
    Callbacks *callbacks,
    const Reference &src,
    const QString &dst = QString(),
    bool force = false,
    bool tags = false);

  // Cancel the current operation.
  void stop();

  static Result clone(
    Callbacks *callbacks,
    const QString &url,
    const QString &path,
    bool bare = false);

  static QString proxyUrl(const QString &url);

  static bool isLoggingEnabled();
  static void setLoggingEnabled(bool enabled);
  static void log(const QString &text);

private:
  Remote(git_remote *remote);

  QSharedPointer<git_remote> d;

  friend class Branch;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Remote);

#endif

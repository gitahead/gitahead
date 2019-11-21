//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Remote.h"
#include "Branch.h"
#include "Config.h"
#include "Id.h"
#include "TagRef.h"
#include "conf/Settings.h"
#include "git2/buffer.h"
#include "git2/clone.h"
#include "git2/remote.h"
#include "git2/signature.h"
#include <cstdlib>
#include <libssh2.h>
#include <QDir>
#include <QFile>
#include <QNetworkProxyFactory>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>
#include <QTime>
#include <QUrl>
#include <QVector>

namespace git {

namespace {

const QString kLogKey = "remote/log";
const QStringList kKeyKinds = {"ed25519", "rsa", "dsa"};

QString keyFile()
{
  QDir dir = QDir::home();
  if (!dir.cd(".ssh"))
    return "";

  foreach (const QString &kind, kKeyKinds) {
    QString name = QString("id_%1").arg(kind);
    if (dir.exists(name))
      return dir.absoluteFilePath(name);
  }

  return "";
}

QString hostName(const QString &url)
{
  QUrl canonical(url);
  if (canonical.scheme() == "ssh")
    return canonical.host();

  int end = url.indexOf(':');
  int begin = url.indexOf('@') + 1;
  return url.mid(begin, end - begin);
}

int pack_progress(
  int stage,
  unsigned int current,
  unsigned int total,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  if (stage == GIT_PACKBUILDER_ADDING_OBJECTS) {
    cbs->add(total, current);
  } else if (stage == GIT_PACKBUILDER_DELTAFICATION) {
    cbs->delta(total, current);
  }

  return 0;
}

int push_transfer_progress(
  unsigned int current,
  unsigned int total,
  size_t bytes,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  if (cbs->state() == Remote::Callbacks::Transfer) {
    if (!cbs->transfer(total, current, bytes))
      return -1;
  }

  return 0;
}

int push_update_reference(
  const char *name,
  const char *status,
  void *payload)
{
  if (status)
    reinterpret_cast<Remote::Callbacks *>(payload)->rejected(name, status);
  return 0;
}

int push_negotiation(
  const git_push_update **updates,
  size_t len,
  void *payload)
{
  QList<Remote::PushUpdate> list;
  for (int i = 0; i < len; ++i) {
    const git_push_update *update = updates[i];
    list.append({
      update->src_refname,
      update->dst_refname,
      update->src,
      update->dst
    });
  }

  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  return cbs->negotiation(list) ? 0 : -1;
}

class ConfigFile
{
public:
  struct Host
  {
    QString file;
    QString hostName;
    QStringList patterns;
  };

  ConfigFile()
  {
    QDir dir = QDir::home();
    if (!dir.cd(".ssh"))
      return;

    mFile.reset(new QFile(dir.filePath("config")));
    if (!mFile->open(QFile::ReadOnly))
      return;

    mValid = true;
  }

  bool isValid() const { return mValid; }

  QList<Host> parse()
  {
    Q_ASSERT(isValid());

    QList<Host> hosts;
    QTextStream in(mFile.data());
    while (!in.atEnd()) {
      // Skip comments and empty lines.
      QString line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#'))
        continue;

      // Split into keyword and arguments.
      // FIXME: Doesn't handle quoted arguments.
      QRegularExpression re("(\\s+|\\s*=\\s*)");
      QStringList words = line.split(re);
      QString keyword = words.takeFirst().toLower();
      if (keyword == "match") {
        hosts.append(Host());
      } else if (keyword == "host") {
        hosts.append({QString(), QString(), words});
      } else if (keyword == "hostname") {
        if (!hosts.isEmpty() && !words.isEmpty())
          hosts.last().hostName = words.first();
      } else if (keyword == "identityfile") {
        if (!hosts.isEmpty() && !words.isEmpty()) {
          QString file = words.first();
          hosts.last().file = file.replace('~', QDir::homePath());
        }
      }
    }

    return hosts;
  }

private:
  bool mValid = false;
  QScopedPointer<QFile> mFile;
};

} // anon. namespace

int Remote::Callbacks::sideband(
  const char *str,
  int len,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  cbs->sideband(QString::fromUtf8(str, len));
  return 0;
}

static void interactiveCallback(
  const char *name,
  int name_len,
  const char *instruction,
  int instruction_len,
  int num_prompts,
  const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
  LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
  void **abstract
)
{
  if (num_prompts == 0)
    return;

  Remote::Callbacks *cbs = (Remote::Callbacks*)*abstract;

  QVector<Remote::SshInteractivePrompt> promptsVector(num_prompts);
  QVector<QString> responsesVector(num_prompts);

  for (int i = 0; i < num_prompts; ++i) {
    promptsVector[i] = {
      QString::fromUtf8(prompts[i].text, prompts[i].length),
      (bool)prompts[i].echo
    };
  }

  cbs->interactiveAuth(
    QString::fromUtf8(name, name_len),
    QString::fromUtf8(instruction, instruction_len),
    promptsVector,
    responsesVector
  );

  if (responsesVector.length() < num_prompts)
    responsesVector.resize(num_prompts);

  for (int i = 0; i < num_prompts; ++i) {
    if (responsesVector[i].isEmpty()) {
      responses[i].text = nullptr;
      responses[i].length = 0;

    } else {
      QByteArray bytes = responsesVector[i].toUtf8();

      // Use malloc and copy the response data to it
      // libssh2 free()s this memory by itself
      responses[i].length = bytes.size() - 1;
      responses[i].text = (char*)malloc(responses[i].length);

      memcpy(responses[i].text, bytes.data(), responses[i].length);
    }
  }
}

int Remote::Callbacks::credentials(
  git_cred **out,
  const char *url,
  const char *name,
  unsigned int types,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  if (types & GIT_CREDTYPE_SSH_KEY) {
    // First try to get key from agent.
    if (!cbs->mAgentNames.contains(name)) {
      log(QString("agent: %1").arg(name));
      cbs->mAgentNames.insert(name);
      LIBSSH2_SESSION *session = libssh2_session_init();
      LIBSSH2_AGENT *agent = libssh2_agent_init(session);
      int error = libssh2_agent_connect(agent);
      if (error != LIBSSH2_ERROR_NONE) {
        char *msg;
        libssh2_session_last_error(session, &msg, nullptr, 0);
        log(QString("agent: %1 (%2)").arg(msg).arg(error));
      }

      libssh2_agent_disconnect(agent);
      libssh2_agent_free(agent);
      libssh2_session_free(session);
      if (error == LIBSSH2_ERROR_NONE)
        return git_cred_ssh_key_from_agent(out, name);
    }

    // Read SSH config file.
    QString key;
    ConfigFile configFile;
    if (configFile.isValid()) {
      // Extract hostname from the unresolved URL.
      QString name = hostName(cbs->url());
      foreach (const ConfigFile::Host &host, configFile.parse()) {
        // Skip entries that don't have an identity file.
        if (host.file.isEmpty() || cbs->mKeyFiles.contains(host.file))
          continue;

        foreach (const QString &pattern, host.patterns) {
          QRegExp re(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
          if (re.exactMatch(name)) {
            key = host.file;
            break;
          }
        }

        if (!key.isEmpty())
          break;
      }
    }

    if (key.isEmpty()) {
      key = keyFile();

      if (cbs->mKeyFiles.contains(key))
        key = "";
    }

    // Search for default keys.
    if (!key.isEmpty()) {
      cbs->mKeyFiles.insert(key);

      if (!QFile::exists(key)) {
        QString err = QString("identity file not found: %1").arg(key);
        git_error_set_str(GIT_ERROR_NET, err.toUtf8());
        return -1;
      }

      QString pub = QString("%1.pub").arg(key);
      if (!QFile::exists(pub))
        pub = QString();

      // Check if the private key is encrypted.
      QFile file(key);
      if (!file.open(QFile::ReadOnly)) {
        git_error_set_str(GIT_ERROR_NET, "failed to open SSH identity file");
        return -1;
      }

      QTextStream in(&file);
      in.readLine(); // -----BEGIN PRIVATE KEY-----
      QString line = in.readLine();
      if (!line.startsWith("Proc-Type:") || !line.endsWith("ENCRYPTED")) {
        QByteArray base64 = QByteArray::fromBase64(line.toLocal8Bit());
        if (!base64.contains("aes256-ctr") || !base64.contains("bcrypt"))
          return git_cred_ssh_key_new(out, name,
            !pub.isEmpty() ? pub.toLocal8Bit().constData() : nullptr,
            key.toLocal8Bit(), nullptr);
      }

      // Prompt for passphrase to decrypt key.
      QString passphrase;
      QString username = name;
      if (!cbs->credentials(url, username, passphrase))
        return -1;

      return git_cred_ssh_key_new(out, username.toUtf8(),
        !pub.isEmpty() ? pub.toLocal8Bit().constData() : nullptr,
        key.toLocal8Bit(), passphrase.toUtf8());
    }
  }

  if (types & GIT_CREDTYPE_USERPASS_PLAINTEXT) {
    QString password;
    QString username = QUrl::fromPercentEncoding(name);
    if (!cbs->credentials(url, username, password))
      return -1;

    return git_cred_userpass_plaintext_new(
      out, username.toUtf8(), password.toUtf8());
  }

  if (types & GIT_CREDTYPE_SSH_INTERACTIVE) {
    return git_cred_ssh_interactive_new(
      out,
      name,
      interactiveCallback,
      cbs
    );
  }

  return -1;
}

int Remote::Callbacks::certificate(
  git_cert *cert,
  int valid,
  const char *host,
  void *payload)
{
  if (valid || cert->cert_type == GIT_CERT_HOSTKEY_LIBSSH2)
    return 0;

  int noVerify = 0;
  QByteArray env = qgetenv("GIT_SSL_NO_VERIFY");
  if (!env.isNull() && !git_config_parse_bool(&noVerify, env) && noVerify)
    return 0;

  Repository repo = reinterpret_cast<Remote::Callbacks *>(payload)->repo();
  Config config = repo.isValid() ? repo.config() : Config::global();
  if (!config.value<bool>("http.sslVerify", true))
    return 0;

  git_error_set_str(GIT_ERROR_SSL, "invalid certificate");
  return -1;
}

int Remote::Callbacks::transfer(
  const git_indexer_progress *stats,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  switch (cbs->state()) {
    case Transfer:
      return cbs->transfer(stats->total_objects, stats->received_objects,
                           stats->received_bytes) ? 0 : -1;

    case Resolve:
      return cbs->resolve(stats->total_deltas, stats->indexed_deltas) ? 0 : -1;

    default:
      return 0;
  }
}

int Remote::Callbacks::update(
  const char *name,
  const git_oid *a,
  const git_oid *b,
  void *payload)
{
  reinterpret_cast<Remote::Callbacks *>(payload)->update(name, a, b);
  return 0;
}

int Remote::Callbacks::url(
  git_buf *out,
  const char *url,
  int direction,
  void *payload)
{
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  QString resolved(url);
  if (!cbs->url(resolved))
    return -1;

  // Extract hostname from SSH URL.
  QString hostName;
  int end = resolved.indexOf(':');
  int begin = resolved.indexOf('@') + 1;
  bool sshUrl = (begin >= 0 && end >= 0 && begin < end);
  if (sshUrl) {
    hostName = resolved.mid(begin, end - begin);
  } else {
    QUrl tmp(resolved);
    if (tmp.scheme() == "ssh")
      hostName = tmp.host();
  }

  // Find matching config entry.
  if (!hostName.isEmpty()) {
    ConfigFile configFile;
    if (configFile.isValid()) {
      foreach (const ConfigFile::Host &host, configFile.parse()) {
        // Skip about entries that don't have a host name.
        if (host.hostName.isEmpty())
          continue;

        QString replacement;
        foreach (const QString &pattern, host.patterns) {
          QRegExp re(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
          if (re.exactMatch(hostName)) {
            replacement = host.hostName;
            break;
          }
        }

        // Replace host name.
        if (!replacement.isEmpty()) {
          if (sshUrl) {
            resolved.replace(begin, end - begin, replacement);
          } else {
            QUrl tmp(resolved);
            tmp.setHost(replacement);
            resolved = tmp.toString();
          }

          break;
        }
      }
    }
  }

  git_buf_set(out, resolved.toUtf8(), resolved.length());
  return 0;
}

Remote::Remote() {}

Remote::Remote(git_remote *remote)
  : d(remote, git_remote_free)
{}

QString Remote::name() const
{
  QString name = git_remote_name(d.data());
  return !name.isEmpty() ? name : url();
}

void Remote::setName(const QString &name)
{
  git_strarray problems;
  const char *current = git_remote_name(d.data());
  git_repository *repo = git_remote_owner(d.data());
  if (git_remote_rename(&problems, repo, current, name.toUtf8()))
    return;

  // FIXME: Report problems?
  git_strarray_free(&problems);
}

QString Remote::url() const
{
  return git_remote_url(d.data());
}

void Remote::setUrl(const QString &url)
{
  git_repository *repo = git_remote_owner(d.data());
  git_remote_set_url(repo, git_remote_name(d.data()), url.toUtf8());
}

Result Remote::fetch(Callbacks *callbacks, bool tags)
{
  git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
  opts.callbacks.sideband_progress = &Remote::Callbacks::sideband;
  opts.callbacks.credentials = &Remote::Callbacks::credentials;
  opts.callbacks.certificate_check = &Remote::Callbacks::certificate;
  opts.callbacks.transfer_progress = &Remote::Callbacks::transfer;
  opts.callbacks.update_tips = &Remote::Callbacks::update;
  opts.callbacks.resolve_url = &Remote::Callbacks::url;
  opts.callbacks.payload = callbacks;

  QByteArray proxy = proxyUrl(url(), opts.proxy_opts.type);
  opts.proxy_opts.url = proxy;

  if (tags)
    opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_ALL;

  // Write reflog message.
  QString msg = QString("fetch: %1").arg(name());

  return git_remote_fetch(d.data(), nullptr, &opts, msg.toUtf8());
}

Result Remote::push(Callbacks *callbacks, const QStringList &refspecs)
{
  git_push_options opts = GIT_PUSH_OPTIONS_INIT;
  opts.callbacks.sideband_progress = &Remote::Callbacks::sideband;
  opts.callbacks.credentials = &Remote::Callbacks::credentials;
  opts.callbacks.certificate_check = &Remote::Callbacks::certificate;
  opts.callbacks.transfer_progress = &Remote::Callbacks::transfer;
  opts.callbacks.update_tips = &Remote::Callbacks::update;
  opts.callbacks.resolve_url = &Remote::Callbacks::url;
  opts.callbacks.pack_progress = &pack_progress;
  opts.callbacks.push_transfer_progress = &push_transfer_progress;
  opts.callbacks.push_update_reference = &push_update_reference;
  opts.callbacks.push_negotiation = &push_negotiation;
  opts.callbacks.payload = callbacks;

  QByteArray proxy = proxyUrl(url(), opts.proxy_opts.type);
  opts.proxy_opts.url = proxy;

  QVector<char *> raw;
  QVector<QByteArray> storage;
  foreach (const QString &refspec, refspecs) {
    storage.append(refspec.toUtf8());
    raw.append(storage.last().data());
  }

  git_strarray array;
  array.strings = raw.data();
  array.count = raw.size();
  return git_remote_push(d.data(), &array, &opts);
}

Result Remote::push(
  Callbacks *callbacks,
  const Reference &src,
  const QString &dst,
  bool force,
  bool tags)
{
  Repository repo(git_remote_owner(d.data()));
  QString prefix = force ? "+" : QString();
  QString refspec = prefix + src.qualifiedName();
  if (!dst.isEmpty()) {
    refspec += ":" + dst;
  } else {
    QString key = QString("branch.%1.merge").arg(src.name());
    QString upstream = repo.config().value<QString>(key);
    if (!upstream.isEmpty())
      refspec += ":" + upstream;
  }

  QStringList refspecs(refspec);
  if (tags) {
    // Add tags individually. Wildcard push refspecs aren't supported yet.
    foreach (const TagRef &tag, repo.tags())
      refspecs.append(prefix + tag.qualifiedName());
  }

  return push(callbacks, refspecs);
}

void Remote::stop()
{
  git_remote_stop(d.data());
}

Result Remote::clone(
  Callbacks *callbacks,
  const QString &url,
  const QString &path,
  bool bare)
{
  git_repository *repo = nullptr;
  git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
  opts.fetch_opts.callbacks.sideband_progress = &Remote::Callbacks::sideband;
  opts.fetch_opts.callbacks.credentials = &Remote::Callbacks::credentials;
  opts.fetch_opts.callbacks.certificate_check = &Remote::Callbacks::certificate;
  opts.fetch_opts.callbacks.transfer_progress = &Remote::Callbacks::transfer;
  opts.fetch_opts.callbacks.update_tips = &Remote::Callbacks::update;
  opts.fetch_opts.callbacks.resolve_url = &Remote::Callbacks::url;
  opts.fetch_opts.callbacks.payload = callbacks;
  opts.bare = bare;

  QByteArray proxy = proxyUrl(url, opts.fetch_opts.proxy_opts.type);
  opts.fetch_opts.proxy_opts.url = proxy;

  return git_clone(&repo, url.toUtf8(), path.toUtf8(), &opts);
}

QByteArray Remote::proxyUrl(const QString &url, git_proxy_t &type)
{
  type = GIT_PROXY_AUTO;

  QUrl tmp(url);
  QNetworkProxyQuery query(tmp);
  QList<QNetworkProxy> proxies =
    QNetworkProxyFactory::systemProxyForQuery(query);
  if (proxies.isEmpty())
    return QByteArray();

  QNetworkProxy proxy = proxies.first();
  if (proxy.type() == QNetworkProxy::NoProxy)
    type = GIT_PROXY_NONE;

  QString host = proxy.hostName();
  if (host.isEmpty())
    return QByteArray();

  type = GIT_PROXY_SPECIFIED;
  return QString("http://%1:%2").arg(host).arg(proxy.port()).toUtf8();
}

bool Remote::isLoggingEnabled()
{
  return QSettings().value(kLogKey).toBool();
}

void Remote::setLoggingEnabled(bool enable)
{
  QSettings().setValue(kLogKey, enable);
}

void Remote::log(const QString &text)
{
  if (!isLoggingEnabled())
    return;

  QFile file(Settings::tempDir().filePath("remote.log"));
  if (!file.open(QFile::WriteOnly | QIODevice::Append))
    return;

  QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
  QTextStream(&file) << time << " - " << text << endl;
}

} // namespace git

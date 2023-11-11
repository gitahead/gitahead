//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CloneDialog.h"
#include "git/Remote.h"
#include "git/Result.h"
#include "git/Submodule.h"
#include "log/LogEntry.h"
#include "log/LogView.h"
#include "ui/ExpandButton.h"
#include "ui/RemoteCallbacks.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QtConcurrent>

namespace {

const QString kPathKey = "repo/path";

class RemotePage : public QWizardPage
{
  Q_OBJECT

public:
  RemotePage(Repository *repo, QWidget *parent = nullptr)
    : QWizardPage(parent)
  {
    setTitle(tr("Remote Repository URL"));
    setSubTitle(repo ?
      tr("Choose protocol to authenticate with the remote.") :
      tr("Enter the URL of the remote repository or browse for a local "
         "directory"));

    if (repo) {
      mProtocol = new QComboBox(this);
      mProtocol->addItem("HTTPS", Repository::Https);
      mProtocol->addItem("SSH", Repository::Ssh);

      // Reset URL when the protocol changes.
      using Signal = void (QComboBox::*)(int);
      auto signal = static_cast<Signal>(&QComboBox::activated);
      connect(mProtocol, signal, [this, repo] {
        int protocol = mProtocol->currentData().toInt();
        mUrl->setText(repo->url(static_cast<Repository::Protocol>(protocol)));
      });
    }

    QFrame *url = new QFrame(this);
    mUrl = new QLineEdit(url);
    mUrl->setMinimumWidth(mUrl->sizeHint().width() * 2);
    connect(mUrl, &QLineEdit::textChanged,
            this, &RemotePage::completeChanged);

    QPushButton *browse = nullptr;
    if (repo) {
      mUrl->setReadOnly(true);
      mUrl->setText(repo->url(Repository::Https));
    } else {
      browse = new QPushButton(tr("..."), url);
      connect(browse, &QPushButton::clicked, [this]() {
        QString title = tr("Choose Directory");
        QFileDialog *dialog = new QFileDialog(this, title, mUrl->text(), QString());
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOption(QFileDialog::ShowDirsOnly);
        connect(dialog, &QFileDialog::fileSelected, [this](const QString &file) {
          mUrl->setText(file);
        });

        dialog->open();
      });
    }

    QHBoxLayout *urlLayout = new QHBoxLayout(url);
    urlLayout->setContentsMargins(0,0,0,0);
    urlLayout->addWidget(mUrl);
    if (browse)
      urlLayout->addWidget(browse);

    QLabel *label = nullptr;
    if (!repo) {
      label = new QLabel(
        tr("Examples of valid URLs include:<table cellspacing='8'>"
           "<tr><td align='right'><b>HTTPS</b></td>"
           "<td>https://hostname/path/to/repo.git</td></tr>"
           "<tr><td align='right'><b>SSH</b></td>"
           "<td>git@hostname:path/to/repo.git</td></tr>"
           "<tr><td align='right'><b>Git</b></td>"
           "<td>git://hostname/path/to/repo.git</td></tr>"
           "<tr><td align='right'><b>Local</b></td>"
           "<td>/path/to/repo, C:\\path\\to\\repo</td></tr></table>"), this);
    }

    QFormLayout *form = new QFormLayout(this);
    if (mProtocol)
      form->addRow(tr("Protocol:"), mProtocol);
    form->addRow(tr("URL:"), url);
    if (label)
      form->addRow(label);

    // Register field.
    registerField("url", mUrl);
  }

  bool isComplete() const override
  {
    return !mUrl->text().isEmpty();
  }

private:
  QLineEdit *mUrl;
  QComboBox *mProtocol = nullptr;
};

class LocationPage : public QWizardPage
{
  Q_OBJECT

public:
  LocationPage(
    CloneDialog::Kind kind,
    QWidget *parent = nullptr,
    const QString &parentPath = QString())
    : QWizardPage(parent), mInit(kind == CloneDialog::Init), mParentPath(parentPath)
  {
    setTitle(tr("Repository Location"));
    setSubTitle(
      tr("Choose the name and location of the new repository. A new "
         "directory will be created if it doesn't already exist."));
    setButtonText(mInit ? QWizard::FinishButton : QWizard::NextButton,
                  mInit ? tr("Initialize") : tr("Clone"));

    mName = new QLineEdit(this);
    mName->setMinimumWidth(mName->sizeHint().width() * 2);
    connect(mName, &QLineEdit::textChanged, this, &LocationPage::updateLabel);

    QFrame *path = new QFrame(this);
    mPath = new QLineEdit(path);
    mPath->setMinimumWidth(mPath->sizeHint().width() * 2);
    connect(mPath, &QLineEdit::textChanged, this, &LocationPage::updateLabel);

    QPushButton *browse = new QPushButton(tr("..."), path);
    connect(browse, &QPushButton::clicked, [this]() {
      QString title = tr("Choose Directory");
      QFileDialog *dialog = new QFileDialog(this, title, mPath->text(), QString());
      dialog->setAttribute(Qt::WA_DeleteOnClose);
      dialog->setFileMode(QFileDialog::Directory);
      dialog->setOption(QFileDialog::ShowDirsOnly);
      connect(dialog, &QFileDialog::fileSelected, [this](const QString &file) {
        mPath->setText(file);
      });

      dialog->open();
    });

    QHBoxLayout *pathLayout = new QHBoxLayout(path);
    pathLayout->setContentsMargins(0,0,0,0);
    pathLayout->addWidget(mPath);
    pathLayout->addWidget(browse);

    // Disable advanced (bare repo) setting in Submodule mode.
    bool enableAdvanced = (kind != CloneDialog::Submodule);

    ExpandButton *expand = new ExpandButton(this);
    expand->setVisible(enableAdvanced);
    QWidget *advanced = new QWidget(this);
    advanced->setVisible(false);

    QFormLayout *form = new QFormLayout;
    form->setFormAlignment(Qt::AlignLeft);
    form->addRow(tr("Name:"), mName);
    form->addRow(tr("Directory:"), path);
    form->addRow(enableAdvanced ? tr("Advanced:") : QString(), expand);

    QCheckBox *bare = new QCheckBox(tr("Create a bare repository"));

    QFormLayout *advancedForm = new QFormLayout(advanced);
    advancedForm->setContentsMargins(-1,0,0,0);
    advancedForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    advancedForm->addRow(bare);

    connect(expand, &ExpandButton::toggled, [this, advanced](bool checked) {
      advanced->setVisible(checked);
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      resize(sizeHint());
    });

    mLabel = new QLabel(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(advanced);
    layout->addWidget(mLabel);

    // Register fields.
    registerField("name", mName);
    registerField("path", mPath);
    registerField("bare", bare);
  }

  bool isComplete() const override
  {
    QString path = mPath->text();
    return (!mName->text().isEmpty() && !path.isEmpty() &&
             verifyPath(path) && QDir(path).exists());
  }

  int nextId() const override
  {
    return mInit ? -1 : QWizardPage::nextId();
  }

  void initializePage() override
  {
    QUrl url(field("url").toString());
    QString name = QFileInfo(url.path()).fileName();
    mName->setText(name.endsWith(".git") ? name.chopped(4) : name);
    mPath->setText(defaultPath());
  }

private:
  void updateLabel()
  {
    QString fmt =
      tr("The new repository will be created at:"
         "<p style='text-indent: 12px'><b>%1</b></p>");

    QString name = mName->text();
    QString path = mPath->text();
    mLabel->setText(fmt.arg(QDir(path).filePath(name)));
    mLabel->setVisible(!name.isEmpty() && !path.isEmpty() && verifyPath(path));

    emit completeChanged();
  }

  QString defaultPath() const
  {
    if (!mParentPath.isEmpty())
      return mParentPath;
    else
      return QSettings().value(kPathKey, QDir::homePath()).toString();
  }

  bool verifyPath(const QString &path) const
  {
    // For submodules, its submodule repository must be cloned into the
    // subdirectory under the parent repository.
    return (mParentPath.isEmpty() || path.startsWith(mParentPath));
  }

  bool mInit;
  QString mParentPath;
  QLineEdit *mName;
  QLineEdit *mPath;
  QLabel *mLabel;
};

class ClonePage : public QWizardPage
{
  Q_OBJECT

public:
  ClonePage(QWidget *parent = nullptr)
    : QWizardPage(parent)
  {
    setTitle(tr("Clone Progress"));
    setSubTitle(tr("The new repository will open after the clone finishes."));

    mLogRoot = new LogEntry(this);
    mLogView = new LogView(mLogRoot, this);
    connect(mLogView, &LogView::operationCanceled, this, &ClonePage::cancel);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mLogView);
  }

  bool isComplete() const override
  {
    return (!mWatcher || mWatcher->isFinished());
  }

  void initializePage() override
  {
    QString url = field("url").toString().trimmed();
    QString name = field("name").toString().trimmed();
    QString path = QDir(field("path").toString().trimmed()).filePath(name);
    bool bare = field("bare").toBool();
    LogEntry *entry = mLogRoot->addEntry(url, tr("Clone"));

    mWatcher = new QFutureWatcher<git::Result>(this);
    connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
    [this, path, entry] {
      entry->setBusy(false);

      git::Result result = mWatcher->result();
      if (mCallbacks->isCanceled()) {
        error(entry, tr("clone"), path, tr("Clone canceled."));
      } else if (!result) {
        error(entry, tr("clone"), path, result.errorString());
      } else {
        mCallbacks->storeDeferredCredentials();
        emit completeChanged();
      }

      mWatcher->deleteLater();

      mWatcher = nullptr;
      mCallbacks = nullptr;
    });

    mCallbacks = new RemoteCallbacks(
      RemoteCallbacks::Receive, entry, url, "origin", mWatcher);

    entry->setBusy(true);
    mWatcher->setFuture(clone_run(mCallbacks, url, path, bare));
  }

  void cleanupPage() override
  {
    cancel();
  }

private:
  virtual QFuture<git::Result> clone_run(
    RemoteCallbacks *callbacks,
    const QString &url,
    const QString &path,
    bool bare)
  {
    return QtConcurrent::run(&git::Remote::clone, callbacks, url, path, bare);
  }

  void cancel()
  {
    // Signal the asynchronous transfer to cancel itself.
    // Wait for it to finish before leaving this page.
    if (mWatcher && mWatcher->isRunning()) {
      mCallbacks->setCanceled(true);
      mWatcher->waitForFinished();
    }
  }

  void error(
    LogEntry *entry,
    const QString &action,
    const QString &name,
    const QString &defaultError)
  {
    QString text = tr("Failed to %1 into '%2' - %3");
    QString detail = git::Repository::lastError(defaultError);
    entry->addEntry(LogEntry::Error, text.arg(action, name, detail));
  }

  LogEntry *mLogRoot;
  LogView *mLogView;

  RemoteCallbacks *mCallbacks = nullptr;
  QFutureWatcher<git::Result> *mWatcher = nullptr;
};

class CloneSubmodulePage : public ClonePage
{
  Q_OBJECT

public:
  CloneSubmodulePage(const git::Repository &repo, QWidget *parent = nullptr)
    : ClonePage(parent), mRepo(repo)
  {
    Q_ASSERT(repo.isValid());
    setSubTitle(tr("The new submodule will be added after the clone finishes."));

    connect(this, &CloneSubmodulePage::completeChanged, [this] {
      if (isComplete() && mSubmodule.isValid())
        mRepo.addSubmoduleFinalize(mSubmodule);
    });
  }

private:
  virtual QFuture<git::Result> clone_run(
    RemoteCallbacks *callbacks,
    const QString &url,
    const QString &path,
    bool bare) override
  {
    if (!mSubmodule.isValid())
      mSubmodule = mRepo.addSubmoduleSetup(path, url);

    return QtConcurrent::run(&git::Remote::clone, callbacks, mSubmodule);
  }

  git::Repository mRepo;
  git::Submodule mSubmodule = git::Submodule();
};

} // anon. namespace

CloneDialog::CloneDialog(
  Kind kind,
  QWidget *parent,
  Repository *repo,
  const git::Repository &parentRepo)
  : QWizard(parent), mKind(kind)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(windowTitle());
  setOptions(QWizard::NoBackButtonOnStartPage | QWizard::CancelButtonOnLeft);
  setWizardStyle(QWizard::ModernStyle);

  QString parentPath = parentRepo.isValid() ?
    parentRepo.workdir().absolutePath() : QString();

  ClonePage *clonePage;
  if (kind == Submodule)
    clonePage = new CloneSubmodulePage(parentRepo, parent);
  else
    clonePage = new ClonePage(parent);

  addPage(new RemotePage(repo, this));
  int location = addPage(new LocationPage(kind, this, parentPath));
  int clone = addPage(clonePage);

  connect(clonePage, &ClonePage::completeChanged, [this, clonePage] {
    if (clonePage->isComplete())
      accept();
  });

  if (kind == Init)
    setStartId(location);
}

void CloneDialog::accept()
{
  if (mKind == Submodule) {
    QDialog::accept();
    return;
  }

  QString path = this->path();
  bool bare = field("bare").toBool();
  if (git::Repository::open(path).isValid() ||
      git::Repository::init(path, bare).isValid()) {
    QSettings().setValue(kPathKey, field("path"));
    QDialog::accept();
  }

  // FIXME: Report error.
}

QString CloneDialog::path() const
{
  return QDir(field("path").toString()).filePath(field("name").toString());
}

QString CloneDialog::message() const
{
  QString url = field("url").toString();
  return url.isEmpty() ?
    tr("Initialized empty repository into '%1'").arg(path()) :
    tr("Cloned repository from '%1' into '%2'").arg(url, path());
}

QString CloneDialog::messageTitle() const
{
  QString url = field("url").toString();
  return url.isEmpty() ? tr("Initialize") : tr("Clone");
}

QString CloneDialog::windowTitle() const
{
  if (mKind == Init)
    return tr("Initialize Repository");
  else if (mKind == Clone)
    return tr("Clone Repository");
  else
    return tr("Add Submodule");
}

#include "CloneDialog.moc"

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Application.h"
#include "conf/Settings.h"
#include "git/Id.h"
#include "git/Repository.h"
#include "ui/MainWindow.h"
#include "ui/MenuBar.h"
#include "ui/RepoView.h"
#include "update/Updater.h"
#include <QCloseEvent>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMessageBox>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
#include <QTranslator>

#if defined(Q_OS_MACOS)
#include <unistd.h>

#elif defined(Q_OS_WIN)
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>

static LPTOP_LEVEL_EXCEPTION_FILTER defaultFilter = nullptr;

static LONG WINAPI exceptionFilter(PEXCEPTION_POINTERS info)
{
  // Protect against reentering.
  static bool entered = false;
  if (entered)
    ExitProcess(1);
  entered = true;

  // Write dump file.
  SYSTEMTIME localTime;
  GetLocalTime(&localTime);

  wchar_t temp[MAX_PATH];
  GetTempPathW(MAX_PATH, temp);

  wchar_t dir[MAX_PATH];
  StringCchPrintfW(dir, MAX_PATH, L"%sGitAhead", temp);
  CreateDirectoryW(dir, NULL);

  wchar_t fileName[MAX_PATH];
  StringCchPrintfW(fileName, MAX_PATH,
    L"%s\\%s-%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
    dir, GITAHEAD_NAME, GITAHEAD_VERSION,
    localTime.wYear, localTime.wMonth, localTime.wDay,
    localTime.wHour, localTime.wMinute, localTime.wSecond,
    GetCurrentProcessId(), GetCurrentThreadId());

  HANDLE dumpFile = CreateFileW(fileName, GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  MINIDUMP_EXCEPTION_INFORMATION expParam;
  expParam.ThreadId = GetCurrentThreadId();
  expParam.ExceptionPointers = info;
  expParam.ClientPointers = TRUE;

  MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
    dumpFile, MiniDumpWithDataSegs, &expParam, NULL, NULL);

  return defaultFilter ? defaultFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}
#endif

Application::Application(
  int &argc,
  char **argv,
  bool haltOnParseError,
  const QString &defaultTheme)
  : QApplication(argc, argv)
{
  Q_INIT_RESOURCE(resources);

  setApplicationName(GITAHEAD_NAME);
  setApplicationVersion(GITAHEAD_VERSION);
  setOrganizationDomain("gitahead.com");

  // Register types that are queued at runtime.
  qRegisterMetaType<git::Id>();

  // Connect updater signals.
  connect(Updater::instance(), &Updater::sslErrors,
          this, &Application::handleSslErrors);

  // Parse command line arguments.
  QCommandLineParser parser;
  parser.setApplicationDescription("GitAhead: Understand your history!");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument(
    "repository", "Try to load a repository from the given path "
    "instead of the current working directory.", "[repository]");
  parser.addOption({{"d", "debug-menu"}, "Show debug menu."});
  parser.addOption({{"t", "theme"}, "Choose theme.", "name"});
  parser.addOption({{"f", "filter"}, "Set the pathspec filter.", "pathspec"});
  parser.addOption({"no-translation", "Disable translation."});

  if (haltOnParseError) {
    parser.process(arguments());
  } else {
    parser.parse(arguments());
  }

  // Remember positional args.
  mPositionalArguments = parser.positionalArguments();

  // Set debug menu option.
  MenuBar::setDebugMenuVisible(parser.isSet("debug-menu"));

  // Set pathspec filter.
  mPathspec = parser.value("filter");

  // Initialize theme.
  QString theme = parser.value("theme");
  mTheme.reset(Theme::create(!theme.isEmpty() ? theme : defaultTheme));
  setStyle(mTheme->style());
  setStyleSheet(mTheme->styleSheet());

  // Read translation settings
  QSettings settings;
  if ((!settings.value("translation/disable", false).toBool()) &&
      (!parser.isSet("no-translation"))) {
    // Load translation files.
    QLocale locale;
    QDir l10n = Settings::l10nDir();
    QString name = QString(GITAHEAD_NAME).toLower();
    QTranslator *translator = new QTranslator(this);
    if (translator->load(locale, name, "_", l10n.absolutePath())) {
      installTranslator(translator);
    } else {
      delete translator;
    }

    // Load Qt translation file.
    QTranslator *qt = new QTranslator(this);
    if (qt->load(locale, "qtbase", "_", l10n.absolutePath())) {
      installTranslator(qt);
    } else {
      QDir dir(QT_TRANSLATIONS_DIR);
      if (dir.exists() && qt->load(locale, "qtbase", "_", dir.absolutePath())) {
        installTranslator(qt);
      } else {
        delete qt;
      }
    }
  }

  // Enable system proxy auto-detection.
  QNetworkProxyFactory::setUseSystemConfiguration(true);

  // Do platform-specific initialization.
#if defined(Q_OS_MACOS)
  // Register service on Mac.
  registerService();

  // Load SF Mono font from Terminal.app.
  QDir dir("/System/Applications");
  if (!dir.exists())
    dir.setPath("/Applications");
  dir.cd("Utilities/Terminal.app/Contents/Resources/Fonts");
  foreach (const QString &name, dir.entryList({"SF*Mono-*.otf"}, QDir::Files))
    QFontDatabase::addApplicationFont(dir.filePath(name));

  // Create shared menu bar on Mac.
  (void) MenuBar::instance(nullptr);

#elif defined(Q_OS_WIN)
  // FIXME: Install in main before anything else?
  defaultFilter = SetUnhandledExceptionFilter(&exceptionFilter);

#elif defined(Q_OS_LINUX)
  QIcon icon;
  icon.addPixmap(QPixmap(":/GitAhead.iconset/icon_16x16.png"));
  icon.addPixmap(QPixmap(":/GitAhead.iconset/icon_32x32.png"));
  icon.addPixmap(QPixmap(":/GitAhead.iconset/icon_64x64.png"));
  icon.addPixmap(QPixmap(":/GitAhead.iconset/icon_128x128.png"));
  setWindowIcon(icon);
#endif

  // Set path to emoji description file.
  git::Commit::setEmojiFile(Settings::confDir().filePath("emoji.json"));

  // Initialize git library.
  git::Repository::init();

  connect(this, &Application::aboutToQuit, [this] {
    // Clean up git library.
    // Make sure windows are really deleted.
    sendPostedEvents(nullptr, QEvent::DeferredDelete);
    git::Repository::shutdown();
  });
}

void Application::autoUpdate()
{
  // Check for updates.
  if (Settings::instance()->value("update/check").toBool()) {
    // Check now.
    Updater::instance()->update(true);

    // Set a timer to check daily.
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, [] {
      Updater::instance()->update(true);
    });

    timer->start(24 * 60 * 60 * 1000);
  }
}

bool Application::restoreWindows()
{
#ifdef Q_OS_MACOS
  // Check for connection to a terminal.
  if (!isatty(fileno(stdin)))
    QDir::setCurrent(Settings::appDir().path());
#endif

  QDir dir = QDir::current();
  if (!mPositionalArguments.isEmpty()) {
    // Check for command line repo.
    QString arg = mPositionalArguments.first();
    if (QFileInfo(arg).isAbsolute()) {
      dir.setPath(arg);
    } else {
      dir.cd(arg);
    }
  }

  // Distinguish two different app modes. The normal mode is entered when the
  // current directory is the same as the app directory (e.g. because the user
  // launched the app by double-clicking in the GUI). It restores the session
  // from the last normal mode invocation. Command line mode tries to load a
  // repository from the current directory. Windows opened in this mode aren't
  // saved to settings. FIXME: Save subsequently opened windows?

  // Load command line arg.
  if (dir != Settings::appDir()) {
    if (MainWindow *win = MainWindow::open(dir.path(), false)) {
      win->currentView()->setPathspec(mPathspec);
      return true;
    }

    return false;
  }

  // Save on exit.
  MainWindow::setSaveWindowSettings(true);

  // Restore previous session.
  return MainWindow::restoreWindows();
}

Theme *Application::theme()
{
  return static_cast<Application *>(instance())->mTheme.data();
}

bool Application::event(QEvent *event)
{
  if (event->type() == QEvent::FileOpen)
    MainWindow::open(static_cast<QFileOpenEvent *>(event)->file());

  return QApplication::event(event);
}

void Application::handleSslErrors(
  QNetworkReply *reply,
  const QList<QSslError> &errors)
{
  QSettings settings;
  if (settings.value("ssl/ignore").toBool()) {
    // FIXME: Ignore individual errors?
    reply->ignoreSslErrors(errors);
    return;
  }

  QString title = tr("SSL Errors");
  QString text =
    tr("Failed to set up SSL session. Do you want to ignore these errors?");
  auto buttons = QMessageBox::Abort | QMessageBox::Ignore;
  QMessageBox msg(QMessageBox::Warning, title, text, buttons);

  QString message;
  foreach (const QSslError &error, errors)
    message.append(QString("<p>%1</p>").arg(error.errorString()));
  msg.setInformativeText(message);

  if (msg.exec()) {
    reply->ignoreSslErrors(errors);
    settings.setValue("ssl/ignore", true);
  }
}

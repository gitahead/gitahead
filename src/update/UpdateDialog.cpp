//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpdateDialog.h"
#include "DownloadDialog.h"
#include "Updater.h"
#include "conf/Settings.h"
#include "dialogs/IconLabel.h"
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

const QString kStyleSheet =
  "h3 {"
  "  color: #696969;"
  "  text-decoration: underline"
  "}";

} // anon. namespace

UpdateDialog::UpdateDialog(
  const QString &version,
  const QString &changelog,
  const QString &link,
  QWidget *parent)
  : QDialog(parent)
{
  QString appName = QCoreApplication::applicationName();
  QString appVersion = QCoreApplication::applicationVersion();

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update %1").arg(appName));

  QIcon icon(":/GitAhead.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 128, 128, this);

  QVBoxLayout *iconLayout = new QVBoxLayout;
  iconLayout->addWidget(iconLabel);
  iconLayout->addStretch();

  QString label =
    tr("<h3>A new version of %1 is available!</h3>"
       "<p>%1 %2 is now available - you have %3. "
       "Would you like to download it now?</p>"
       "<b>Release Notes:</b>").arg(appName, version, appVersion);

  QTextBrowser *browser = new QTextBrowser;
  browser->document()->setDocumentMargin(12);
  browser->document()->setDefaultStyleSheet(kStyleSheet);
  browser->setHtml(changelog);

  QCheckBox *download = new QCheckBox(
    tr("Automatically download and install updates"), this);
  download->setChecked(Settings::instance()->value("update/download").toBool());
  connect(download, &QCheckBox::toggled, [](bool checked) {
    Settings::instance()->setValue("update/download", checked);
  });

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(tr("Install Update"), QDialogButtonBox::AcceptRole);
  buttons->addButton(tr("Remind Me Later"), QDialogButtonBox::RejectRole);
  QPushButton *skip =
    buttons->addButton(tr("Skip This Version"), QDialogButtonBox::ResetRole);
  connect(buttons, &QDialogButtonBox::accepted, this, &UpdateDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &UpdateDialog::reject);
  connect(skip, &QPushButton::clicked, [this, version] {
    Settings *settings = Settings::instance();
    QStringList skipped = settings->value("update/skip").toStringList();
    if (!skipped.contains(version))
      settings->setValue("update/skip", skipped << version);

    reject();
  });

  QVBoxLayout *content = new QVBoxLayout;
  content->addWidget(new QLabel(label, this));
  content->addWidget(browser);
  content->addWidget(download);
  content->addWidget(buttons);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(iconLayout);
  layout->addLayout(content);

  connect(this, &UpdateDialog::accepted, [link] {
    // Start download.
    if (Updater::DownloadRef download = Updater::instance()->download(link)) {
      DownloadDialog *dialog = new DownloadDialog(download);
      dialog->show();
    }
  });
}

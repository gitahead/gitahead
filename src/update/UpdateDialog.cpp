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
#include "ui/MenuBar.h"
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
#include <QDesktopServices>
#include <QMessageBox>

namespace {

const QString kStyleSheet =
  "h3 {"
  "  color: #696969;"
  "  text-decoration: underline"
  "}";

} // anon. namespace

UpdateDialog::UpdateDialog(
  const QString &platform,
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

  QIcon icon(":/Gittyup.iconset/icon_128x128.png");
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
    settings->beginGroup("update");
    QStringList skipped = settings->value("skip").toStringList();
    if (!skipped.contains(version))
      settings->setValue("skip", skipped << version);
    settings->endGroup();
    reject();
  });

  QHBoxLayout* l = new QHBoxLayout();
  QPushButton* supportButton = new QPushButton(QIcon(":/liberapay_icon_130890.png"), tr("Donate"), this);
  QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
  l->addWidget(download);
  l->addItem(spacer);
  l->addWidget(supportButton);
  connect(supportButton, &QPushButton::pressed, [] () {
	  QDesktopServices::openUrl(QUrl(MenuBar::donationUrlLiberapay));
  });

  QVBoxLayout *content = new QVBoxLayout;
  content->addWidget(new QLabel(label, this));
  content->addWidget(browser);
  content->addLayout(l);
  content->addWidget(buttons);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(iconLayout);
  layout->addLayout(content);

  connect(this, &UpdateDialog::accepted, [this, platform, link] {

#ifndef FLATAPK
	// Only the pure linux package is missing
	if (platform == "linux") {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Linux download"));

		msgBox.setText(tr("Linux download"));
		msgBox.setInformativeText(tr("We don't provide a binary to install Gittyup manually on linux. Please download Gittyup from your package manager."));
		msgBox.setStandardButtons(QMessageBox::StandardButton::Ok);
		msgBox.setIcon(QMessageBox::Icon::Information);
		msgBox.exec();
		return;

//		msgBox.setText(tr("On linux distributions the preferred way to install applications is the packet manager."));
//		msgBox.setInformativeText(tr("Do you still want to download the software manually?"));
//		msgBox.setStandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
//		msgBox.setDefaultButton(QMessageBox::StandardButton::No);
//		msgBox.setIcon(QMessageBox::Icon::Question);
//		if (msgBox.exec() == QMessageBox::StandardButton::No)
//			return;
	}
#else
	  QMessageBox msgBox(this);
	  msgBox.setWindowTitle(tr("Flatpak download"));

	  msgBox.setText(tr("If you have downloaded the package from flathub, please wait until the package will be updated there (about 1 day)."));
	  msgBox.setInformativeText(tr("Would you like to download the flatpak package anyway manuall? "));
	  msgBox.setStandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
	  msgBox.setDefaultButton(QMessageBox::StandardButton::No);
	  msgBox.setIcon(QMessageBox::Icon::Question);
	  if (msgBox.exec() == QMessageBox::StandardButton::No)
		  return;
#endif

    // Start download.
	if (Updater::DownloadRef download = Updater::instance()->download(link)) {
      DownloadDialog *dialog = new DownloadDialog(download);
      dialog->show();
    }
  });
}

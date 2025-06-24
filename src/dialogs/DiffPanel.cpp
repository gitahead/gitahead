//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffPanel.h"
#include "ConfigDialog.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCodec>

DiffPanel::DiffPanel(const git::Repository &repo, QWidget *parent)
  : QWidget(parent), mConfig(repo ? repo.config() : git::Config::global())
{
  // diff context
  QSpinBox *context = new QSpinBox(this);
  QLabel *contextLabel = new QLabel(tr("lines"), this);
  QHBoxLayout *contextLayout = new QHBoxLayout;
  contextLayout->addWidget(context);
  contextLayout->addWidget(contextLabel);
  contextLayout->addStretch();
  context->setValue(mConfig.value<int>("diff.context", 3));

  auto contextSignal = QOverload<int>::of(&QSpinBox::valueChanged);
  connect(context, contextSignal, [this](int value) {
    mConfig.setValue("diff.context", value);
    foreach (MainWindow *window, MainWindow::windows()) {
      for (int i = 0; i < window->count(); ++i)
        window->view(i)->refresh();
    }
  });

  // encoding
  QComboBox *encoding = new QComboBox(this);
  encoding->addItem(tr("System Locale"), -1);
  encoding->insertSeparator(encoding->count());
  foreach (int mib, QTextCodec::availableMibs())
    encoding->addItem(QTextCodec::codecForMib(mib)->name(), mib);

  QString name = mConfig.value<QString>("gui.encoding");
  if (!name.isEmpty())
    encoding->setCurrentIndex(encoding->findText(name));

  auto encodingSignal = QOverload<int>::of(&QComboBox::currentIndexChanged);
  connect(encoding, encodingSignal, [this, encoding](int index) {
    if (encoding->itemData(index).toInt() < 0) {
      mConfig.remove("gui.encoding");
    } else {
      mConfig.setValue("gui.encoding", encoding->itemText(index));
    }

    foreach (MainWindow *window, MainWindow::windows()) {
      for (int i = 0; i < window->count(); ++i)
        window->view(i)->refresh();
    }
  });

  QFormLayout *layout = new QFormLayout(this);
  layout->addRow(tr("Context lines:"), contextLayout);
  layout->addRow(tr("Character Encoding:"), encoding);

  // Remaining settings are strictly global.
  if (qobject_cast<ConfigDialog *>(parent))
    return;

  // ignore whitespace
  // The ignore whitespace option is global because it's
  // not a config setting. It's a flag (-w) to git diff.
  QCheckBox *ignoreWs = new QCheckBox(tr("Ignore Whitespace (-w)"), this);
  ignoreWs->setChecked(Settings::instance()->isWhitespaceIgnored());
  connect(ignoreWs, &QCheckBox::toggled, [](bool checked) {
    Settings::instance()->setWhitespaceIgnored(checked);
  });

  // auto collapse
  Settings *settings = Settings::instance();
  QCheckBox *collapseAdded = new QCheckBox(tr("Added files"), this);
  collapseAdded->setChecked(settings->value("collapse/added").toBool());
  connect(collapseAdded, &QCheckBox::toggled, [settings](bool checked) {
    settings->setValue("collapse/added", checked);
  });

  QCheckBox *collapseDeleted = new QCheckBox(tr("Deleted files"), this);
  collapseDeleted->setChecked(settings->value("collapse/deleted").toBool());
  connect(collapseDeleted, &QCheckBox::toggled, [settings](bool checked) {
    settings->setValue("collapse/deleted", checked);
  });

  // hunk limit
  QSpinBox *hunkLimit = new QSpinBox(this);
  QHBoxLayout *hunkLimitLayout = new QHBoxLayout;
  hunkLimitLayout->addWidget(new QLabel(tr("Show first"), this));
  hunkLimitLayout->addWidget(hunkLimit);
  hunkLimitLayout->addWidget(new QLabel(tr("hunks only"), this));
  hunkLimitLayout->addStretch();
  hunkLimit->setMinimum(10);
  hunkLimit->setMaximum(10000);
  hunkLimit->setValue(settings->value("diff/hunklimit").toInt());

  auto hunkLimitSignal = QOverload<int>::of(&QSpinBox::valueChanged);
  connect(hunkLimit, hunkLimitSignal, [settings](int value) {
    settings->setValue("diff/hunklimit", value);
  });

  layout->addRow(tr("Whitespace:"), ignoreWs);
  layout->addRow(tr("Auto Collapse:"), collapseAdded);
  layout->addRow(QString(), collapseDeleted);
  layout->addRow(tr("Hunk:"), hunkLimitLayout);
}

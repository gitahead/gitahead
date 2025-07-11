//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TabWidget.h"
#include "MenuBar.h"
#include "TabBar.h"
#include "dialogs/AccountDialog.h"
#include "dialogs/CloneDialog.h"
#include "host/Account.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";
const QString kSupportLink = "mailto:support@gitahead.com";
const QString kVideoLink = "https://gitahead.com/#tutorials";

class DefaultWidget : public QFrame
{
  Q_OBJECT

public:
  DefaultWidget(QWidget *parent = nullptr)
    : QFrame(parent)
  {
    setFrameShape(QFrame::Box);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    QPushButton *clone =
      addButton(QIcon(":/clone.png"), tr("Clone repository"));
    connect(clone, &QPushButton::clicked, [this] {
      CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, this);
      connect(dialog, &CloneDialog::accepted, [this, dialog] {
        if (MainWindow *window = MainWindow::open(dialog->path()))
          window->currentView()->addLogEntry(
            dialog->message(), dialog->messageTitle());
      });
      dialog->open();
    });

    QPushButton *open =
      addButton(QIcon(":/open.png"), tr("Open existing repository"));
    connect(open, &QPushButton::clicked, [this] {
      // FIXME: Filter out non-git dirs.
      QFileDialog *dialog =
        new QFileDialog(this, tr("Open Repository"), QDir::homePath());
      dialog->setAttribute(Qt::WA_DeleteOnClose);
      dialog->setFileMode(QFileDialog::Directory);
      dialog->setOption(QFileDialog::ShowDirsOnly);
      connect(dialog, &QFileDialog::fileSelected, [](const QString &path) {
        MainWindow::open(path);
      });
      dialog->open();
    });

    QPushButton *init =
      addButton(QIcon(":/new.png"), tr("Initialize new repository"));
    connect(init, &QPushButton::clicked, [this] {
      CloneDialog *dialog = new CloneDialog(CloneDialog::Init, this);
      connect(dialog, &CloneDialog::accepted, [this, dialog] {
        if (MainWindow *window = MainWindow::open(dialog->path()))
          window->currentView()->addLogEntry(
            dialog->message(), dialog->messageTitle());
      });
      dialog->open();
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->addWidget(clone);
    layout->addWidget(open);
    layout->addWidget(init);
    layout->addWidget(addSeparator());

    for (int i = 0; i < 4; ++i) {
      Account::Kind kind = static_cast<Account::Kind>(i);
      QString text = tr("Add %1 account").arg(Account::name(kind));
      QPushButton *account = addButton(Account::icon(kind), text);
      connect(account, &QPushButton::clicked, [this, kind] {
        AccountDialog *dialog = new AccountDialog(nullptr, this);
        dialog->setKind(kind);
        dialog->open();
      });

      layout->addWidget(account);
    }

    layout->addWidget(addSeparator());
    layout->addWidget(addLink(tr("View getting started videos"), kVideoLink));
    layout->addWidget(addLink(tr("Contact us for support"), kSupportLink));
  }

private:
  QPushButton *addButton(const QIcon &icon, const QString &text)
  {
    QPushButton *button = new QPushButton(icon, text, this);
    button->setStyleSheet("color: palette(bright-text); text-align: left");
    button->setIconSize(QSize(32, 32));
    button->setFlat(true);

    QFont font = button->font();
    font.setPointSize(font.pointSize() + 10);
    button->setFont(font);

    return button;
  }

  QLabel *addLink(const QString &text, const QString &link = QString())
  {
    QLabel *label = new QLabel(kLinkFmt.arg(link, text), this);
    label->setOpenExternalLinks(true);

    QFont font = label->font();
    font.setPointSize(font.pointSize() + 3);
    label->setFont(font);

    return label;
  }

  QFrame *addSeparator()
  {
    QFrame *separator = new QFrame(this);
    separator->setStyleSheet("border: 2px solid palette(dark)");
    separator->setFrameShape(QFrame::HLine);
    return separator;
  }
};

} // anon. namespace

TabWidget::TabWidget(QWidget *parent)
  : QTabWidget(parent)
{
  TabBar *bar = new TabBar(this);
  bar->setMovable(true);
  bar->setTabsClosable(true);
  setTabBar(bar);

  // Create default widget.
  mDefaultWidget = new DefaultWidget(this);

  // Handle tab close.
  connect(this, &TabWidget::tabCloseRequested, [this](int index) {
    widget(index)->close();
  });
}

void TabWidget::resizeEvent(QResizeEvent *event)
{
  QTabWidget::resizeEvent(event);

  QSize size = event->size();
  QSize sizeHint = mDefaultWidget->sizeHint();
  int x = (size.width() - sizeHint.width()) / 2;
  int y = (size.height() - sizeHint.height()) / 2;
  mDefaultWidget->move(x, y);
}

void TabWidget::tabInserted(int index)
{
  emit tabAboutToBeInserted();
  QTabWidget::tabInserted(index);
  MenuBar::instance(this)->updateWindow();
  emit tabInserted();

  mDefaultWidget->setVisible(false);
}

void TabWidget::tabRemoved(int index)
{
  emit tabAboutToBeRemoved();
  QTabWidget::tabRemoved(index);
  MenuBar::instance(this)->updateWindow();
  emit tabRemoved();

  mDefaultWidget->setVisible(!count());
}

#include "TabWidget.moc"

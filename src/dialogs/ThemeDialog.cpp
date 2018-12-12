//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "ThemeDialog.h"
#include "app/Theme.h"
#include "conf/Settings.h"
#include <QIcon>
#include <QLabel>
#include <QPalette>
#include <QPointer>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

class ThemeButton : public QPushButton
{
  Q_OBJECT

public:
  ThemeButton(
    const QString &title,
    const QIcon &icon,
    const QString &description,
    QWidget *parent = nullptr)
    : QPushButton(parent)
  {
    setStyleSheet(
      "ThemeButton {"
      "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
      "    stop: 0 #E2E2E2, stop: 1 #ECECEC);"
      "  border: 1px solid #D3D3D3;"
      "  border-radius: 4px"
      "}"
      
      "ThemeButton:default {"
      "  background: #DCEBFB;"
      "  border: 2px solid #AFD1F5"
      "}"
      
      "ThemeButton:pressed {"
      "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
      "    stop: 0 #D8D8D8, stop: 1 #E2E2E2);"
      "  border: 1px solid #D3D3D3"
      "}"
      
      "ThemeButton #description {"
      "  color: #9A9A9A;"
      "  font-size: 12px;"
      "  font-style: italics;"
      "  margin: 0px 0px 16px 0px"
      "}"

      "ThemeButton #title {"
      "  color: #9A9A9A;"
      "  font-weight: bold;"
      "  font-size: 16px;"
      "  margin: 13px 0px 20px 8px"
      "}"
    );
    
    QSize iconSize = QSize(350, 280);
    setFixedHeight(iconSize.height() + 80);
    setFixedWidth(iconSize.width() + 35);
    setFocusPolicy(Qt::StrongFocus);
    
    mTitle = new QLabel(title, this);
    mTitle->setObjectName("title");
    
    setIcon(icon);
    setIconSize(iconSize);
    
    mDescription = new QLabel(description, this);
    mDescription->setAlignment(Qt::AlignHCenter);
    mDescription->setMinimumWidth(rect().width());
    mDescription->setObjectName("description");
    
    connect(this, &QPushButton::clicked, [this] {
      if (mTitle->text().startsWith("dark", Qt::CaseInsensitive)) {
        Settings::instance()->setValue("window/theme", "Dark");
      } else {
        Settings::instance()->setValue("window/theme", "Default");
      }

      window()->close();
    });
  }

  void resizeEvent(QResizeEvent *event) override
  {
    QSize size = mDescription->sizeHint();
    mDescription->move(rect().left(), height() - (size.height() + 5));
    mTitle->move(rect().left() + 5, rect().top());
  }

private:
  QLabel *mTitle;
  QLabel *mDescription;
};

} // anon. namespace

ThemeDialog::ThemeDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle("Pick a theme for GitAhead");
  
  ThemeButton *native = new ThemeButton(
    tr("Native Theme"),
    QIcon(":/native.png"),
    tr("A flexible look matching system colors")
  );
  
  ThemeButton *dark = new ThemeButton(
    tr("Dark Theme"), 
    QIcon(":/dark.png"),
    tr("A consistent look optimal for reducing eye strain")
  );
  
  QHBoxLayout *themeButtons = new QHBoxLayout;
  themeButtons->addWidget(native);
  themeButtons->addWidget(dark);
  
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(themeButtons);
}

#include "ThemeDialog.moc"

//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>

class TabWidget;

class SideBar : public QWidget
{
public:
  SideBar(TabWidget *tabs, QWidget *parent = nullptr);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
};

#endif

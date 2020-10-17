//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFPANEL_H
#define DIFFPANEL_H

#include "git/Config.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QWidget>

class QHBoxLayout;

class DiffPanel : public QWidget
{
  Q_OBJECT

public:
  DiffPanel(const git::Repository &repo, QWidget *parent = nullptr);

  void refresh(void);

private:
  git::Config mConfig;
  git::Config mAppConfig;

  QSpinBox *mContext;
  QComboBox *mEncoding;

  QCheckBox *mIgnoreWs = nullptr;

  QCheckBox *mCollapseAdded;
  QCheckBox *mCollapseDeleted;
};

#endif

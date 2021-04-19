//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "git/Repository.h"
#include <QTreeWidget>

class PluginsPanel : public QTreeWidget
{
  Q_OBJECT

public:
  enum Column
  {
    Name,
    Kind,
    Description
  };

  PluginsPanel(const git::Repository &repo, QWidget *parent = nullptr);

  void refresh();

  QSize sizeHint() const override;

private:
  git::Repository mRepo;
};

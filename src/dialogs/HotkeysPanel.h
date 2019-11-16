//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include <QTreeView>

class HotkeysPanel : public QTreeView
{
public:
  enum Column
  {
    Name,
    Kind,
    Description
  };

  HotkeysPanel(QWidget *parent = nullptr);

  QSize sizeHint() const override;
};

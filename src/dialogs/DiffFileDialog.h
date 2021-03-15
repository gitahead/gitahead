//
//          Copyright (c) 2021, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef DIFFFILEDIALOG_H
#define DIFFFILEDIALOG_H

#include <QCoreApplication>

class QString;
class QWidget;

class DiffFileDialog
{
  Q_DECLARE_TR_FUNCTIONS(DiffFileDialog)

public:
  static QString getApplyFileName(QWidget *parent = nullptr);
  static QString getSaveFileName(QWidget *parent = nullptr);

private:
  static QString filter();

  static QString lastDir();
  static void saveLastDir(const QString &path);
};

#endif

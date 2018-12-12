//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TAGDIALOG_H
#define TAGDIALOG_H

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QTextEdit;

namespace git {
class Repository;
}

class TagDialog : public QDialog
{
  Q_OBJECT

public:
  TagDialog(
    const git::Repository &repo,
    const QString &id,
    QWidget *parent = nullptr);

  bool force() const;
  QString name() const;
  QString message() const;

private:
  QLineEdit *mNameField;
  QCheckBox *mForce;
  QTextEdit *mMessage;
};

#endif

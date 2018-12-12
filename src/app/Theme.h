//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef THEME_H
#define THEME_H

#include <QDir>
#include <QPalette>
#include <QString>

class QStyle;
class QStyleOption;

class Theme
{
public:
  enum class BadgeRole
  {
    Foreground,
    Background
  };

  enum class BadgeState
  {
    Normal,
    Selected,
    Conflicted,
    Head,
    Notification
  };

  enum class Diff
  {
    Ours,
    Theirs,
    Addition,
    Deletion,

    WordAddition,
    WordDeletion,
    Plus,
    Minus,

    Note,
    Warning,
    Error
  };

  enum class HeatMap
  {
    Hot,
    Cold
  };

  enum class Comment
  {
    Background,
    Body,
    Author,
    Timestamp
  };

  virtual ~Theme() = default;

  virtual QDir dir() const;
  virtual QString name() const;
  virtual QPalette palette() const;
  virtual QStyle *style() const;
  virtual QString styleSheet() const;

  virtual QColor badge(BadgeRole role, BadgeState state);
  virtual QList<QColor> branchTopologyEdges();
  virtual QColor buttonChecked();
  virtual QPalette commitList();
  virtual QColor diff(Diff color);
  virtual QPalette fileList();
  virtual QPalette footer();
  virtual QColor heatMap(HeatMap color);
  virtual QColor remoteComment(Comment color);
  virtual QPalette stars();
  virtual QColor windowBrightText();

  static void drawCloseButton(
    const QStyleOption *option,
    QPainter *painter);

  static Theme *create(const QString &name = QString());
};

#endif

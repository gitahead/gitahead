//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BADGE_H
#define BADGE_H

#include <QWidget>

class QStyleOption;

enum BadgeAlignment {
  LEFT,
  RIGHT
};

class Badge : public QWidget
{
public:
  struct Label
  {
    Label(const QString &text = QString(), bool bold = false, bool tag = false)
      : text(text), bold(bold), tag(tag)
    {}

    QString text;
    bool bold;
    bool tag;
  };

  Badge(const QList<Label> &labels, QWidget *parent = nullptr);

  void appendLabel(const Label &label);
  void setLabels(const QList<Label> &labels);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  static QSize size(const QFont &font, const QList<Label> &labels);
  static QSize size(const QFont &font, const Label &label = Label());

  static int paint(
    QPainter *painter,
    const QList<Label> &labels,
    const QRect &rect,
    QStyleOption *opt = nullptr,
    BadgeAlignment alignment = RIGHT);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QList<Label> mLabels;
  static int paintRight(QPainter *painter,
    const QList<Label> &labels,
    const QRect &rect,
     const QFont font,
    bool selected,
    bool active);

  static int paintLeft(QPainter *painter,
    const QList<Label> &labels,
    const QRect &rect,
    const QFont font,
    bool selected,
    bool active);

  static void paintBadge(
    QPainter *painter,
    bool selected,
    bool active,
    Label label,
    QRect labelRect,
    const QFont font
  );
};

#endif

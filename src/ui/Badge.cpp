//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Badge.h"
#include "app/Application.h"
#include <QPainter>
#include <QStyleOption>

namespace {

const int kSpacing = 4;
const int kPadding = 12;
const int kIconWidth = 14;
const QString kEllipsis = "...";
Qt::Alignment kLabelAlignment = Qt::AlignHCenter | Qt::AlignVCenter;

bool isEmpty(QList<Badge::Label> &labels)
{
  if (!labels.isEmpty() && labels.last().text == kEllipsis)
    labels.removeLast();
  return labels.isEmpty();
}

} // anon. namepsace

Badge::Badge(const QList<Label> &labels, QWidget *parent)
  : QWidget(parent), mLabels(labels)
{
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}

void Badge::appendLabel(const Label &label)
{
  mLabels.append(label);
  updateGeometry();
}

void Badge::setLabels(const QList<Label> &labels)
{
  mLabels = labels;
  updateGeometry();
}

QSize Badge::sizeHint() const
{
  return !mLabels.isEmpty() ? size(font(), mLabels) : QSize();
}

QSize Badge::minimumSizeHint() const
{
  return !mLabels.isEmpty() ? size(font(), kEllipsis) : QSize();
}

QSize Badge::size(const QFont &font, const QList<Label> &labels)
{
  Q_ASSERT(!labels.isEmpty());

  int width = 0;
  QFontMetrics fm(font);
  foreach (const Label &label, labels)
    width += size(font, label).width();
  return QSize(width + ((labels.size() - 1) * kSpacing), fm.lineSpacing() + 2);
}

QSize Badge::size(const QFont &font, const Label &label)
{
  QFont copy = font;
  copy.setBold(label.bold);

  QFontMetrics fm(copy);
  int icon = label.tag ? kIconWidth : 0;
  int width = (label.text.length() > 1) ?
    fm.horizontalAdvance(label.text) : fm.averageCharWidth();
  return QSize(icon + width + kPadding, fm.lineSpacing() + 2);
}

/** Draws the bagdes to all available space. Default Right Aligned.
 * Retuns the final width occupied by the Badges. */
int Badge::paint(
  QPainter *painter,
  const QList<Label> &list,
  const QRect &rect,
  QStyleOption *opt,
  BadgeAlignment alignment)
{

  bool active = (opt && (opt->state & QStyle::State_Active));
  bool selected = (opt && (opt->state & QStyle::State_Selected));

#ifdef Q_OS_WIN
  // Disable selected state in the default theme.
  selected = (selected && Application::theme()->name() != "Default");
#endif

  // Elide labels.
  QList<Label> labels = list;
  QFont font = painter->font();
  while (size(font, labels).width() > rect.width() && !isEmpty(labels))
    labels.last().text = kEllipsis;

  // Draw labels.
  painter->save();
  painter->setRenderHints(QPainter::Antialiasing);

  if (alignment == RIGHT) {
    return Badge::paintRight(painter, labels, rect, font, selected, active);
  }
  else {
    return Badge::paintLeft(painter, labels, rect, font, selected, active);
  }
}

int Badge::paintLeft(
  QPainter *painter,
  const QList<Label> &labels,
  const QRect &rect,
  const QFont font,
  bool selected,
  bool active )
{
  int x = rect.x();

  for (int i = 0; i < labels.size(); i++) {
    Label label = labels.at(i);
    QSize labelSize = size(font, label);
    QRect labelRect(QPoint(x, rect.y()), labelSize);
    paintBadge(painter, selected, active, label, labelRect, font);
    x += labelSize.width() + kSpacing;
  }

  painter->restore();
  return x;
};

int Badge::paintRight(
  QPainter *painter,
  const QList<Label> &labels,
  const QRect &rect,
  const QFont font,
  bool selected,
  bool active)
{
  int x = rect.x() + rect.width();

  for (int i = labels.size() - 1; i >= 0; --i) {
    Label label = labels.at(i);
    QSize labelSize = size(font, label);
    QRect labelRect(QPoint(x - labelSize.width(), rect.y()), labelSize);
    paintBadge(painter, selected, active, label, labelRect, font);
    x -= labelSize.width() + kSpacing;
  }

  painter->restore();
  return rect.x() + rect.width() - x;
};

void Badge::paintBadge(
  QPainter *painter,
  bool selected,
  bool active,
  Label label,
  QRect labelRect,
  const QFont font
) {
    Theme *theme = Application::theme();
    Theme::BadgeState state = Theme::BadgeState::Normal;
    if (selected && active) {
      state = Theme::BadgeState::Selected;
    } else if (label.text == "!") {
      state = Theme::BadgeState::Conflicted;
    } else if (label.bold) {
      state = Theme::BadgeState::Head;
    }

    QColor fore = theme->badge(Theme::BadgeRole::Foreground, state);
    QColor back = theme->badge(Theme::BadgeRole::Background, state);

    painter->setBrush(back);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(labelRect, 4, 4);

    QFont copy = font;
    copy.setBold(label.bold);
    painter->setFont(copy);

    if (label.tag) {
      qreal x = labelRect.x() + (kPadding / 2) + 5;
      qreal y = labelRect.y() + (labelRect.height() / 2) + 1;
      QPainterPath path;
      path.moveTo(x, y - 5);
      path.lineTo(x - 3.5, y - 5);
      path.quadTo(x - 5, y - 5, x - 5, y - 3.5);
      path.lineTo(x - 5, y);
      path.lineTo(x - 1.5, y + 3.5);
      path.quadTo(x, y + 5, x + 1.5, y + 3.5);
      path.lineTo(x + 3.5, y + 1.5);
      path.quadTo(x + 5, y, x + 3.5, y - 1.5);
      path.closeSubpath();
      painter->fillPath(path, fore);
      painter->drawEllipse(QRectF(x - 3.5, y - 3.5, 2, 2));

      labelRect.setX(labelRect.x() + kIconWidth);
    }

    painter->setPen(fore);
    painter->drawText(labelRect, kLabelAlignment, label.text);
}

void Badge::paintEvent(QPaintEvent *event)
{
  if (mLabels.isEmpty())
    return;

  QPainter painter(this);
  paint(&painter, mLabels, rect());
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Theme.h"
#include "CustomTheme.h"
#include "conf/Settings.h"
#include "dialogs/ThemeDialog.h"
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOption>
#include <QWidget>

namespace {

class Style : public QProxyStyle
{
public:
  Style(const Theme *theme)
    : QProxyStyle("fusion"), mTheme(theme)
  {}

  void drawPrimitive(
    PrimitiveElement elem,
    const QStyleOption *option,
    QPainter *painter,
    const QWidget *widget) const override
  {
    switch (elem) {
      case PE_IndicatorTabClose:
        Theme::drawCloseButton(option, painter);
        break;

      default:
        baseStyle()->drawPrimitive(elem, option, painter, widget);
        break;
    }
  }

  void polish(QPalette &palette) override
  {
    baseStyle()->polish(palette);
    mTheme->polish(palette);
  }

private:
  const Theme *mTheme;
};

} // anon. namespace

Theme::Theme()
{
  QPalette palette;
  QColor base = palette.color(QPalette::Base);
  QColor text = palette.color(QPalette::Text);
  mDark = (text.lightnessF() > base.lightnessF());
}

QDir Theme::dir() const
{
  QDir dir = Settings::confDir();
  dir.cd("themes");
  return dir;
}

QString Theme::name() const
{
  return "Default";
}

QStyle *Theme::style() const
{
  return new Style(this);
}

QString Theme::styleSheet() const
{
  return QString();
}

void Theme::polish(QPalette &palette) const
{
  palette.setColor(QPalette::BrightText, mDark ? "#9C9C9C" : "#646464");
  palette.setColor(QPalette::Light, mDark ? "#121212" : "#E6E6E6");
  palette.setColor(QPalette::Shadow, palette.color(QPalette::Mid));

  if (mDark)
    palette.setColor(QPalette::Link, "#2A82DA");
}

QColor Theme::badge(BadgeRole role, BadgeState state)
{
  if (mDark) {
    switch (role) {
      case BadgeRole::Foreground:
        switch (state) {
          case BadgeState::Selected:
            return "#2A82DA";

          default:
            return "#E1E5F2";
        }

      case BadgeRole::Background:
        switch (state) {
          case BadgeState::Normal:
            return "#2A82DA";

          case BadgeState::Selected:
            return "#E1E5F2";

          case BadgeState::Conflicted:
            return "#DA2ADA";

          case BadgeState::Head:
            return "#52A500";

          case BadgeState::Notification:
            return "#8C2026";
        }
    }
  }

  switch (role) {
    case BadgeRole::Foreground:
      switch (state) {
        case BadgeState::Selected:
          return "#6C6C6C";

        default:
          return Qt::white;
      }

    case BadgeRole::Background:
      switch (state) {
        case BadgeState::Normal:
          return "#A6ACB6";

        case BadgeState::Selected:
          return Qt::white;

        case BadgeState::Conflicted:
          return "#D22222";

        case BadgeState::Head:
          return "#6F7379";

        case BadgeState::Notification:
          return Qt::red;
      }
  }
}

QList<QColor> Theme::branchTopologyEdges()
{
  return {
    "steelblue",
    "crimson",
    "forestgreen",
    "goldenrod",
    "darkviolet",
    "darkcyan",
    "orange",
    "cornflowerblue",
    "tomato",
    "darkturquoise",
    "palevioletred",
    "sandybrown"
  };
}

QColor Theme::buttonChecked()
{
  return mDark ? "#19B4FB" : "#0086F3";
}

QPalette Theme::commitList()
{
  QPalette palette;
  QColor bright = palette.color(QPalette::BrightText);
  QColor inactive = mDark ? Qt::white : Qt::black;

  palette.setColor(QPalette::Active, QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, inactive);
  palette.setColor(QPalette::Active, QPalette::WindowText, "#C0C0C0");
  palette.setColor(QPalette::Inactive, QPalette::WindowText, bright);
  return palette;
}

QColor Theme::commitEditor(CommitEditor color)
{
  if (mDark) {
    switch (color) {
      case CommitEditor::SpellError:    return "#BC0009";
      case CommitEditor::SpellIgnore:   return "#E1E5F2";
      case CommitEditor::LengthWarning: return "#464614";
    }
  }
  switch (color) {
    case CommitEditor::SpellError:    return Qt::red;
    case CommitEditor::SpellIgnore:   return Qt::gray;
    case CommitEditor::LengthWarning: return "#EFF0F1";
  }
}

QColor Theme::diff(Diff color)
{
  if (mDark) {
    switch (color) {
      case Diff::Ours:         return "#000060";
      case Diff::Theirs:       return "#600060";
      case Diff::Addition:     return "#394734";
      case Diff::Deletion:     return "#5E3638";
      case Diff::WordAddition: return "#296812";
      case Diff::WordDeletion: return "#781B20";
      case Diff::Plus:         return "#207A00";
      case Diff::Minus:        return "#BC0009";
      case Diff::Note:         return "#E1E5F2";
      case Diff::Warning:      return "#E8C080";
      case Diff::Error:        return "#7E494B";
    }
  }

  switch (color) {
    case Diff::Ours:         return "#DCFFFF";
    case Diff::Theirs:       return "#FFDCFF";
    case Diff::Addition:     return "#DCFFDC";
    case Diff::Deletion:     return "#FFDCDC";
    case Diff::WordAddition: return "#B0F2B0";
    case Diff::WordDeletion: return "#F2B0B0";
    case Diff::Plus:         return "#45CC45";
    case Diff::Minus:        return "#F28080";
    case Diff::Note:         return Qt::black;
    case Diff::Warning:      return Qt::yellow;
    case Diff::Error:        return Qt::red;
  }
}

QPalette Theme::fileList()
{
  return QPalette();
}

QColor Theme::heatMap(HeatMap color)
{
  if (mDark) {
    switch (color) {
      case HeatMap::Hot:  return "#5E3638";
      case HeatMap::Cold: return "#282940";
    }
  }

  switch (color) {
    case HeatMap::Hot:  return "#FFC0C0";
    case HeatMap::Cold: return "#C0C0FF";
  }
}

QColor Theme::remoteComment(Comment color)
{
  switch (color) {
    case Comment::Background: return QPalette().color(QPalette::Base);
    case Comment::Body:       return "#383838";
    case Comment::Author:     return "#1A76F4";
    case Comment::Timestamp:  return "#6C6C6C";
  }
}

QColor Theme::star()
{
  return "#FFCE6D";
}

void Theme::drawCloseButton(
  const QStyleOption *option,
  QPainter *painter)
{
  qreal in = 3.5;
  qreal out = 8.0;
  QRect rect = option->rect;
  qreal x = rect.x() + (rect.width() / 2);
  qreal y = rect.y() + (rect.height() / 2);

  painter->save();
  painter->setRenderHints(QPainter::Antialiasing);

  // Draw background.
  if (option->state & QStyle::State_MouseOver) {
    painter->save();
    painter->setPen(Qt::NoPen);
    bool selected = (option->state & QStyle::State_Selected);
    painter->setBrush(QColor(selected ? QPalette().color(QPalette::Highlight) :
                                        QPalette().color(QPalette::Base)));
    QRectF background(x - out, y - out, 2 * out, 2 * out);
    painter->drawRoundedRect(background, 2.0, 2.0);
    painter->restore();
  }

  // Draw x.
  painter->setPen(QPen(QPalette().color(QPalette::WindowText), 1.5));
  painter->drawLine(QPointF(x - in, y - in), QPointF(x + in, y + in));
  painter->drawLine(QPointF(x - in, y + in), QPointF(x + in, y - in));
  painter->restore();
}

Theme *Theme::create(const QString &defaultName)
{
  // Upgrade theme key to capital case.
  Settings *settings = Settings::instance();
  QString key = settings->value("window/theme").toString();
  if (key == "default" || key == "dark") {
    key[0] = key.at(0).toUpper();
    settings->setValue("window/theme", key);
  }

  QString name = !defaultName.isEmpty() ? defaultName : key;
  if (name.isEmpty()) {
    ThemeDialog dialog;
    dialog.exec();
    name = settings->value("window/theme").toString();
  }

  // Load custom theme.
  if (CustomTheme::isValid(name))
    return new CustomTheme(name);

  // Use Qt theme.
  return new Theme;
}

//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ColumnView.h"
#include "Badge.h"
#include "TreeModel.h"
#include <QFormLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#define ICON_SIZE 48
#define SCROLL_BAR_WIDTH 18
#else
#define ICON_SIZE 64
#define SCROLL_BAR_WIDTH 0
#endif

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

class PreviewWidget : public QFrame
{
  Q_OBJECT

public:
  PreviewWidget(ColumnView *parent)
    : QFrame(parent)
  {
    mIcon = new QLabel(this);
    mIcon->setAlignment(Qt::AlignHCenter);

    mName = new QLabel(this);
    mName->setAlignment(Qt::AlignHCenter);
    mName->setContentsMargins(4,4,4,4);

    mKind = new QLabel(this);
    mKind->setAlignment(Qt::AlignHCenter);

    mAdded = new QLabel(this);
    mModified = new QLabel(this);

    connect(mAdded, &QLabel::linkActivated,
            parent, &ColumnView::linkActivated);
    connect(mModified, &QLabel::linkActivated,
            parent, &ColumnView::linkActivated);

    QFormLayout *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    form->setHorizontalSpacing(4);
    form->setVerticalSpacing(2);
    form->addRow(kLabelFmt.arg(tr("Added")), mAdded);
    form->addRow(kLabelFmt.arg(tr("Modified")), mModified);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->addWidget(mIcon);
    layout->addWidget(mName);
    layout->addWidget(mKind);
    layout->addLayout(form);
  }

  void setFile(const QModelIndex &index, int width)
  {
    mIndex = index;
    if (!index.isValid() || index.model()->rowCount(index) > 0) {
      hide();
      return;
    }

    QWindow *win = window()->windowHandle();
    qreal dpr = win ? win->devicePixelRatio() : 1.0;
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    mIcon->setPixmap(icon.pixmap(QSize(ICON_SIZE, ICON_SIZE), dpr));

    mName->setText(kNameFmt.arg(index.data(Qt::DisplayRole).toString()));

    QString kind = index.data(TreeModel::KindRole).toString();
    mKind->setVisible(!kind.isEmpty());
    mKind->setText(kLabelFmt.arg(kind));

    mAdded->setText(index.data(TreeModel::AddedRole).toString());
    mModified->setText(index.data(TreeModel::ModifiedRole).toString());

    // QColumnView resizes the preview container
    // to the *size* of the preview widget.
    resize(QSize(width, sizeHint().height()));
    show();
  }

signals:
  void iconDoubleClicked(const QModelIndex &index);

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override
  {
    if (mIcon->geometry().contains(event->pos()))
      emit iconDoubleClicked(mIndex);
  }

private:
  QLabel *mIcon;
  QLabel *mName;
  QLabel *mKind;
  QLabel *mAdded;
  QLabel *mModified;
  QModelIndex mIndex;
};

class ColumnViewDelegate : public QItemDelegate
{
public:
  ColumnViewDelegate(QObject *parent = nullptr)
    : QItemDelegate(parent)
  {}

  void paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override
  {
    QStyleOptionViewItem opt = option;
    drawBackground(painter, opt, index);

    // Draw >.
    if (index.model()->hasChildren(index)) {
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing, true);

      QColor color = opt.palette.color(QPalette::Active, QPalette::BrightText);
      if (opt.state & QStyle::State_Selected)
        color = !opt.showDecorationSelected ?
          opt.palette.color(QPalette::Active, QPalette::WindowText) :
          opt.palette.color(QPalette::Active, QPalette::HighlightedText);

      painter->setPen(color);
      painter->setBrush(color);

      int x = opt.rect.x() + opt.rect.width() - 3;
      int y = opt.rect.y() + (opt.rect.height() / 2);

      QPainterPath path;
      path.moveTo(x, y);
      path.lineTo(x - 5, y - 3);
      path.lineTo(x - 5, y + 3);
      path.closeSubpath();
      painter->drawPath(path);

      painter->restore();

      // Adjust rect to exclude the arrow.
      opt.rect.adjust(0, 0, -11, 0);
    }

    // Draw badges.
    QString status = index.data(TreeModel::StatusRole).toString();
    if (!status.isEmpty()) {
      QSize size = Badge::size(painter->font());
      int width = size.width();
      int height = size.height();

      // Add extra space.
      opt.rect.adjust(0, 0, -3, 0);

      for (int i = 0; i < status.length(); ++i) {
        int x = opt.rect.x() + opt.rect.width();
        int y = opt.rect.y() + (opt.rect.height() / 2);
        QRect rect(x - width, y - (height / 2), width, height);
        Badge::paint(painter, {Badge::Label(status.at(i))}, rect, &opt);

        // Adjust rect.
        opt.rect.adjust(0, 0, -width - 3, 0);
      }
    }

    QItemDelegate::paint(painter, opt, index);
  }

  QSize sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override
  {
    // Increase spacing.
    QSize size = QItemDelegate::sizeHint(option, index);
    size.setHeight(Badge::size(option.font).height() + 4);
    return size;
  }
};

} // anon. namespace

ColumnView::ColumnView(QWidget *parent)
  : QColumnView(parent), mSharedDelegate(new ColumnViewDelegate(this))
{
  PreviewWidget *preview = new PreviewWidget(this);
  connect(preview, &PreviewWidget::iconDoubleClicked,
          this, &ColumnView::doubleClicked);

  setPreviewWidget(preview);
}

void ColumnView::setModel(QAbstractItemModel *model)
{
  QColumnView::setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &ColumnView::handleSelectionChange);
}

bool ColumnView::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
    QWidget *columnViewport = static_cast<QWidget *>(obj);
    QPointF pos = static_cast<QMouseEvent *>(event)->globalPosition();
    QModelIndex index = indexAt(viewport()->mapFromGlobal(pos).toPoint());
    if (!columnViewport->hasFocus() && index.row() < 0) {
      columnViewport->setFocus();
      selectionModel()->clearSelection();
    }
  }

  return false;
}

QAbstractItemView *ColumnView::createColumn(const QModelIndex &index)
{
  QAbstractItemView *view = QColumnView::createColumn(index);
  view->setItemDelegate(mSharedDelegate);
  view->viewport()->installEventFilter(this);
  return view;
}

void ColumnView::handleSelectionChange(
  const QItemSelection &selected,
  const QItemSelection &deselected)
{
  // FIXME: The argument sent by Qt doesn't contain the whole selection.
  int width = columnWidths().last() - SCROLL_BAR_WIDTH;
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  QModelIndex index = (indexes.size() == 1) ? indexes.first() : QModelIndex();
  static_cast<PreviewWidget *>(previewWidget())->setFile(index, width);
  emit fileSelected(index);

  // Handle deselection.
  if (indexes.isEmpty() && !deselected.indexes().isEmpty()) {
    QModelIndex parent = deselected.indexes().first().parent();
    setCurrentIndex(parent);
    if (!parent.isValid())
      setRootIndex(QModelIndex());
  }
}

#include "ColumnView.moc"

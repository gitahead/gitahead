//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DetailView.h"
#include "Badge.h"
#include "ContextMenuButton.h"
#include "DiffWidget.h"
#include "MenuBar.h"
#include "SpellChecker.h"
#include "TreeWidget.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include "git/TagRef.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>
#include <QtConcurrent>

namespace {

const int kSize = 64;
const char *kCacheKey = "cache_key";
const QString kRangeFmt = "%1..%2";
const QString kDateRangeFmt = "%1-%2";
const QString kBoldFmt = "<b>%1</b>";
const QString kItalicFmt = "<i>%1</i>";
const QString kLinkFmt = "<a href='%1'>%2</a>";
const QString kAuthorFmt = "<b>%1 &lt;%2&gt;</b>";
const QString kAltFmt = "<span style='color: %1'>%2</span>";
const QString kUrl = "http://www.gravatar.com/avatar/%1?s=%2&d=mm";

const QString kSpin =
  "QSpinBox {"
  "  border: none;"
  "}"
  "QSpinBox::up-button {"
  "  top: 0px;"
  "}"
  "QSpinBox::down-button {"
  "  bottom: 0px;"
  "}";

const QString kComboBox =
  "QComboBox {"
  "  border: none;"
  "  padding: 1px, 1px, 1px, 1px;"
  "}";

const QString kSubjectCheckKey = "commit.subject.lengthcheck";
const QString kSubjectLimitKey = "commit.subject.limit";
const QString kBlankKey = "commit.blank.insert";
const QString kBodyCheckKey = "commit.body.lengthcheck";
const QString kBodyLimitKey = "commit.body.limit";
const QString kDictKey = "commit.spellcheck.dict";

const Qt::TextInteractionFlags kTextFlags =
  Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;

QString brightText(const QString &text)
{
  return kAltFmt.arg(QPalette().color(QPalette::BrightText).name(), text);
}

class MessageLabel : public QTextEdit
{
public:
  MessageLabel(QWidget *parent = nullptr)
    : QTextEdit(parent)
  {
    setObjectName("MessageLabel");
    setFrameShape(QFrame::NoFrame);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setReadOnly(true);
    document()->setDocumentMargin(0);

    // Notify the layout system when size hint changes.
    connect(document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &MessageLabel::updateGeometry);
  }

protected:
  QSize minimumSizeHint() const override
  {
    QSize size = QTextEdit::minimumSizeHint();
    return QSize(size.width(), fontMetrics().lineSpacing());
  }

  QSize viewportSizeHint() const override
  {
    // Choose the smaller of the height of the document or five lines.
    QSize size = QTextEdit::viewportSizeHint();
    int height = document()->documentLayout()->documentSize().height();
    return QSize(size.width(), qMin(height, 5 * fontMetrics().lineSpacing()));
  }
};

class StackedWidget : public QStackedWidget
{
public:
  StackedWidget(QWidget *parent = nullptr)
    : QStackedWidget(parent)
  {}

  QSize sizeHint() const override
  {
    return currentWidget()->sizeHint();
  }

  QSize minimumSizeHint() const override
  {
    return currentWidget()->minimumSizeHint();
  }
};

class AuthorDate : public QWidget
{
public:
  AuthorDate(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mAuthor = new QLabel(this);
    mAuthor->setTextInteractionFlags(kTextFlags);

    mDate = new QLabel(this);
    mDate->setTextInteractionFlags(kTextFlags);

    mSpacing = style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
  }

  void moveEvent(QMoveEvent *event) override
  {
    updateLayout();
  }

  void resizeEvent(QResizeEvent *event) override
  {
    updateLayout();
  }

  QSize sizeHint() const override
  {
    QSize date = mDate->sizeHint();
    QSize author = mAuthor->sizeHint();
    int width = author.width() + date.width() + mSpacing;
    return QSize(width, qMax(author.height(), date.height()));
  }

  QSize minimumSizeHint() const override
  {
    QSize date = mDate->minimumSizeHint();
    QSize author = mAuthor->minimumSizeHint();
    int width = qMax(author.width(), date.width());
    return QSize(width, qMax(author.height(), date.height()));
  }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int width) const override
  {
    int date = mDate->sizeHint().height();
    int author = mAuthor->sizeHint().height();
    bool wrapped = (width < sizeHint().width());
    return wrapped ? (author + date + mSpacing) : qMax(author, date);
  }

  void setAuthor(const QString &author)
  {
    mAuthor->setText(author);
    mAuthor->adjustSize();
    updateGeometry();
  }

  void setDate(const QString &date)
  {
    mDate->setText(date);
    mDate->adjustSize();
    updateGeometry();
  }

private:
  void updateLayout()
  {
    mAuthor->move(0, 0);

    bool wrapped = (width() < sizeHint().width());
    int x = wrapped ? 0 : width() - mDate->width();
    int y = wrapped ? mAuthor->height() + mSpacing : 0;
    mDate->move(x, y);
  }

  QLabel *mAuthor;
  QLabel *mDate;

  int mSpacing;
};

class CommitDetail : public QFrame
{
  Q_OBJECT

public:
  CommitDetail(QWidget *parent = nullptr)
    : QFrame(parent)
  {
    mAuthorDate = new AuthorDate(this);

    mHash = new QLabel(this);
    mHash->setTextInteractionFlags(kTextFlags);

    QToolButton *copy = new QToolButton(this);
    copy->setText(tr("Copy"));
    connect(copy, &QToolButton::clicked, [this] {
      QApplication::clipboard()->setText(mId);
    });

    mRefs = new Badge(QList<Badge::Label>(), this);

    QHBoxLayout *line2 = new QHBoxLayout;
    line2->addWidget(mHash);
    line2->addWidget(copy);
    line2->addStretch();
    line2->addWidget(mRefs);

    mParents = new QLabel(this);
    mParents->setTextInteractionFlags(kTextFlags);

    QHBoxLayout *line3 = new QHBoxLayout;
    line3->addWidget(mParents);
    line3->addStretch();

    QVBoxLayout *details = new QVBoxLayout;
    details->setSpacing(6);
    details->addWidget(mAuthorDate);
    details->addLayout(line2);
    details->addLayout(line3);
    details->addStretch();

    mPicture = new QLabel(this);

    QHBoxLayout *header = new QHBoxLayout;
    header->addLayout(details);
    header->addWidget(mPicture);

    mSeparator = new QFrame(this);
    mSeparator->setObjectName("separator");
    mSeparator->setFrameShape(QFrame::HLine);

    mMessage = new MessageLabel(this);
    connect(mMessage, &QTextEdit::copyAvailable,
            MenuBar::instance(this), &MenuBar::updateCutCopyPaste);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addWidget(mSeparator);
    layout->addWidget(mMessage);

    connect(&mMgr, &QNetworkAccessManager::finished,
            this, &CommitDetail::setPicture);

    RepoView *view = RepoView::parentView(this);
    connect(mHash, &QLabel::linkActivated, view, &RepoView::visitLink);
    connect(mParents, &QLabel::linkActivated, view, &RepoView::visitLink);

    connect(&mWatcher, &QFutureWatcher<QString>::finished, this, [this] {
      QString result = mWatcher.result();
      if (result.contains('+'))
        mRefs->appendLabel({result, false, true});
    });

    // Respond to reference changes.
    auto resetRefs = [this] {
      RepoView *view = RepoView::parentView(this);
      setReferences(view->commits());
    };

    git::RepositoryNotifier *notifier = view->repo().notifier();
    connect(notifier, &git::RepositoryNotifier::referenceAdded, resetRefs);
    connect(notifier, &git::RepositoryNotifier::referenceRemoved, resetRefs);
    connect(notifier, &git::RepositoryNotifier::referenceUpdated, resetRefs);
  }

  void setReferences(const QList<git::Commit> &commits)
  {
    QList<Badge::Label> refs;
    foreach (const git::Commit &commit, commits) {
      foreach (const git::Reference &ref, commit.refs())
        refs.append({ref.name(), ref.isHead(), ref.isTag()});
    }

    mRefs->setLabels(refs);

    // Compute description asynchronously.
    if (commits.size() == 1)
      mWatcher.setFuture(
        QtConcurrent::run(commits.first(), &git::Commit::description));
  }

  void setCommits(const QList<git::Commit> &commits)
  {
    // Clear fields.
    mHash->setText(QString());
    mAuthorDate->setDate(QString());
    mAuthorDate->setAuthor(QString());
    mParents->setText(QString());
    mMessage->setPlainText(QString());
    mPicture->setPixmap(QPixmap());

    mParents->setVisible(false);
    mSeparator->setVisible(false);
    mMessage->setVisible(false);

    // Reset references.
    setReferences(commits);

    if (commits.isEmpty())
      return;

    // Show range details.
    if (commits.size() > 1) {
      git::Commit last = commits.last();
      git::Commit first = commits.first();

      // Add names.
      QSet<QString> names;
      foreach (const git::Commit &commit, commits)
        names.insert(kBoldFmt.arg(commit.author().name()));
      QStringList list = names.values();
      if (list.size() > 3)
        list = list.mid(0, 3) << kBoldFmt.arg("...");
      mAuthorDate->setAuthor(list.join(", "));

      // Set date range.
      QDate lastDate = last.committer().date().date();
      QDate firstDate = first.committer().date().date();
      QString lastDateStr = lastDate.toString(Qt::DefaultLocaleShortDate);
      QString firstDateStr = firstDate.toString(Qt::DefaultLocaleShortDate);
      QString dateStr = (lastDate == firstDate) ? lastDateStr :
        kDateRangeFmt.arg(lastDateStr, firstDateStr);
      mAuthorDate->setDate(brightText(dateStr));

      // Set id range.
      QUrl lastUrl;
      lastUrl.setScheme("id");
      lastUrl.setPath(last.id().toString());
      QString lastId = kLinkFmt.arg(lastUrl.toString(), last.shortId());

      QUrl firstUrl;
      firstUrl.setScheme("id");
      firstUrl.setPath(first.id().toString());
      QString firstId = kLinkFmt.arg(firstUrl.toString(), first.shortId());

      QString range = kRangeFmt.arg(lastId, firstId);
      mHash->setText(brightText(tr("Range:")) + " " + range);

      // Remember the range.
      mId = kRangeFmt.arg(last.id().toString(), first.id().toString());

      return;
    }

    // Show commit details.
    mParents->setVisible(true);
    mSeparator->setVisible(true);
    mMessage->setVisible(true);

    // Populate details.
    git::Commit commit = commits.first();
    git::Signature author = commit.author();
    QDateTime date = commit.committer().date();
    mHash->setText(brightText(tr("Id:")) + " " + commit.shortId());
    mAuthorDate->setDate(brightText(date.toString(Qt::DefaultLocaleLongDate)));
    mAuthorDate->setAuthor(kAuthorFmt.arg(author.name(), author.email()));

    QStringList parents;
    foreach (const git::Commit &parent, commit.parents()) {
      QUrl url;
      url.setScheme("id");
      url.setPath(parent.id().toString());
      parents.append(kLinkFmt.arg(url.toString(), parent.shortId()));
    }

    QString initial = kItalicFmt.arg(tr("initial commit"));
    QString text = parents.isEmpty() ? initial : parents.join(", ");
    mParents->setText(brightText("Parents:") + " " + text);

    QString msg = commit.message(git::Commit::SubstituteEmoji).trimmed();
    mMessage->setPlainText(msg);

    int size = kSize * window()->windowHandle()->devicePixelRatio();
    QByteArray email = commit.author().email().trimmed().toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(email, QCryptographicHash::Md5);

    // Check the cache first.
    QByteArray key = hash.toHex() + '@' + QByteArray::number(size);
    mPicture->setPixmap(mCache.value(key));

    // Request the image from gravatar.
    if (!mCache.contains(key)) {
      QUrl url(kUrl.arg(QString::fromUtf8(hash.toHex()), QString::number(size)));
      QNetworkReply *reply = mMgr.get(QNetworkRequest(url));
      reply->setProperty(kCacheKey, key);
    }

    // Remember the id.
    mId = commit.id().toString();
  }

  void setPicture(QNetworkReply *reply)
  {
    // Load source.
    QPixmap source;
    source.loadFromData(reply->readAll());

    // Render clipped to circle.
    QPixmap pixmap(source.size());
    pixmap.fill(Qt::transparent);

    // Clip to path. The region overload doesn't antialias.
    QPainterPath path;
    path.addEllipse(pixmap.rect());

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, source);

    // Cache the transformed pixmap.
    pixmap.setDevicePixelRatio(window()->windowHandle()->devicePixelRatio());
    mCache.insert(reply->property(kCacheKey).toByteArray(), pixmap);
    mPicture->setPixmap(pixmap);
    reply->deleteLater();
  }

  void cancelBackgroundTasks()
  {
    // Just wait.
    mWatcher.waitForFinished();
  }

private:
  Badge *mRefs;
  QLabel *mHash;
  QLabel *mParents;
  QLabel *mPicture;
  QFrame *mSeparator;
  QTextEdit *mMessage;
  AuthorDate *mAuthorDate;

  QString mId;
  QNetworkAccessManager mMgr;
  QMap<QByteArray,QPixmap> mCache;
  QFutureWatcher<QString> mWatcher;
};

class ElidedLabel : public QLabel
{
public:
  ElidedLabel(const QString &text,
              const QString &shorttext = QString(),
              const Qt::TextElideMode &elidemode = Qt::ElideMiddle,
              QWidget *parent = nullptr)
    : QLabel(parent), mText(text), mShortText(shorttext), mElideMode(elidemode)
  {
    QLabel(text, parent);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  }

  ElidedLabel(const QString &text,
              const Qt::TextElideMode &elidemode = Qt::ElideMiddle,
              QWidget *parent = nullptr)
    : QLabel(parent), mText(text), mElideMode(elidemode)
  {
    QLabel(text, parent);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  }

  void setText(const QString &text,
               const QString &shorttext = QString())
  {
    mText = text;
    mShortText = shorttext;

    QFontMetrics fm = getFontMetric();
    QString plain = QTextDocumentFragment::fromHtml(text).toPlainText();
    mWidth = fm.boundingRect(plain).width() + fm.averageCharWidth();

    QLabel::setText(text);
  }

  void setElideMode(const Qt::TextElideMode &elidemode)
  {
    mElideMode = elidemode;
    repaint();
  }

  QSize sizeHint() const override
  {
    if (mElided)
      return QSize(mWidth, QLabel::sizeHint().height());

    return QLabel::sizeHint();
  }

  QSize minimumSizeHint() const override
  {
    if (mElided && !mShortText.isEmpty())
      return QLabel::minimumSizeHint();

    QFontMetrics fm = getFontMetric();

    return QSize(fm.boundingRect("...").width() + fm.averageCharWidth(), QLabel::minimumSizeHint().height());
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    QFontMetrics fm = getFontMetric();
    QString plain = QTextDocumentFragment::fromHtml(mText).toPlainText();
    QString elide = fm.elidedText(plain, mElideMode, width());
    if (plain.length() > elide.length()) {
      // Use short text or elide text.
      if (!mShortText.isEmpty())
        QLabel::setText(mShortText);
      else {
        QString text = mText;
        QLabel::setText(text.replace(plain, elide));
      }
      mElided = true;
    } else {
      // Use text.
      QLabel::setText(mText);
      mElided = false;
      mWidth = QLabel::width();
    }

    QLabel::paintEvent(event);
  }

private:
  QFontMetrics getFontMetric(void) const
  {
    QFont font = QLabel::font();

    // Detect common HTML tags.
    font.setBold(font.bold() || mText.contains("<b>"));
    font.setItalic(font.italic() || mText.contains("<i>"));

    QFontMetrics fm(font);

    return fm;
  }

  QString mText;
  QString mShortText;

  Qt::TextElideMode mElideMode = Qt::ElideMiddle;

  bool mElided = false;
  int mWidth = 10000;
};

class TextEdit : public QTextEdit
{
  Q_OBJECT

public:
  explicit TextEdit(QWidget *parent = nullptr)
    : QTextEdit(parent)
  {
    // Spell check with delay timeout.
    connect(&mTimer, &QTimer::timeout, [this] {
      mTimer.stop();
      if (mSpellCheckValid)
        checkspelling();
    });
  }

  void SpellCheckSetup(const QString &dictPath,
                       const QString &userDict,
                       const QTextCharFormat &spellFormat,
                       const QTextCharFormat &ignoredFormat)
  {
    mSpellChecker = new SpellChecker(dictPath, userDict);
    if (mSpellChecker->isValid()) {
      mSpellFormat = spellFormat;
      mIgnoredFormat = ignoredFormat;
      checkspelling();
      mSpellCheckValid = true;
    } else {
      mSpellList.clear();
      setselections();
      mSpellCheckValid = false;
    }
  }

  void SpellCheck(void)
  {
    if (mSpellCheckValid)
      checkspelling();
  }

  void LineLengthSetup(const QList<int> &lineLength,
                       const QList<int> &blankLines,
                       const QTextCharFormat &lineFormat)
  {
    // Length set or blank line enabled.
    if (lineLength.count() || mBlankLines.count()) {
      mLineLength = lineLength;
      mBlankLines = blankLines;
      mLineFormat = lineFormat;
      checklength();
      mLineLengthCheckValid = true;
    } else {
      mLineList.clear();
      setselections();
      mLineLengthCheckValid = false;
    }
  }

  void LineLengthCheck(void)
  {
    if (mLineLengthCheckValid)
      checklength();
  }

private:
  void contextMenuEvent(QContextMenuEvent *event) override
  {
    QMenu *menu = createStandardContextMenu();
    bool replaced = false;

    // Spell checking enabled.
    if (mSpellCheckValid) {
      QTextCursor cursor = cursorForPosition(event->pos());
      cursor.select(QTextCursor::WordUnderCursor);
      QString word = cursor.selectedText();

      // Selected word under cursor.
      if (!word.isEmpty()) {
        foreach (QTextEdit::ExtraSelection es, mSpellList) {
          if ((es.cursor == cursor) &&
              (es.format == mSpellFormat)) {

            // Replace standard context menu.
            menu->clear();
            replaced = true;

            QStringList suggestions = mSpellChecker->suggest(word);
            if (!suggestions.isEmpty()) {
              QMenu *spellReplace = menu->addMenu(tr("Replace..."));
              QMenu *spellReplaceAll = menu->addMenu(tr("Replace All..."));
              foreach (QString str, suggestions) {
                QAction *replace = spellReplace->addAction(str);
                connect(replace, &QAction::triggered, [this, event, str] {
                  QTextCursor cursor = cursorForPosition(event->pos());
                  cursor.select(QTextCursor::WordUnderCursor);
                  cursor.insertText(str);
                  checkspelling();
                });

                QAction *replaceAll = spellReplaceAll->addAction(str);
                  connect(replaceAll, &QAction::triggered, [this, word, str] {
                  QTextCursor cursor(document());
                  while (!cursor.atEnd()) {
                    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
                    QString search = getword(cursor);
                    if (!search.isEmpty() && (search == word) && !ignoredword(cursor))
                      cursor.insertText(str);

                    cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
                  }
                  checkspelling();
                });
              }
              menu->addSeparator();
            }

            QAction *spellIgnore = menu->addAction(tr("Ignore"));
            connect(spellIgnore, &QAction::triggered, [this, event] {
              QTextCursor cursor = cursorForPosition(event->pos());
              cursor.select(QTextCursor::WordUnderCursor);

              for (int i = 0; i < mSpellList.count(); i++) {
                QTextEdit::ExtraSelection es = mSpellList.at(i);
                if (es.cursor == cursor) {
                  mSpellList.removeAt(i);
                  es.format = mIgnoredFormat;
                  mSpellList << es;

                  setselections();
                  break;
                }
              }
              checkspelling();
            });

            QAction *spellIgnoreAll = menu->addAction(tr("Ignore All"));
            connect(spellIgnoreAll, &QAction::triggered, [this, word] {
              mSpellChecker->ignoreWord(word);
              checkspelling();
            });

            QAction *spellAdd = menu->addAction(tr("Add to User Dictionary"));
            connect(spellAdd, &QAction::triggered, [this, word] {
              mSpellChecker->addToUserDict(word);
              checkspelling();
            });
            break;
          }

          // Ignored words.
          if ((es.cursor == cursor) &&
              (es.format == mIgnoredFormat)) {

            // Replace standard context menu.
            menu->clear();
            replaced = true;

            QAction *spellIgnore = menu->addAction(tr("Do not Ignore"));
            connect(spellIgnore, &QAction::triggered, [this, event] {
              QTextCursor cursor = cursorForPosition(event->pos());
              cursor.select(QTextCursor::WordUnderCursor);

              for (int i = 0; i < mSpellList.count(); i++) {
                QTextEdit::ExtraSelection es = mSpellList.at(i);
                if (es.cursor == cursor) {
                  mSpellList.removeAt(i);

                  setselections();
                  break;
                }
              }
              checkspelling();
            });
            break;
          }
        }
      }
    }

    // Line length checking enabled.
    if (mLineLengthCheckValid) {
      QTextCursor cursor = cursorForPosition(event->pos());
      int row = cursor.blockNumber();
      foreach (QTextEdit::ExtraSelection es, mLineList) {
        if ((es.cursor.position() <= cursor.position()) &&
            (es.cursor.anchor() >= cursor.position())) {

          // Replace standard context menu.
          if (replaced)
            menu->addSeparator();
          else {
            menu->clear();
            replaced = true;
          }

          QAction *lineWrap;
          if (mBlankLines.contains(row + 1))
            lineWrap = menu->addAction(tr("Truncate Line"));
          else
            lineWrap = menu->addAction(tr("Insert Wordwrap"));

          cursor = es.cursor;
          connect(lineWrap, &QAction::triggered, [this, cursor, row] {
            wordwrap(cursor, row, true);
            checklength();
            checkspelling();
          });

          if (!mBlankLines.contains(row + 1)) {
            QAction *lineAll = menu->addAction(tr("Insert All Wordwraps"));
            connect(lineAll, &QAction::triggered, [this] {
              wordwraps();
              checklength();
              checkspelling();
            });
          }
          break;
        }
      }
    }
    menu->exec(event->globalPos());
    delete menu;
  }

  void keyPressEvent(QKeyEvent *event) override
  {
    QTextEdit::keyPressEvent(event);

    QString text = event->text();
    if (text.length()) {
      QChar chr = text.at(0);

      // Spell check.
      if (chr.isLetter() || chr.isNumber())
        mTimer.start(500);
      else if (mSpellCheckValid && !event->isAutoRepeat())
        checkspelling();

      // Line length check.
      if (mLineLengthCheckValid)
        checklength();
    }
  }

  void checkspelling(void)
  {
    QTextCursor cursor(document());
    mSpellList.clear();

    while (!cursor.atEnd()) {
      cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
      QString word = getword(cursor);
      if (!word.isEmpty() && !mSpellChecker->spell(word)) {

        // Highlight the unknown or ignored word.
        QTextEdit::ExtraSelection es;
        es.cursor = cursor;
        if (ignoredword(cursor))
          es.format = mIgnoredFormat;
        else
          es.format = mSpellFormat;

        mSpellList << es;
      }
      cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
    setselections();
  }

  bool ignoredword(const QTextCursor &cursor)
  {
    foreach (QTextEdit::ExtraSelection es, extraSelections()) {
      if ((es.cursor == cursor) &&
          (es.format == mIgnoredFormat))
        return true;
    }
    return false;
  }

  const QString getword(QTextCursor &cursor)
  {
    QString word = cursor.selectedText();

    // For a better recognition of words
    // punctuation etc. does not belong to words.
    while (!word.isEmpty() && !word.at(0).isLetter() &&
           (cursor.anchor() < cursor.position())) {
      int cursorPos = cursor.position();
      cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
      cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
      word = cursor.selectedText();
    }
    return word;
  }

  void checklength(void)
  {
    QTextCursor cursor(document());
    QStringList strlist = toPlainText().split('\n');
    mLineList.clear();

    for (int row = 0; row < strlist.count(); row++) {
      int len = strlist[row].length();

      // Forced blank lines.
      if (mBlankLines.contains(row) && (len > 0)) {
          cursor.insertText("\n");
        break;
      }

      int limit;
      if (row >= mLineLength.count())
        limit = mLineLength.last();
      else
        limit = mLineLength.at(row);

      cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor, 1);
      if ((limit > 0) && (len > limit)) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, len - limit);

        // Highlight length violation.
        QTextEdit::ExtraSelection es;
        es.cursor = cursor;
        es.format = mLineFormat;
        mLineList << es;

        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor, 1);
      }
      cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 1);
    }
    setselections();
  }

  bool wordwrap(QTextCursor cursor, int row, bool truncate)
  {
    QStringList strlist = toPlainText().split('\n');
    if (row <= strlist.count()) {
      int limit;
      if (row >= mLineLength.count())
        limit = mLineLength.last();
      else
        limit = mLineLength.at(row);

      if (mBlankLines.contains(row + 1)) {

        // Truncate selection.
        if (truncate)
          cursor.deleteChar();
        else
          return false;
      } else {

        // Insert Wordwrap.
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 1);
        if (cursor.positionInBlock() > 0) {
          cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
          cursor.insertText("\n");
        } else {
          cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, limit);
          cursor.insertText("\n");
        }
      }
      return true;
    }
    return false;
  }

  void wordwraps(void)
  {
    int index = 0;
    while (index < mLineList.count()) {
      QTextEdit::ExtraSelection es = mLineList.at(index);
      int row = es.cursor.blockNumber();
      if (!wordwrap(es.cursor, row, false))
        index += 1;
      else
        checklength();
    }
  }

  void setselections(void)
  {
    QList<QTextEdit::ExtraSelection> esList;
    esList.append(mLineList);
    esList.append(mSpellList);
    setExtraSelections(esList);
  }

  QTimer mTimer;

  SpellChecker *mSpellChecker = nullptr;
  QTextCharFormat mSpellFormat;
  QTextCharFormat mIgnoredFormat;
  QList<QTextEdit::ExtraSelection> mSpellList;
  bool mSpellCheckValid = false;

  QList<int> mLineLength;
  QList<int> mBlankLines;
  QTextCharFormat mLineFormat;
  QList<QTextEdit::ExtraSelection> mLineList;
  bool mLineLengthCheckValid = false;
};

class CommitEditor : public QFrame
{
  Q_OBJECT

public:
  CommitEditor(const git::Repository &repo, QWidget *parent = nullptr)
    : QFrame(parent), mRepo(repo)
  {
    ElidedLabel *label = new ElidedLabel(kBoldFmt.arg(tr("Commit Message:")), Qt::ElideLeft, this);
    label->setAlignment(Qt::AlignLeft);

    // Read configuration.
    git::Config appconfig = repo.appConfig();
    mDict = appconfig.value<QString>(kDictKey, "-----");
    mDictPath = Settings::dictionariesDir().path();
    mUserDict = Settings::userDir().path() + "/user.dic";
    QFile userdict(mUserDict);
    if (!userdict.exists()) {
      userdict.open(QIODevice::WriteOnly);
      userdict.close();
    }

    // Style and color setup for checks.
    mSpellError.setUnderlineColor(Application::theme()->diff(Theme::Diff::Error));
    mSpellError.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    mSpellIgnore.setUnderlineColor(Application::theme()->diff(Theme::Diff::Note));
    mSpellIgnore.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    mLineLengthWarn.setBackground(Application::theme()->diff(Theme::Diff::Warning));

    mLengthLabel = new ElidedLabel(QString(), QString(), Qt::ElideLeft, this);
    mLengthLabel->setAlignment(Qt::AlignRight);

    mLengthSpin = new QSpinBox(this);
    mLengthSpin->setRange(0, 999);
    mLengthSpin->setValue(mSubjectLimit);
    mLengthSpin->setStyleSheet(kSpin);
    mLengthSpin->setToolTip(tr("Line Length Limit"));
    connect(mLengthSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
      int row = mMessage->textCursor().blockNumber();
      if (row == (mInsertBlank->isChecked() ? 1 : -1))
        return;

      // Save settings.
      git::Config appconfig = mRepo.appConfig();
      if (row == 0) {
        mSubjectLimit = value;
        appconfig.setValue(kSubjectLimitKey, value);
      } else {
        mBodyLimit = value;
        appconfig.setValue(kBodyLimitKey, value);
      }

      // Apply changes.
      updateChecks(false, true);
    });

    // Dictionary.
    mDictBox = new QComboBox(this);
    mDictBox->setStyleSheet(kComboBox);
    mDictBox->setToolTip(tr("Spell Check Language"));
    QDir dictdir = Settings::dictionariesDir();
    QStringList dictlist = dictdir.entryList({"*.dic"}, QDir::Files, QDir::Name);
    if (dictlist.count() > 0) {
      mDictBox->addItem("-----");
      dictlist.replaceInStrings(".dic", "");
      mDictBox->addItems(dictlist);
      mDictBox->setCurrentText(mDict);
    }
    mDictBox->setVisible(mDictBox->count() > 0);
    connect(mDictBox, &QComboBox::currentTextChanged, [this](QString text) {
      mDict = text;

      // Save settings.
      git::Config appconfig = mRepo.appConfig();
      appconfig.setValue(kDictKey, text);

      // Apply changes.
      updateChecks(true, false);
    });

    mStatus = new ElidedLabel(QString(), Qt::ElideRight, this);
    mStatus->setAlignment(Qt::AlignRight);

    // Context button.
    ContextMenuButton *button = new ContextMenuButton(this);
    QMenu *menu = new QMenu(this);
    button->setMenu(menu);

    mSubjectCheck = menu->addAction(tr("Subject Line Length Check"), [this] {
      // Save settings.
      git::Config appconfig = mRepo.appConfig();
      appconfig.setValue(kSubjectCheckKey, mSubjectCheck->isChecked());

      // Apply changes.
      if (mSubjectCheck->isChecked())
        mSubjectLimit = appconfig.value<int>(kSubjectLimitKey, 0);
      else
        mSubjectLimit = 0;

      updateChecks(false, true);
    });
    mSubjectCheck->setCheckable(true);
    mSubjectCheck->setChecked(appconfig.value<bool>(kSubjectCheckKey, false));
    if (mSubjectCheck->isChecked())
      mSubjectLimit = appconfig.value<int>(kSubjectLimitKey, 0);

    mInsertBlank = menu->addAction(tr("Insert Blank Line between Subject and Body"), [this] {
      // Save settings.
      git::Config appconfig = mRepo.appConfig();
      appconfig.setValue(kBlankKey, mInsertBlank->isChecked());

      // Apply changes.
      updateChecks(false, true);
    });
    mInsertBlank->setCheckable(true);
    mInsertBlank->setChecked(appconfig.value<bool>(kBlankKey, false));

    mBodyCheck = menu->addAction(tr("Body Text Length Check"), [this] {
      // Save settings.
      git::Config appconfig = mRepo.appConfig();
      appconfig.setValue(kBodyCheckKey, mBodyCheck->isChecked());

      // Apply changes.
      if (mBodyCheck->isChecked())
        mBodyLimit = appconfig.value<int>(kBodyLimitKey, 0);
      else
        mBodyLimit = 0;

      updateChecks(false, true);
    });
    mBodyCheck->setCheckable(true);
    mBodyCheck->setChecked(appconfig.value<bool>(kBodyCheckKey, false));
    if (mBodyCheck->isChecked())
      mBodyLimit = appconfig.value<int>(kBodyLimitKey, 0);

    QHBoxLayout *labelLayout = new QHBoxLayout;
    labelLayout->addWidget(label);
    labelLayout->addStretch();
    labelLayout->addWidget(mLengthLabel);
    labelLayout->addWidget(mLengthSpin);
    labelLayout->addWidget(mDictBox);
    labelLayout->addStretch();
    labelLayout->addWidget(mStatus);
    labelLayout->addWidget(button);

    mMessage = new TextEdit(this);
    mMessage->setAcceptRichText(false);
    mMessage->setObjectName("MessageEditor");
    mMessage->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    connect(mMessage, &QTextEdit::textChanged, [this] {
      mPopulate = false;

      bool empty = mMessage->toPlainText().isEmpty();
      if (mEditorEmpty != empty)
        updateButtons();
      mEditorEmpty = empty;

      mPopulate = mMessage->toPlainText().isEmpty();
    });

    // Update header on cursor position change.
    connect(mMessage, &QTextEdit::cursorPositionChanged, [this] {
      updateChecks(false, false);
    });
    updateChecks(true, true);

    // Update menu items.
    MenuBar *menuBar = MenuBar::instance(this);
    connect(mMessage, &QTextEdit::undoAvailable,
            menuBar, &MenuBar::updateUndoRedo);
    connect(mMessage, &QTextEdit::redoAvailable,
            menuBar, &MenuBar::updateUndoRedo);
    connect(mMessage, &QTextEdit::copyAvailable,
            menuBar, &MenuBar::updateCutCopyPaste);

    QVBoxLayout *messageLayout = new QVBoxLayout;
    messageLayout->setContentsMargins(12,8,0,0);
    messageLayout->addLayout(labelLayout);
    messageLayout->addWidget(mMessage);

    mStage = new QPushButton(tr("Stage All"), this);
    mStage->setObjectName("StageAll");
    connect(mStage, &QPushButton::clicked, this, &CommitEditor::stage);

    mUnstage = new QPushButton(tr("Unstage All"), this);
    connect(mUnstage, &QPushButton::clicked, this, &CommitEditor::unstage);

    mCommit = new QPushButton(tr("Commit"), this);
    mCommit->setDefault(true);
    connect(mCommit, &QPushButton::clicked, this, &CommitEditor::commit);

    // Update buttons on index change.
    connect(repo.notifier(), &git::RepositoryNotifier::indexChanged,
    [this](const QStringList &paths, bool yieldFocus) {
      updateButtons(yieldFocus);
    });

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->setContentsMargins(0,8,12,0);
    buttonLayout->addStretch();
    buttonLayout->addWidget(mStage);
    buttonLayout->addWidget(mUnstage);
    buttonLayout->addWidget(mCommit);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,12);
    layout->addLayout(messageLayout);
    layout->addLayout(buttonLayout);
  }

  void commit()
  {
    // Check for a merge head.
    git::AnnotatedCommit upstream;
    RepoView *view = RepoView::parentView(this);
    if (git::Reference mergeHead = view->repo().lookupRef("MERGE_HEAD"))
      upstream = mergeHead.annotatedCommit();

    if (view->commit(mMessage->toPlainText(), upstream))
      mMessage->clear(); // Clear the message field.
  }

  bool isCommitEnabled() const
  {
    return mCommit->isEnabled();
  }

  void stage()
  {
    mDiff.setAllStaged(true);
  }

  bool isStageEnabled() const
  {
    return mStage->isEnabled();
  }

  void unstage()
  {
    mDiff.setAllStaged(false);
  }

  bool isUnstageEnabled() const
  {
    return mUnstage->isEnabled();
  }

  void setMessage(const QString &message)
  {
    mMessage->setPlainText(message);
    mMessage->selectAll();
  }

  void setDiff(const git::Diff &diff)
  {
    mDiff = diff;
    updateButtons(false);

    // Pre-populate commit editor with the merge message.
    QString msg = RepoView::parentView(this)->repo().message();
    if (!msg.isEmpty())
      mMessage->setPlainText(msg);
  }

private:
  void updateButtons(bool yieldFocus = true)
  {
    if (!mDiff.isValid()) {
      mStage->setEnabled(false);
      mUnstage->setEnabled(false);
      mCommit->setEnabled(false);
      return;
    }

    QStringList list;
    int staged = 0;
    int partial = 0;
    int conflicted = 0;
    int count = mDiff.count();
    git::Index index = mDiff.index();
    for (int i = 0; i < count; ++i) {
      QString name = mDiff.name(i);
      switch (index.isStaged(name)) {
        case git::Index::Disabled:
        case git::Index::Unstaged:
          break;

        case git::Index::PartiallyStaged:
          list.append(QFileInfo(name).fileName());
          ++partial;
          break;

        case git::Index::Staged:
          list.append(QFileInfo(name).fileName());
          ++staged;
          break;

        case git::Index::Conflicted:
          ++conflicted;
          break;
      }
    }

    if (mPopulate) {
      QSignalBlocker blocker(mMessage);
      (void) blocker;

      QString msg;
      switch (list.size()) {
        case 0:
          break;

        case 1:
          msg = tr("Update %1").arg(list.first());
          break;

        case 2:
          msg = tr("Update %1 and %2").arg(list.first(), list.last());
          break;

        case 3:
          msg = tr("Update %1, %2, and %3").arg(
                  list.at(0), list.at(1), list.at(2));
          break;

        default:
          msg = tr("Update %1, %2, and %3 more files...").arg(
                  list.at(0), list.at(1), QString::number(list.size() - 2));
          break;
      }

      setMessage(msg);
      if (yieldFocus && !mMessage->toPlainText().isEmpty())
        mMessage->setFocus();
    }

    int total = staged + partial + conflicted;
    mStage->setEnabled(count > staged);
    mUnstage->setEnabled(total);
    mCommit->setEnabled(total && !mMessage->document()->isEmpty());

    // Set status text.
    QString status = tr("Nothing staged");
    if (staged || partial || conflicted) {
      QString fmt = (staged == 1 && count == 1) ?
        tr("%1 of %2 file staged") : tr("%1 of %2 files staged");
      QStringList fragments(fmt.arg(staged).arg(count));

      if (partial) {
        QString partialFmt = (partial == 1) ?
          tr("%1 file partially staged") : tr("%1 files partially staged");
        fragments.append(partialFmt.arg(partial));
      }

      if (conflicted) {
        QString conflictedFmt = (conflicted == 1) ?
          tr("%1 unresolved conflict") : tr("%1 unresolved conflicts");
        fragments.append(conflictedFmt.arg(conflicted));
      } else if (mDiff.isConflicted()) {
        fragments.append(tr("all conflicts resolved"));
      }

      status = fragments.join(", ");
    }

    mStatus->setText(brightText(status));

    // Change commit button text for committing a merge.
    git::Repository repo = RepoView::parentView(this)->repo();
    bool merging = (repo.state() == GIT_REPOSITORY_STATE_MERGE);
    mCommit->setText(merging ? tr("Commit Merge") : tr("Commit"));

    // Update menu actions.
    MenuBar::instance(this)->updateRepository();
  }

  void updateChecks(bool spellchecker, bool linelengthchecker)
  {
    int row = mMessage->textCursor().blockNumber();
    QStringList strlist = mMessage->toPlainText().split('\n');
    int len = strlist[row].length();

    // Setup header.
    switch (row) {
      case 0:
        if (mSubjectCheck->isChecked()) {
          QString text = QString::number(len) + "/";
          mLengthLabel->setText(tr("Subject:") + " " + text, text);
          mLengthSpin->setValue(mSubjectLimit);
        }
        mLengthLabel->setVisible(mSubjectCheck->isChecked());
        mLengthSpin->setVisible(mSubjectCheck->isChecked());
        mLengthSpin->setEnabled(true);
        break;

      case 1:
        if (mInsertBlank->isChecked()) {
          mLengthLabel->setText(tr("Blank:") + " 0/", QString("0 /"));
          mLengthSpin->setValue(0);
          mLengthLabel->setVisible(true);
          mLengthSpin->setVisible(true);
          mLengthSpin->setEnabled(false);
          break;
        }
        // Fall through.

      default:
        if (mBodyCheck->isChecked()) {
          QString text = QString::number(len) + "/";
          mLengthLabel->setText(tr("Body:") + " " + text, text);
          mLengthSpin->setValue(mBodyLimit);
        }
        mLengthLabel->setVisible(mBodyCheck->isChecked());
        mLengthSpin->setVisible(mBodyCheck->isChecked());
        mLengthSpin->setEnabled(true);
        break;
    }

    // Setup checks.
    if (spellchecker)
      mMessage->SpellCheckSetup(mDictPath + "/" + mDict,
                                mUserDict,
                                mSpellError,
                                mSpellIgnore);

    if (linelengthchecker)
      mMessage->LineLengthSetup({mSubjectLimit, mBodyLimit},
                                {mInsertBlank->isChecked() ? 1 : -1},
                                mLineLengthWarn);
  }

  git::Repository mRepo;
  git::Diff mDiff;

  ElidedLabel *mLengthLabel;
  QSpinBox *mLengthSpin;
  QComboBox *mDictBox;
  ElidedLabel *mStatus;

  QAction *mSubjectCheck;
  QAction *mInsertBlank;
  QAction *mBodyCheck;

  TextEdit *mMessage;
  QPushButton *mStage;
  QPushButton *mUnstage;
  QPushButton *mCommit;

  bool mEditorEmpty = true;
  bool mPopulate = true;
  int mSubjectLimit = 0;
  int mBodyLimit = 0;
  QString mDict;
  QString mDictPath;
  QString mUserDict;

  QTextCharFormat mSpellError;
  QTextCharFormat mSpellIgnore;
  QTextCharFormat mLineLengthWarn;
};

} // anon. namespace

ContentWidget::ContentWidget(QWidget *parent)
  : QWidget(parent)
{}

ContentWidget::~ContentWidget() {}

DetailView::DetailView(const git::Repository &repo, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  mDetail = new StackedWidget(this);
  mDetail->setVisible(false);
  layout->addWidget(mDetail);

  mDetail->addWidget(new CommitDetail(this));
  mDetail->addWidget(new CommitEditor(repo, this));

  mContent = new QStackedWidget(this);
  layout->addWidget(mContent, 1);

  mContent->addWidget(new DiffWidget(repo, this));
  mContent->addWidget(new TreeWidget(repo, this));
}

DetailView::~DetailView() {}

void DetailView::commit()
{
  Q_ASSERT(isCommitEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->commit();
}

bool DetailView::isCommitEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isCommitEnabled());
}

void DetailView::stage()
{
  Q_ASSERT(isStageEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->stage();
}

bool DetailView::isStageEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isStageEnabled());
}

void DetailView::unstage()
{
  Q_ASSERT(isUnstageEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->unstage();
}

bool DetailView::isUnstageEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isUnstageEnabled());
}

RepoView::ViewMode DetailView::viewMode() const
{
  return static_cast<RepoView::ViewMode>(mContent->currentIndex());
}

void DetailView::setViewMode(RepoView::ViewMode mode, bool spontaneous)
{
  if (mode == mContent->currentIndex())
    return;

  mContent->setCurrentIndex(mode);

  // Emit own signal so that the view can respond *after* index change.
  emit viewModeChanged(mode, spontaneous);
}

QString DetailView::file() const
{
  return static_cast<ContentWidget *>(mContent->currentWidget())->selectedFile();
}

void DetailView::setCommitMessage(const QString &message)
{
  static_cast<CommitEditor *>(mDetail->widget(EditorIndex))->setMessage(message);
}

void DetailView::setDiff(
  const git::Diff &diff,
  const QString &file,
  const QString &pathspec)
{
  RepoView *view = RepoView::parentView(this);
  QList<git::Commit> commits = view->commits();

  mDetail->setCurrentIndex(commits.isEmpty() ? EditorIndex : CommitIndex);
  mDetail->setVisible(diff.isValid());

  if (commits.isEmpty()) {
    static_cast<CommitEditor *>(mDetail->currentWidget())->setDiff(diff);
  } else {
    static_cast<CommitDetail *>(mDetail->currentWidget())->setCommits(commits);
  }

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->setDiff(diff, file, pathspec);

  // Update menu actions.
  MenuBar::instance(this)->updateRepository();
}

void DetailView::cancelBackgroundTasks()
{
  CommitDetail *cd = static_cast<CommitDetail *>(mDetail->widget(CommitIndex));
  cd->cancelBackgroundTasks();

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->cancelBackgroundTasks();
}

void DetailView::find()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->find();
}

void DetailView::findNext()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->findNext();
}

void DetailView::findPrevious()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->findPrevious();
}

#include "DetailView.moc"

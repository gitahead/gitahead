//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

// Do not reorder.
#include <cstddef>
#include "ILexer.h"
#include "LexerModule.h"
#include "Catalogue.h"
#include "SciLexer.h"
#include "ScintillaIFace.h"

class TextEditor : public Scintilla::ScintillaIFace
{
  Q_OBJECT

public:
  enum Margin {
    LineNumber,
    LineNumbers,
    ErrorMargin
  };

  enum Marker {
    Context,
    Ours,
    Theirs,
    Addition,
    Deletion,
    NoteMarker,
    WarningMarker,
    ErrorMarker
  };

  enum Indicator {
    FindAll = INDIC_CONTAINER,
    FindCurrent,
    WordAddition,
    WordDeletion,
    NoteIndicator,
    WarningIndicator,
    ErrorIndicator
  };

  enum Style {
    EofNewline = STYLE_MAX - 4,
    CommentBody,
    CommentAuthor,
    CommentTimestamp
  };

  enum DiagnosticKind {
    Note,
    Warning,
    Error
  };

  struct Range
  {
    int pos;
    int len;
  };

  struct Diagnostic
  {
    DiagnosticKind kind;
    QString message;
    QString description;

    Range range;
    QString replacement;
  };

  TextEditor(QWidget *parent = nullptr);

  void applySettings();

  QString lexer() const;

  void setLineCount(int lines);
  void setLexer(const QString &path);
  void load(const QString &path, const QString &text);

  void clearHighlights();
  int highlightAll(const QString &text);
  int find(const QString &text, bool forward = true, bool indicator = true);

  QList<Diagnostic> diagnostics(int line);
  void addDiagnostic(int line, const Diagnostic &diag);

  // Make wheel event public.
  // FIXME: This should be an event filter?
  void wheelEvent(QWheelEvent *event) override
  {
    ScintillaIFace::wheelEvent(event);
  }

  QRect textRectangle() const
  {
    Scintilla::PRectangle pr = GetTextRectangle();
    return QRect(pr.left, pr.top, pr.Width(), pr.Height());
  }

signals:
  void settingsChanged();
  void highlightActivated(bool active);
  void diagnosticAdded(int line, const Diagnostic &diag);

protected:
  QSize viewportSizeHint() const override;

private:
  int diagnosticMarker(int line);
  void loadMarkerIcon(Marker marker, const QIcon &icon);

  QString mPath;
  int mLineCount = -1;

  QColor mOursColor;
  QColor mTheirsColor;
  QColor mAdditionColor;
  QColor mDeletionColor;

  QIcon mNoteIcon;
  QIcon mWarningIcon;
  QIcon mErrorIcon;

  QMap<int,QList<Diagnostic>> mDiagnostics;
};

#endif

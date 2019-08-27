// Copyright 2012-2019 Mitchell mitchell.att.foicica.com. See License.txt.
// Scintilla implemented in a curses (terminal) environment.
// Contains platform facilities and a curses-specific subclass of ScintillaBase.
// Note: setlocale(LC_CTYPE, "") must be called before initializing curses in
// order to display UTF-8 characters properly in ncursesw.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <wchar.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "CharacterCategory.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"
#include "ScintillaCurses.h"

/**
 * Returns the given Scintilla `WindowID` as a curses `WINDOW`.
 * @param w A Scintilla `WindowID`.
 * @return curses `WINDOW`.
 */
#define _WINDOW(w) reinterpret_cast<WINDOW *>(w)

#if _WIN32
#define wcwidth(_) 1 // TODO: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
#endif

using namespace Scintilla;

// Font handling.

/**
 * Allocates a new Scintilla font for curses.
 * Since terminals handle fonts on their own, the only use for Scintilla font
 * objects is to indicate which attributes terminal characters have. This is
 * done in `Font::Create()`.
 * @see Font::Create
 */
Font::Font() noexcept : fid(0) {}
/** Deletes the font. Currently empty. */
Font::~Font() {}
/**
 * Sets terminal character attributes for a particular font.
 * These attributes are a union of curses attributes and stored in the font's
 * `fid`.
 * The curses attributes are not constructed from various fields in *fp* since
 * there is no `underline` parameter. Instead, you need to manually set the
 * `weight` parameter to be the union of your desired attributes.
 * Scintilla's lexers/LexLPeg.cxx has an example of this.
 */
void Font::Create(const FontParameters &fp) {
  Release();
  attr_t attrs = 0;
  if (fp.weight == SC_WEIGHT_BOLD)
    attrs = A_BOLD;
  else if (fp.weight != SC_WEIGHT_NORMAL && fp.weight != SC_WEIGHT_SEMIBOLD)
    attrs = fp.weight; // font attributes are stored in fp.weight
  fid = reinterpret_cast<FontID>(attrs);
}
/** Releases a font's resources. */
void Font::Release() { fid = 0; }

// Color handling.

static int COLOR_LBLACK = COLOR_BLACK + 8;
static int COLOR_LRED = COLOR_RED + 8;
static int COLOR_LGREEN = COLOR_GREEN + 8;
static int COLOR_LYELLOW = COLOR_YELLOW + 8;
static int COLOR_LBLUE = COLOR_BLUE + 8;
static int COLOR_LMAGENTA = COLOR_MAGENTA + 8;
static int COLOR_LCYAN = COLOR_CYAN + 8;
static int COLOR_LWHITE = COLOR_WHITE + 8;

static bool initialized_colors = false;

/**
 * Initializes colors in curses if they have not already been initialized.
 * Creates all possible color pairs using the `SCI_COLOR_PAIR()` macro.
 * This is called automatically from `scintilla_new()`.
 */
static void init_colors() {
  if (initialized_colors) return;
  if (has_colors()) {
    start_color();
    for (int back = 0; back < ((COLORS < 16) ? 8 : 16); back++)
      for (int fore = 0; fore < ((COLORS < 16) ? 8 : 16); fore++)
        init_pair(SCI_COLOR_PAIR(fore, back), fore, back);
    if (COLORS < 16) {
      // Do not distinguish between light and normal colors.
      COLOR_LBLACK -= 8;
      COLOR_LRED -= 8;
      COLOR_LGREEN -= 8;
      COLOR_LYELLOW -= 8;
      COLOR_LBLUE -= 8;
      COLOR_LMAGENTA -= 8;
      COLOR_LCYAN -= 8;
      COLOR_LWHITE -= 8;
    }
  }
  initialized_colors = true;
}

static ColourDesired BLACK(0, 0, 0);
static ColourDesired RED(0x80, 0, 0);
static ColourDesired GREEN(0, 0x80, 0);
static ColourDesired YELLOW(0x80, 0x80, 0);
static ColourDesired BLUE(0, 0, 0x80);
static ColourDesired MAGENTA(0x80, 0, 0x80);
static ColourDesired CYAN(0, 0x80, 0x80);
static ColourDesired WHITE(0xC0, 0xC0, 0xC0);
static ColourDesired LBLACK(0x40, 0x40, 0x40);
static ColourDesired LRED(0xFF, 0, 0);
static ColourDesired LGREEN(0, 0xFF, 0);
static ColourDesired LYELLOW(0xFF, 0xFF, 0);
static ColourDesired LBLUE(0, 0, 0xFF);
static ColourDesired LMAGENTA(0xFF, 0, 0xFF);
static ColourDesired LCYAN(0, 0xFF, 0xFF);
static ColourDesired LWHITE(0xFF, 0xFF, 0xFF);
static ColourDesired SCI_COLORS[] = {
  BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, LBLACK, LRED, LGREEN,
  LYELLOW, LBLUE, LMAGENTA, LCYAN, LWHITE
};

/**
 * Returns a curses color for the given Scintilla color.
 * Recognized colors are: black (0x000000), red (0x800000), green (0x008000),
 * yellow (0x808000), blue (0x000080), magenta (0x800080), cyan (0x008080),
 * white (0xc0c0c0), light black (0x404040), light red (0xff0000), light green
 * (0x00ff00), light yellow (0xffff00), light blue (0x0000ff), light magenta
 * (0xff00ff), light cyan (0x00ffff), and light white (0xffffff). If the color
 * is not recognized, returns `COLOR_WHITE` by default.
 * @param color Color to get a curses color for.
 * @return curses color
 */
static int term_color(ColourDesired color) {
  if (color == BLACK) return COLOR_BLACK;
  else if (color == RED) return COLOR_RED;
  else if (color == GREEN) return COLOR_GREEN;
  else if (color == YELLOW) return COLOR_YELLOW;
  else if (color == BLUE) return COLOR_BLUE;
  else if (color == MAGENTA) return COLOR_MAGENTA;
  else if (color == CYAN) return COLOR_CYAN;
  else if (color == LBLACK) return COLOR_LBLACK;
  else if (color == LRED) return COLOR_LRED;
  else if (color == LGREEN) return COLOR_LGREEN;
  else if (color == LYELLOW) return COLOR_LYELLOW;
  else if (color == LBLUE) return COLOR_LBLUE;
  else if (color == LMAGENTA) return COLOR_LMAGENTA;
  else if (color == LCYAN) return COLOR_LCYAN;
  else if (color == LWHITE) return COLOR_LWHITE;
  else return COLOR_WHITE;
}

/**
 * Returns a curses color for the given curses color.
 * This overloaded method only exists for the `term_color_pair()` macro.
 */
static int term_color(int color) { return color; }

/**
 * Returns a curses color pair from the given fore and back colors.
 * @param f Foreground color, either a Scintilla color or curses color.
 * @param b Background color, either a Scintilla color or curses color.
 * @return curses color pair suitable for calling `COLOR_PAIR()` with.
 */
#define term_color_pair(f, b) SCI_COLOR_PAIR(term_color(f), term_color(b))

// Surface handling.

/**
 * Implementation of a Scintilla surface for curses.
 * The surface is initialized with a curses `WINDOW` for drawing on. Since
 * curses can only show text, many of Scintilla's pixel-based functions are
 * not implemented.
 */
class SurfaceImpl : public Surface {
  WINDOW *win;
  PRectangle clip;

  /**
   * Returns the number of columns used to display the first UTF-8 character in
   * `s`, taking into account zero-width combining characters.
   * @param s The string that contains the first UTF-8 character to display.
   */
  int grapheme_width(const char *s) {
    wchar_t wch;
    if (mbtowc(&wch, s, MB_CUR_MAX) < 1) return 1;
    int width = wcwidth(wch);
    return width >= 0 ? width : 1;
  }
public:
  /** Allocates a new Scintilla surface for curses. */
  SurfaceImpl() : win(0) {}
  /** Deletes the surface. */
  ~SurfaceImpl() { Release(); }

  /**
   * Initializes/reinitializes the surface with a curses `WINDOW` for drawing
   * on.
   * @param wid Curses `WINDOW`.
   */
  void Init(WindowID wid) {
    Release();
    win = _WINDOW(wid);
  }
  /** Identical to `Init()` using the given curses `WINDOW`. */
  void Init(SurfaceID sid, WindowID wid) { Init(wid); }
  /** Initializing the surface as a pixmap is not implemented. */
  void InitPixMap(int width, int height, Surface *surface_, WindowID wid) {}

  /** Releases the surface's resources. */
  void Release() { win = 0; }
  /**
   * Returns `true` since this method is only called for pixmap surfaces and
   * those surfaces are not implemented.
   */
  bool Initialised() { return true; }
  /**
   * Setting the surface's foreground color is not implemented because all uses
   * in Scintilla involve special drawing that is not supported in curses.
   */
  void PenColour(ColourDesired fore) {}
  /** Unused; return value irrelevant. */
  int LogPixelsY() { return 1; }
  /** Returns 1 since font height is always 1 in curses. */
  int DeviceHeightFont(int points) { return 1; }
  /**
   * Moving to a particular position is not implemented because all uses in
   * Scintilla involve subsequent calls to `LineTo()`, which is also
   * unimplemented.
   */
  void MoveTo(int x_, int y_) {}
  /**
   * Drawing lines is not implemented because more often than not lines are
   * being drawn for decoration (e.g. line markers, underlines, indicators,
   * arrows, etc.).
   */
  void LineTo(int x_, int y_) {}
  /**
   * Draws the character equivalent of shape outlined by the given polygon's
   * points.
   * Scintilla only calls this method for CallTip arrows.
   * Line markers that Scintilla would normally draw as polygons are handled in
   * `DrawLineMarker()`.
   */
  void Polygon(Point *pts, size_t npts, ColourDesired fore,
               ColourDesired back) {
    wattr_set(win, 0, term_color_pair(back, COLOR_WHITE), NULL); // invert
    if (pts[0].y < pts[npts - 1].y) // up arrow
      mvwaddstr(win, pts[0].y, pts[npts - 1].x + 1, "▲");
    else if (pts[0].y > pts[npts - 1].y) // down arrow
      mvwaddstr(win, pts[0].y - 2, pts[npts - 1].x + 1, "▼");
  }
  /**
   * Scintilla will never call this method.
   * Line markers that Scintilla would normally draw as rectangles are handled
   * in `DrawLineMarker()`.
   */
  void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {}
  /**
   * Clears the given portion of the screen with the given background color.
   * In some cases, it can be determined that whitespace is being drawn. If so,
   * draw it appropriately instead of clearing the given portion of the screen.
   */
  void FillRectangle(PRectangle rc, ColourDesired back) {
    wattr_set(win, 0, term_color_pair(COLOR_WHITE, back), NULL);
    chtype ch = ' ';
    if (fabs(rc.left - (int)rc.left) > 0.1) {
      // If rc.left is a fractional value (e.g. 4.5) then whitespace dots are
      // being drawn. Draw them appropriately.
      // TODO: set color to vs.whitespaceColours.fore and back.
      wcolor_set(win, term_color_pair(COLOR_BLACK, COLOR_BLACK), NULL);
      rc.right = (int)rc.right, ch = ACS_BULLET | A_BOLD;
    }
    for (int y = rc.top; y < rc.bottom; y++)
      for (int x = rc.left; x < rc.right; x++)
        mvwaddch(win, y, x, ch);
  }
  /**
   * Instead of filling a portion of the screen with a surface pixmap, fills the
   * the screen portion with black.
   */
  void FillRectangle(PRectangle rc, Surface &surfacePattern) {
    FillRectangle(rc, BLACK);
  }
  /**
   * Scintilla will never call this method.
   * Line markers that Scintilla would normally draw as rounded rectangles are
   * handled in `DrawLineMarker()`.
   */
  void RoundedRectangle(PRectangle rc, ColourDesired fore,
                        ColourDesired back) {}
  /**
   * Drawing alpha rectangles is not fully supported.
   * Instead, fills the background color of the given rectangle with the fill
   * color, emulating INDIC_STRAIGHTBOX with no transparency.
   * This is called by Scintilla to draw INDIC_ROUNDBOX and INDIC_STRAIGHTBOX
   * indicators, text blobs, and translucent line states and selections.
   */
  void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill,
                      int alphaFill, ColourDesired outline, int alphaOutline,
                      int flags) {
    for (int x = rc.left, y = rc.top - 1; x < rc.right; x++) {
      attr_t attrs = mvwinch(win, y, x) & A_ATTRIBUTES;
      short pair = PAIR_NUMBER(attrs), fore, unused;
      if (pair > 0) pair_content(pair, &fore, &unused);
      mvwchgat(win, y, x, 1, attrs, term_color_pair(fore, fill), NULL);
    }
  }
  /** Drawing gradients is not implemented. */
  void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops,
                         GradientOptions options) {}
  /** Drawing images is not implemented. */
  void DrawRGBAImage(PRectangle rc, int width, int height,
                     const unsigned char *pixelsImage) {}
  /**
   * Scintilla will never call this method.
   * Line markers that Scintilla would normally draw as circles are handled in
   * `DrawLineMarker()`.
   */
  void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {}
  /**
   * Draw an indentation guide.
   * Scintilla will only call this method when drawing indentation guides or
   * during certain drawing operations when double buffering is enabled. Since
   * the latter is not supported, assume the former.
   */
  void Copy(PRectangle rc, Point from, Surface &surfaceSource) {
    // TODO: handle indent guide highlighting.
    wattr_set(win, 0, term_color_pair(COLOR_BLACK, COLOR_BLACK), NULL);
    mvwaddch(win, rc.top, rc.left - 1, '|' | A_BOLD);
  }

  /**
   * Draws the given text at the given position on the screen with the given
   * foreground and background colors.
   * Takes into account any clipping boundaries previously specified.
   */
  void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase,
                      const char *s, int len, ColourDesired fore,
                      ColourDesired back) {
    intptr_t attrs = reinterpret_cast<intptr_t>(font_.GetID());
    wattr_set(win, static_cast<attr_t>(attrs), term_color_pair(fore, back),
              NULL);
    if (rc.left < clip.left) {
      // Do not overwrite margin text.
      int clip_chars = static_cast<int>(clip.left - rc.left);
      int offset = 0;
      for (int chars = 0; offset < len; offset++) {
        if (!UTF8IsTrailByte((unsigned char)s[offset]))
          chars += grapheme_width(s + offset);
        if (chars > clip_chars) break;
      }
      s += offset, len -= offset, rc.left = clip.left;
    }
    // Do not write beyond right window boundary.
    int clip_chars = getmaxx(win) - rc.left;
    int bytes = 0;
    for (int chars = 0; bytes < len; bytes++) {
      if (!UTF8IsTrailByte((unsigned char)s[bytes]))
        chars += grapheme_width(s + bytes);
      if (chars > clip_chars) break;
    }
    mvwaddnstr(win, rc.top, rc.left, s, std::min(len, bytes));
  }
  /**
   * Similar to `DrawTextNoClip()`.
   * Scintilla calls this method for drawing the caret, text blobs, and
   * `SC_MARKCHARACTER` line markers.
   * When drawing control characters, *rc* needs to have its pixel padding
   * removed since curses has smaller resolution. Similarly when drawing line
   * markers, *rc* needs to be reshaped.
   * @see DrawTextNoClip
   */
  void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase,
                       const char *s, int len, ColourDesired fore,
                       ColourDesired back) {
    if (rc.left >= rc.right) // when drawing text blobs
      rc.left -= 2, rc.right -= 2, rc.top -= 1, rc.bottom -= 1;
    DrawTextNoClip(rc, font_, ybase, s, len, fore, back);
  }
  /**
   * Similar to `DrawTextNoClip()`.
   * Scintilla calls this method for drawing CallTip text and two-phase buffer
   * text. However, the latter is not supported.
   */
  void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase,
                           const char *s, int len, ColourDesired fore) {
    if ((int)rc.top >= getmaxy(win) - 1) return;
    attr_t attrs = mvwinch(win, (int)rc.top, (int)rc.left);
    short pair = PAIR_NUMBER(attrs), unused, back;
    if (pair > 0) pair_content(pair, &unused, &back);
    DrawTextNoClip(rc, font_, ybase, s, len, fore, SCI_COLORS[back]);
  }
  /**
   * Measures the width of characters in the given string and writes them to the
   * given position list.
   * Curses characters always have a width of 1 if they are not UTF-8 trailing
   * bytes.
   */
  void MeasureWidths(Font &font_, const char *s, int len,
                     XYPOSITION *positions) {
    for (int i = 0, j = 0; i < len; i++) {
      if (!UTF8IsTrailByte((unsigned char)s[i])) j += grapheme_width(s + i);
      positions[i] = j;
    }
  }
  /**
   * Returns the number of UTF-8 characters in the given string since curses
   * characters always have a width of 1.
   */
  XYPOSITION WidthText(Font &font_, const char *s, int len) {
    int width = 0;
    for (int i = 0; i < len; i++)
      if (!UTF8IsTrailByte((unsigned char)s[i])) width += grapheme_width(s + i);
    return width;
  }
  /** Returns 0 since curses characters have no ascent. */
  XYPOSITION Ascent(Font &font_) { return 0; }
  /** Returns 0 since curses characters have no descent. */
  XYPOSITION Descent(Font &font_) { return 0; }
  /** Returns 0 since curses characters have no leading. */
  XYPOSITION InternalLeading(Font &font_) { return 0; }
  /** Returns 1 since curses characters always have a height of 1. */
  XYPOSITION Height(Font &font_) { return 1; }
  /** Returns 1 since curses characters always have a width of 1. */
  XYPOSITION AverageCharWidth(Font &font_) { return 1; }

  /**
   * Ensure text to be drawn in subsequent calls to `DrawText*()` is drawn
   * within the given rectangle.
   * This is needed in order to prevent long lines from overwriting margin text
   * when scrolling to the right.
   */
  void SetClip(PRectangle rc) {
    clip.left = rc.left, clip.top = rc.top;
    clip.right = rc.right, clip.bottom = rc.bottom;
  }
  /** Flushing cache is not implemented. */
  void FlushCachedState() {}

  /** Unsetting unicode mode is not implemented. UTF-8 is assumed. */
  void SetUnicodeMode(bool unicodeMode_) {}
  /** Setting DBCS mode is not implemented. UTF-8 is used. */
  void SetDBCSMode(int codePage) {}

  /** Draws the text representation of a line marker, if possible. */
  void DrawLineMarker(PRectangle &rcWhole, Font &fontForCharacter, int tFold,
                      const void *data) {
    // TODO: handle fold marker highlighting.
    const LineMarker *marker = reinterpret_cast<const LineMarker *>(data);
    wattr_set(win, 0, term_color_pair(marker->fore, marker->back), NULL);
    switch (marker->markType) {
    case SC_MARK_CIRCLE:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "●");
      return;
    case SC_MARK_SMALLRECT:
    case SC_MARK_ROUNDRECT:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "■");
      return;
    case SC_MARK_ARROW:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "►");
      return;
    case SC_MARK_SHORTARROW:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "→");
      return;
    case SC_MARK_ARROWDOWN:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "▼");
      return;
    case SC_MARK_MINUS:
      mvwaddch(win, rcWhole.top, rcWhole.left, '-');
      return;
    case SC_MARK_BOXMINUS:
    case SC_MARK_BOXMINUSCONNECTED:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "⊟");
      return;
    case SC_MARK_CIRCLEMINUS:
    case SC_MARK_CIRCLEMINUSCONNECTED:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "⊖");
      return;
    case SC_MARK_PLUS:
      mvwaddch(win, rcWhole.top, rcWhole.left, '+');
      return;
    case SC_MARK_BOXPLUS:
    case SC_MARK_BOXPLUSCONNECTED:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "⊞");
      return;
    case SC_MARK_CIRCLEPLUS:
    case SC_MARK_CIRCLEPLUSCONNECTED:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "⊕");
      return;
    case SC_MARK_VLINE:
      mvwaddch(win, rcWhole.top, rcWhole.left, ACS_VLINE);
      return;
    case SC_MARK_LCORNER:
    case SC_MARK_LCORNERCURVE:
      mvwaddch(win, rcWhole.top, rcWhole.left, ACS_LLCORNER);
      return;
    case SC_MARK_TCORNER:
    case SC_MARK_TCORNERCURVE:
      mvwaddch(win, rcWhole.top, rcWhole.left, ACS_LTEE);
      return;
    case SC_MARK_DOTDOTDOT:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "…");
      return;
    case SC_MARK_ARROWS:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "»");
      return;
    case SC_MARK_FULLRECT:
      FillRectangle(rcWhole, marker->back);
      return;
    case SC_MARK_LEFTRECT:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "▌");
      return;
    case SC_MARK_BOOKMARK:
      mvwaddstr(win, rcWhole.top, rcWhole.left, "Σ");
      return;
    }
    if (marker->markType >= SC_MARK_CHARACTER) {
      char ch = static_cast<char>(marker->markType - SC_MARK_CHARACTER);
      DrawTextClipped(rcWhole, fontForCharacter, rcWhole.bottom, &ch, 1,
                      marker->fore, marker->back);
      return;
    }
  }
  /** Draws the text representation of a wrap marker. */
  void DrawWrapMarker(PRectangle rcPlace, bool isEndMarker,
                      ColourDesired wrapColour) {
    wattr_set(win, 0, term_color_pair(wrapColour, COLOR_BLACK), NULL);
    mvwaddstr(win, rcPlace.top, rcPlace.left, isEndMarker ? "↩" : "↪");
  }
  /** Draws the text representation of a tab arrow. */
  void DrawTabArrow(PRectangle rcTab) {
    // TODO: set color to vs.whitespaceColours.fore and back.
    wattr_set(win, 0, term_color_pair(COLOR_BLACK, COLOR_BLACK), NULL);
    for (int i = rcTab.left - 1; i < rcTab.right; i++)
      mvwaddch(win, rcTab.top, i, '-' | A_BOLD);
    mvwaddch(win, rcTab.top, rcTab.right, '>' | A_BOLD);
  }
};

/** Creates a new curses surface. */
Surface *Surface::Allocate(int) { return new SurfaceImpl(); }

/** Custom function for drawing line markers in curses. */
static void DrawLineMarker(Surface *surface, PRectangle &rcWhole,
                           Font &fontForCharacter, int tFold, int marginStyle,
                           const void *data) {
  reinterpret_cast<SurfaceImpl *>(surface)->DrawLineMarker(rcWhole,
                                                           fontForCharacter,
                                                           tFold, data);
}
/** Custom function for drawing wrap markers in curses. */
static void DrawWrapVisualMarker(Surface *surface, PRectangle rcPlace,
                                 bool isEndMarker, ColourDesired wrapColour) {
  reinterpret_cast<SurfaceImpl *>(surface)->DrawWrapMarker(rcPlace, isEndMarker,
                                                           wrapColour);
}
/** Custom function for drawing tab arrows in curses. */
static void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid) {
  reinterpret_cast<SurfaceImpl *>(surface)->DrawTabArrow(rcTab);
}

// Window handling.

/** Deletes the window. */
Window::~Window() {}
/**
 * Releases the window's resources.
 * Since the only Windows created are AutoComplete and CallTip windows, and
 * since those windows are created in `ListBox::Create()` and
 * `ScintillaCurses::CreateCallTipWindow()`, respectively, via `newwin()`, it is
 * safe to use `delwin()`.
 * It is important to note that even though `ScintillaCurses::wMain` is a Window,
 * its `Destroy()` function is never called, hence why `scintilla_delete()` is
 * the complement to `scintilla_new()`.
 */
void Window::Destroy() {
  if (wid) delwin(_WINDOW(wid));
  wid = 0;
}
/**
 * Returns the window's boundaries.
 * Unlike other platforms, Scintilla paints in coordinates relative to the
 * window in curses. Therefore, this function should always return the window
 * bounds to ensure all of it is painted.
 * @return PRectangle with the window's boundaries.
 */
PRectangle Window::GetPosition() const {
  int maxx = wid ? getmaxx(_WINDOW(wid)) : 0;
  int maxy = wid ? getmaxy(_WINDOW(wid)) : 0;
  return PRectangle(0, 0, maxx, maxy);
}
/**
 * Sets the position of the window relative to its parent window.
 * It will take care not to exceed the boundaries of the parent.
 * @param rc The position relative to the parent window.
 * @param relativeTo The parent window.
 */
void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
  int begx = 0, begy = 0, x = 0, y = 0;
  // Determine the relative position.
  getbegyx(_WINDOW(relativeTo->GetID()), begy, begx);
  x = begx + rc.left;
  if (x < begx) x = begx;
  y = begy + rc.top;
  if (y < begy) y = begy;
  // Correct to fit the parent if necessary.
  int sizex = rc.right - rc.left;
  int sizey = rc.bottom - rc.top;
  int screen_width = getmaxx(_WINDOW(relativeTo->GetID()));
  int screen_height = getmaxy(_WINDOW(relativeTo->GetID()));
  if (sizex > screen_width)
    x = begx; // align left
  else if (x + sizex > begx + screen_width)
    x = begx + screen_width - sizex; // align right
  if (y + sizey > begy + screen_height) {
    y = begy + screen_height - sizey; // align bottom
    if (screen_height == 1) y--; // show directly above the relative window
  }
  if (y < 0) y = begy; // align top
  // Update the location.
  mvwin(_WINDOW(wid), y, x);
}
/** Identical to `Window::GetPosition()`. */
PRectangle Window::GetClientPosition() const { return GetPosition(); }
void Window::Show(bool show) { /* TODO: */ }
void Window::InvalidateAll() { /* notify repaint */ }
void Window::InvalidateRectangle(PRectangle rc) { /* notify repaint*/ }
/** Setting the font is not implemented. */
void Window::SetFont(Font &) {}
/** Setting the cursor icon is not implemented. */
void Window::SetCursor(Cursor curs) {}
/** Identical to `Window::GetPosition()`. */
PRectangle Window::GetMonitorRect(Point pt) { return GetPosition(); }

/**
 * Implementation of a Scintilla ListBox for curses.
 * Instead of registering images to types, printable UTF-8 characters are
 * registered to types.
 */
class ListBoxImpl : public ListBox {
  int height, width;
  std::vector<std::string> list;
  char types[IMAGE_MAX + 1][5]; // UTF-8 character plus terminating '\0'
  int selection;
public:
  IListBoxDelegate *delegate;

  /** Allocates a new Scintilla ListBox for curses. */
  ListBoxImpl() : height(5), width(10), selection(0), delegate(NULL) {
    list.reserve(10);
    ClearRegisteredImages();
  }
  /** Deletes the ListBox. */
  ~ListBoxImpl() {}

  /** Setting the font is not implemented. */
  void SetFont(Font &font) {}
  /**
   * Creates a new listbox.
   * The `Show()` function resizes window with the appropriate height and width.
   */
  void Create(Window &parent, int ctrlID, Point location_, int lineHeight_,
              bool unicodeMode_, int technology_) {
    wid = newwin(1, 1, 0, 0);
  }
  /**
   * Setting average char width is not implemented since all curses characters
   * have a width of 1.
   */
  void SetAverageCharWidth(int width) {}
  /** Sets the number of visible rows in the listbox. */
  void SetVisibleRows(int rows) {
    height = rows;
    wresize(_WINDOW(wid), height + 2, width + 2);
  }
  /** Returns the number of visible rows in the listbox. */
  int GetVisibleRows() const { return height; }
  /** Returns the desired size of the listbox. */
  PRectangle GetDesiredRect() {
    return PRectangle(0, 0, width + 2, height + 2); // add border widths
  }
  /**
   * Returns the left-offset of the ListBox with respect to the caret.
   * Takes into account the border width and type character width.
   * @return 2 to shift the ListBox to the left two characters.
   */
  int CaretFromEdge() { return 2; }
  /** Clears the contents of the listbox. */
  void Clear() {
    list.clear();
    width = 0;
  }
  /**
   * Adds the given string list item to the listbox.
   * Prepends the item's type character (if any) to the list item for display.
   */
  void Append(char *s, int type = -1) {
    if (type >= 0 && type <= IMAGE_MAX) {
      char *chtype = types[type];
      list.push_back(std::string(chtype, strlen(chtype)) + std::string(s));
    } else list.push_back(std::string(" ") + std::string(s));
    int len = strlen(s); // TODO: UTF-8 awareness?
    if (width < len) {
      width = len + 1; // include type character len
      wresize(_WINDOW(wid), height + 2, width + 2);
    }
  }
  /** Returns the number of items in the listbox. */
  int Length() { return list.size(); }
  /** Selects the given item in the listbox and repaints the listbox. */
  void Select(int n) {
    WINDOW *w = _WINDOW(wid);
    wclear(w);
    box(w, '|', '-');
    int len = static_cast<int>(list.size());
    int s = n - height / 2;
    if (s + height > len) s = len - height;
    if (s < 0) s = 0;
    for (int i = s; i < s + height && i < len; i++) {
      mvwaddstr(w, i - s + 1, 1, list.at(i).c_str());
      if (i == n) mvwchgat(w, i - s + 1, 2, width - 1, A_REVERSE, 0, NULL);
    }
    wmove(w, n - s + 1, 1); // place cursor on selected line
    wnoutrefresh(w);
    selection = n;
  }
  /** Returns the currently selected item in the listbox. */
  int GetSelection() { return selection; }
  /**
   * Searches the listbox for the items matching the given prefix string and
   * returns the index of the first match.
   * Since the type is displayed as the first character, the value starts on the
   * second character; match strings starting there.
   */
  int Find(const char *prefix) {
    int len = strlen(prefix);
    for (unsigned int i = 0; i < list.size(); i++) {
      const char *item = list.at(i).c_str();
      item += UTF8DrawBytes(reinterpret_cast<const unsigned char *>(item),
                            strlen(item));
      if (strncmp(prefix, item, len) == 0) return i;
    }
    return -1;
  }
  /**
   * Gets the item in the listbox at the given index and stores it in the given
   * string.
   * Since the type is displayed as the first character, the value starts on the
   * second character.
   */
  void GetValue(int n, char *value, int len) {
    if (len > 0) {
      const char *item = list.at(n).c_str();
      item += UTF8DrawBytes(reinterpret_cast<const unsigned char *>(item),
                            strlen(item));
      strncpy(value, item, len);
      value[len - 1] = '\0';
    } else value[0] = '\0';
  }
  /**
   * Registers the first UTF-8 character of the given string to the given type.
   * By default, ' ' (space) is registered to all types.
   * @usage SCI_REGISTERIMAGE(1, "*") // type 1 shows '*' in front of list item.
   * @usage SCI_REGISTERIMAGE(2, "+") // type 2 shows '+' in front of list item.
   * @usage SCI_REGISTERIMAGE(3, "■") // type 3 shows '■' in front of list item.
   */
  void RegisterImage(int type, const char *xpm_data) {
    if (type < 0 || type > IMAGE_MAX) return;
    int len = UTF8DrawBytes(reinterpret_cast<const unsigned char *>(xpm_data),
                            strlen(xpm_data));
    for (int i = 0; i < len; i++) types[type][i] = xpm_data[i];
    types[type][len] = '\0';
  }
  /** Registering images is not implemented. */
  void RegisterRGBAImage(int type, int width, int height,
                         const unsigned char *pixelsImage) {}
  /** Clears all registered types back to ' ' (space). */
  void ClearRegisteredImages() {
    for (int i = 0; i <= IMAGE_MAX; i++) types[i][0] = ' ', types[i][1] = '\0';
  }
  /** Defines the delegate for ListBox actions. */
  void SetDelegate(IListBoxDelegate *lbDelegate) {
    delegate = lbDelegate;
  }
  /** Sets the list items in the listbox to the given items. */
  void SetList(const char *listText, char separator, char typesep) {
    Clear();
    int len = strlen(listText);
    char *text = new char[len + 1];
    if (!text) return;
    memcpy(text, listText, len + 1);
    char *word = text, *type = NULL;
    for (int i = 0; i <= len; i++) {
      if (text[i] == separator || i == len) {
        text[i] = '\0';
        if (type) *type = '\0';
        Append(word, type ? atoi(type + 1) : -1);
        word = text + i + 1, type = NULL;
      } else if (text[i] == typesep)
        type = text + i;
    }
    delete []text;
  }
};

/** Creates a new Scintilla ListBox. */
ListBox::ListBox() noexcept {}
/** Deletes the ListBox. */
ListBox::~ListBox() {}
/** Creates a new curses ListBox. */
ListBox *ListBox::Allocate() { return new ListBoxImpl(); }

// Menus are not implemented.
Menu::Menu() noexcept : mid(0) {}
void Menu::CreatePopUp() {}
void Menu::Destroy() {}
void Menu::Show(Point pt, Window &w) {}

/** Dynamic library loading is not implemented. */
DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
  /* TODO */ return 0;
}

ColourDesired Platform::Chrome() { return ColourDesired(0, 0, 0); }
ColourDesired Platform::ChromeHighlight() { return ColourDesired(0, 0, 0); }
const char *Platform::DefaultFont() { return "monospace"; }
int Platform::DefaultFontSize() { return 10; }
unsigned int Platform::DoubleClickTime() { return 500; /* ms */ }
void Platform::DebugDisplay(const char *s) { fprintf(stderr, "%s", s); }
void Platform::DebugPrintf(const char *format, ...) {}
//bool Platform::ShowAssertionPopUps(bool assertionPopUps_) { return true; }
void Platform::Assert(const char *c, const char *file, int line) {
  char buffer[2000];
  sprintf(buffer, "Assertion [%s] failed at %s %d\r\n", c, file, line);
  Platform::DebugDisplay(buffer);
  abort();
}

/** Implementation of Scintilla for curses. */
class ScintillaCurses : public ScintillaBase {
  Surface *sur; // window surface to draw on
  int width, height; // window dimensions
  void (*callback)(void *, int, void *, void *); // SCNotification callback
  int scrollBarVPos, scrollBarHPos; // positions of the scroll bars
  int scrollBarHeight, scrollBarWidth; // height and width of the scroll bars
  SelectionText clipboard; // current clipboard text
  bool capturedMouse; // whether or not the mouse is currently captured
  unsigned int autoCompleteLastClickTime; // last click time in the AC box
  bool draggingVScrollBar, draggingHScrollBar; // a scrollbar is being dragged
  int dragOffset; // the distance to the position of the scrollbar being dragged

  /**
   * Uses the given UTF-8 code point to fill the given UTF-8 byte sequence and
   * length.
   * This algorithm was inspired by Paul Evans' libtermkey.
   * (http://www.leonerd.org.uk/code/libtermkey)
   * @param code The UTF-8 code point.
   * @param s The string to write the UTF-8 byte sequence in. Must be at least
   *   6 bytes in size.
   * @param len The integer to put the number of UTF-8 bytes written in.
   */
  void toutf8(int code, char *s, int *len) {
    if (code < 0x80) *len = 1;
    else if (code < 0x800) *len = 2;
    else if (code < 0x10000) *len = 3;
    else if (code < 0x200000) *len = 4;
    else if (code < 0x4000000) *len = 5;
    else *len = 6;
    for (int b = *len - 1; b > 0; b--) s[b] = 0x80 | (code & 0x3F), code >>= 6;
    if (*len == 1) s[0] = code & 0x7F;
    else if (*len == 2) s[0] = 0xC0 | (code & 0x1F);
    else if (*len == 3) s[0] = 0xE0 | (code & 0x0F);
    else if (*len == 4) s[0] = 0xF0 | (code & 0x07);
    else if (*len == 5) s[0] = 0xF8 | (code & 0x03);
    else if (*len == 6) s[0] = 0xFC | (code & 0x01);
  }
public:
  /**
   * Creates a new Scintilla instance in a curses `WINDOW`.
   * However, the `WINDOW` itself will not be created until it is absolutely
   * necessary. When the `WINDOW` is created, it will initially be full-screen.
   * @param callback_ Callback function for Scintilla notifications.
   */
  ScintillaCurses(void (*callback_)(void *, int, void *, void *)) :
                 width(0), height(0), scrollBarHeight(1), scrollBarWidth(1) {
    callback = callback_;
    sur = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);

    // Defaults for curses.
    marginView.wrapMarkerPaddingRight = 0; // no padding for margin wrap markers
    marginView.customDrawWrapMarker = DrawWrapVisualMarker; // draw text markers
    view.tabWidthMinimumPixels = 0; // no proportional fonts
    view.drawOverstrikeCaret = false; // always draw normal caret
    view.bufferedDraw = false; // draw directly to the screen
    view.phasesDraw = EditView::phasesOne; // no need for two-phase drawing
    view.tabArrowHeight = 0; // no additional tab arrow height
    view.customDrawTabArrow = DrawTabArrow; // draw text arrows for tabs
    view.customDrawWrapMarker = DrawWrapVisualMarker; // draw text wrap markers
    mouseSelectionRectangularSwitch = true; // easier rectangular selection
    doubleClickCloseThreshold = Point(0, 0); // double-clicks only in same cell
    horizontalScrollBarVisible = false; // no horizontal scroll bar
    scrollWidth = 5 * width; // reasonable default for any horizontal scroll bar
    vs.selColours.fore = ColourDesired(0, 0, 0); // black on white selection
    vs.selColours.fore.isSet = true; // setting selection foreground above
    vs.caretcolour = ColourDesired(0xFF, 0xFF, 0xFF); // white caret
    vs.caretStyle = CARETSTYLE_BLOCK; // block caret
    vs.leftMarginWidth = 0, vs.rightMarginWidth = 0; // no margins
    vs.ms[1].width = 1; // marker margin width should be 1
    vs.extraDescent = -1; // hack to make lineHeight 1 instead of 2
    // Set default marker foreground and background colors.
    for (int i = 0; i <= MARKER_MAX; i++) {
      vs.markers[i].fore = ColourDesired(0xC0, 0xC0, 0xC0);
      vs.markers[i].back = ColourDesired(0, 0, 0);
      if (i >= 25) vs.markers[i].markType = SC_MARK_EMPTY;
      vs.markers[i].customDraw = DrawLineMarker;
    }
    // Use '+' and '-' fold markers.
    vs.markers[SC_MARKNUM_FOLDEROPEN].markType = SC_MARK_MINUS;
    vs.markers[SC_MARKNUM_FOLDER].markType = SC_MARK_PLUS;
    vs.markers[SC_MARKNUM_FOLDEROPENMID].markType = SC_MARK_MINUS;
    vs.markers[SC_MARKNUM_FOLDEREND].markType = SC_MARK_PLUS;
    displayPopupMenu = false; // no context menu
    vs.marginNumberPadding = 0; // no number margin padding
    vs.ctrlCharPadding = 0; // no ctrl character text blob padding
    vs.lastSegItalicsOffset = 0; // no offset for italic characters at EOLs
    ac.widthLBDefault = 10; // more sane bound for autocomplete width
    ac.heightLBDefault = 10; // more sane bound for autocomplete  height
    ct.colourBG = ColourDesired(0, 0, 0); // black background color
    ct.colourUnSel = ColourDesired(0xC0, 0xC0, 0xC0); // white text
    ct.insetX = 2; // border and arrow widths are 1 each
    ct.widthArrow = 1; // arrow width is 1 character
    ct.borderHeight = 1; // no extra empty lines in border height
    ct.verticalOffset = 0; // no extra offset of calltip from line
  }
  /** Deletes the Scintilla instance. */
  ~ScintillaCurses() {
    if (wMain.GetID())
      delwin(GetWINDOW());
    if (sur) {
      sur->Release();
      delete sur;
    }
  }
  /** Initializing code is unnecessary. */
  void Initialise() {}
  /** Disable drag and drop since it is not implemented. */
  void StartDrag() {
    inDragDrop = ddNone;
    SetDragPosition(SelectionPosition(Sci::invalidPosition));
  }
  /** Draws the vertical scroll bar. */
  void SetVerticalScrollPos() {
    if (!wMain.GetID() || !verticalScrollBarVisible) return;
    WINDOW *w = GetWINDOW();
    int maxy = getmaxy(w), maxx = getmaxx(w);
    // Draw the gutter.
    wattr_set(w, 0, term_color_pair(COLOR_WHITE, COLOR_BLACK), NULL);
    for (int i = 0; i < maxy; i++) mvwaddch(w, i, maxx - 1, ACS_CKBOARD);
    // Draw the bar.
    scrollBarVPos = static_cast<float>(topLine) /
                    (MaxScrollPos() + LinesOnScreen() - 1) * maxy;
    wattr_set(w, 0, term_color_pair(COLOR_BLACK, COLOR_WHITE), NULL);
    for (int i = scrollBarVPos; i < scrollBarVPos + scrollBarHeight; i++)
      mvwaddch(w, i, maxx - 1, ' ');
  }
  /** Draws the horizontal scroll bar. */
  void SetHorizontalScrollPos() {
    if (!wMain.GetID() || !horizontalScrollBarVisible) return;
    WINDOW *w = GetWINDOW();
    int maxy = getmaxy(w), maxx = getmaxx(w);
    // Draw the gutter.
    wattr_set(w, 0, term_color_pair(COLOR_WHITE, COLOR_BLACK), NULL);
    for (int i = 0; i < maxx; i++) mvwaddch(w, maxy - 1, i, ACS_CKBOARD);
    // Draw the bar.
    scrollBarHPos = static_cast<float>(xOffset) / scrollWidth * maxx;
    wattr_set(w, 0, term_color_pair(COLOR_BLACK, COLOR_WHITE), NULL);
    for (int i = scrollBarHPos; i < scrollBarHPos + scrollBarWidth; i++)
      mvwaddch(w, maxy - 1, i, ' ');
  }
  /**
   * Sets the height of the vertical scroll bar and width of the horizontal
   * scroll bar.
   * The height is based on the given size of a page and the total number of
   * pages. The width is based on the width of the view and the view's scroll
   * width property.
   */
  bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
    if (!wMain.GetID()) return false;
    WINDOW *w = GetWINDOW();
    int maxy = getmaxy(w), maxx = getmaxx(w);
    int height = roundf(static_cast<float>(nPage) / nMax * maxy);
    scrollBarHeight = Sci::clamp(height, 1, maxy);
    int width = roundf(static_cast<float>(maxx) / scrollWidth * maxx);
    scrollBarWidth = Sci::clamp(width, 1, maxx);
    return true;
  }
  /**
   * Copies the selected text to the internal clipboard.
   * The primary and secondary X selections are unaffected.
   */
  void Copy() { if (!sel.Empty()) CopySelectionRange(&clipboard); }
  /**
   * Pastes text from the internal clipboard, not from primary or secondary X
   * selections.
   */
  void Paste() {
    if (clipboard.Empty()) return;
    ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
    InsertPasteShape(clipboard.Data(), static_cast<int>(clipboard.Length()),
                     !clipboard.rectangular ? pasteStream : pasteRectangular);
    EnsureCaretVisible();
  }
  /** Setting of the primary and/or secondary X selections is not supported. */
  void ClaimSelection() {}
  /** Notifying the parent of text changes is not yet supported. */
  void NotifyChange() {}
  /** Send Scintilla notifications to the parent. */
  void NotifyParent(SCNotification scn) {
    if (callback)
      (*callback)(reinterpret_cast<void *>(this), 0, (void *)&scn, 0);
  }
  /**
   * Handles an unconsumed key.
   * If a character is being typed, add it to the editor. Otherwise, notify the
   * container.
   */
  int KeyDefault(int key, int modifiers) {
    if ((IsUnicodeMode() || key < 256) && modifiers == 0) {
      if (IsUnicodeMode()) {
        char utf8[6];
        int len;
        toutf8(key, utf8, &len);
        return (AddCharUTF(utf8, len), 1);
      } else return (AddChar(key), 1);
    } else {
      SCNotification scn = {};
      scn.nmhdr.code = SCN_KEY;
      scn.ch = key;
      scn.modifiers = modifiers;
      return (NotifyParent(scn), 0);
    }
  }
  /**
   * Copies the given text to the internal clipboard.
   * Like `Copy()`, does not affect the primary and secondary X selections.
   */
  void CopyToClipboard(const SelectionText &selectedText) {
    clipboard.Copy(selectedText);
  }
  /** A ticking caret is not implemented. */
  bool FineTickerRunning(TickReason reason) { return false; }
  /** A ticking caret is not implemented. */
  void FineTickerStart(TickReason reason, int millis, int tolerance) {}
  /** A ticking caret is not implemented. */
  void FineTickerCancel(TickReason reason) {}
  /**
   * Sets whether or not the mouse is captured.
   * This is used by Scintilla to handle mouse clicks, drags, and releases.
   */
  void SetMouseCapture(bool on) { capturedMouse = on; }
  /** Returns whether or not the mouse is captured. */
  bool HaveMouseCapture() { return capturedMouse; }
  /** A Scintilla direct pointer is not implemented. */
  sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
    return 0;
  }
  /** Draws a CallTip, creating the curses window for it if necessary. */
  void CreateCallTipWindow(PRectangle rc) {
    if (!wMain.GetID()) return;
    if (!ct.wCallTip.Created()) {
      rc.right -= 1; // remove right-side padding
      int begx = 0, begy = 0, maxx = 0, maxy = 0;
      getbegyx(GetWINDOW(), begy, begx);
      int xoffset = begx - rc.left, yoffset = begy - rc.top;
      if (xoffset > 0) rc.left += xoffset, rc.right += xoffset;
      if (yoffset > 0) rc.top += yoffset, rc.bottom += yoffset;
      getmaxyx(GetWINDOW(), maxy, maxx);
      if (rc.Width() > maxx) rc.right = rc.left + maxx;
      if (rc.Height() > maxy) rc.bottom = rc.top + maxy;
      ct.wCallTip = newwin(rc.Height(), rc.Width(), rc.top, rc.left);
    }
    WindowID wid = ct.wCallTip.GetID();
    box(_WINDOW(wid), '|', '-');
    Surface *sur = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
    if (sur) {
      sur->Init(wid);
      ct.PaintCT(sur);
      wnoutrefresh(_WINDOW(wid));
      sur->Release();
      delete sur;
    }
  }
  /** Adding menu items to the popup menu is not implemented. */
  void AddToPopUp(const char *label, int cmd=0, bool enabled=true) {}
  /**
   * Sends the given message and parameters to Scintilla unless it is a message
   * that changes an unsupported property.
   */
  sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
    try {
      switch (iMessage) {
        case SCI_GETDIRECTFUNCTION:
          return reinterpret_cast<sptr_t>(scintilla_send_message);
        case SCI_GETDIRECTPOINTER: return reinterpret_cast<sptr_t>(this);
        // Ignore attempted changes of the following unsupported properties.
        case SCI_SETBUFFEREDDRAW:
        case SCI_SETWHITESPACESIZE:
        case SCI_SETTWOPHASEDRAW: case SCI_SETPHASESDRAW:
        case SCI_SETEXTRAASCENT: case SCI_SETEXTRADESCENT:
          return 0;
        // Pass to Scintilla.
        default: return ScintillaBase::WndProc(iMessage, wParam, lParam);
      }
    } catch (std::bad_alloc&) {
      errorStatus = SC_STATUS_BADALLOC;
    } catch (...) {
      errorStatus = SC_STATUS_FAILURE;
    }
    return 0;
  }

  /**
   * Returns the curses `WINDOW` associated with this Scintilla instance.
   * If the `WINDOW` has not been created yet, create it now.
   */
  WINDOW *GetWINDOW() {
    if (!wMain.GetID()) {
      init_colors();
      wMain = newwin(0, 0, 0, 0);
      WINDOW *w = _WINDOW(wMain.GetID());
      keypad(w, TRUE);
      if (sur)
        sur->Init(w);
      getmaxyx(w, height, width);
      InvalidateStyleRedraw(); // needed to fully initialize Scintilla
    }
    return _WINDOW(wMain.GetID());
  }
  /**
   * Repaints the Scintilla window on the virtual screen.
   * If an autocompletion list, user list, or calltip is active, redraw it over
   * the buffer's contents.
   * It is the application's responsibility to call the curses `doupdate()` in
   * order to refresh the physical screen.
   * To paint to the physical screen instead, use `Refresh()`.
   * @see Refresh
   */
  void NoutRefresh() {
    WINDOW *w = GetWINDOW();
    rcPaint.top = 0, rcPaint.left = 0; // paint from (0, 0), not (begy, begx)
    getmaxyx(w, rcPaint.bottom, rcPaint.right);
    if (rcPaint.bottom != height || rcPaint.right != width)
      height = rcPaint.bottom, width = rcPaint.right, ChangeSize();
    Paint(sur, rcPaint);
    SetVerticalScrollPos(), SetHorizontalScrollPos();
    wnoutrefresh(w);
#if PDCURSES
    touchwin(w); // pdcurses sometimes has problems drawing overlapping windows
#endif
    if (ac.Active())
      ac.lb->Select(ac.lb->GetSelection()); // redraw
    else if (ct.inCallTipMode)
      CreateCallTipWindow(PRectangle(0, 0, 0, 0)); // redraw
    if (hasFocus) {
      // Update cursor position, even if it's not visible, as the container may
      // have a use for it.
      int pos = WndProc(SCI_GETCURRENTPOS, 0, 0);
      move(getbegy(w) + WndProc(SCI_POINTYFROMPOSITION, 0, pos),
           getbegx(w) + WndProc(SCI_POINTXFROMPOSITION, 0, pos));
    }
  }
  /**
   * Repaints the Scintilla window on the physical screen.
   * If an autocompletion list, user list, or calltip is active, redraw it over
   * the buffer's contents.
   * To paint to the virtual screen instead, use `NoutRefresh()`.
   * @see NoutRefresh
   */
  void Refresh() {
    NoutRefresh();
    doupdate();
  }
  /**
   * Sends a key to Scintilla.
   * Usually if a key is consumed, the screen should be repainted. However, when
   * autocomplete is active, that window is consuming the keys and any
   * repainting of the main Scintilla window will overwrite the autocomplete
   * window.
   * @param key The key pressed.
   * @param shift Flag indicating whether or not the shift modifier key is
   *   pressed.
   * @param shift Flag indicating whether or not the control modifier key is
   *   pressed.
   * @param shift Flag indicating whether or not the alt modifier key is
   *   pressed.
   */
  void KeyPress(int key, bool shift, bool ctrl, bool alt) {
    KeyDownWithModifiers(key, ModifierFlags(shift, ctrl, alt), NULL);
  }
  /**
   * Handles a mouse button press.
   * @param button The button number pressed, or `0` if none.
   * @param time The time in milliseconds of the mouse event.
   * @param y The y coordinate of the mouse event relative to this window.
   * @param x The x coordinate of the mouse event relative to this window.
   * @param shift Flag indicating whether or not the shift modifier key is
   *   pressed.
   * @param ctrl Flag indicating whether or not the control modifier key is
   *   pressed.
   * @param alt Flag indicating whether or not the alt modifier key is pressed.
   * @return whether or not the mouse event was handled
   */
  bool MousePress(int button, unsigned int time, int y, int x, bool shift,
                  bool ctrl, bool alt) {
    GetWINDOW(); // ensure the curses `WINDOW` has been created
    if (ac.Active() && (button == 1 || button == 4 || button == 5)) {
      // Select an autocompletion list item if possible or scroll the list.
      WINDOW *w = _WINDOW(ac.lb->GetID()), *parent = GetWINDOW();
      int begy = getbegy(w) - getbegy(parent); // y is relative to the view
      int begx = getbegx(w) - getbegx(parent); // x is relative to the view
      int maxy = getmaxy(w) - 1, maxx = getmaxx(w) - 1; // ignore border
      int ry = y - begy, rx = x - begx; // relative to list box
      if (ry > 0 && ry < maxy && rx > 0 && rx < maxx) {
        if (button == 1) {
          // Select a list item.
          // The currently selected item is normally displayed in the middle.
          int middle = ac.lb->GetVisibleRows() / 2;
          int n = ac.lb->GetSelection(), ny = middle;
          if (n < middle)
            ny = n; // the currently selected item is near the beginning
          else if (n >= ac.lb->Length() - middle)
            ny = (n - 1) % ac.lb->GetVisibleRows(); // it's near the end
          // Compute the index of the item to select.
          int offset = ry - ny - 1; // -1 ignores list box border
          if (offset == 0 &&
              time - autoCompleteLastClickTime < Platform::DoubleClickTime()) {
            ListBoxImpl* listbox = reinterpret_cast<ListBoxImpl *>(ac.lb.get());
            if (listbox->delegate) {
              ListBoxEvent event(ListBoxEvent::EventType::doubleClick);
              listbox->delegate->ListNotify(&event);
            }
          } else ac.lb->Select(n + offset);
          autoCompleteLastClickTime = time;
        } else {
          // Scroll the list.
          int n = ac.lb->GetSelection();
          if (button == 4 && n > 0)
            ac.lb->Select(n - 1);
          else if (button == 5 && n < ac.lb->Length() - 1)
            ac.lb->Select(n + 1);
        }
        return true;
      } else if (ry == 0 || ry == maxy || rx == 0 || rx == maxx)
        return true; // ignore border click
    } else if (ct.inCallTipMode && button == 1) {
      // Send the click to the CallTip.
      WINDOW *w = _WINDOW(ct.wCallTip.GetID()), *parent = GetWINDOW();
      int begy = getbegy(w) - getbegy(parent); // y is relative to the view
      int begx = getbegx(w) - getbegx(parent); // x is relative to the view
      int maxy = getmaxy(w) - 1, maxx = getmaxx(w) - 1; // ignore border
      int ry = y - begy, rx = x - begx; // relative to list box
      if (ry >= 0 && ry <= maxy && rx >= 0 && rx <= maxx) {
        ct.MouseClick(Point(rx, ry));
        return (CallTipClick(), true);
      }
    }

    if (button == 1) {
      if (verticalScrollBarVisible && x == getmaxx(GetWINDOW()) - 1) {
        // Scroll the vertical scrollbar.
        if (y < scrollBarVPos)
          return (ScrollTo(topLine - LinesOnScreen()), true);
        else if (y >= scrollBarVPos + scrollBarHeight)
          return (ScrollTo(topLine + LinesOnScreen()), true);
        else
          draggingVScrollBar = true, dragOffset = y - scrollBarVPos;
      } else if (horizontalScrollBarVisible && y == getmaxy(GetWINDOW()) - 1) {
        // Scroll the horizontal scroll bar.
        if (x < scrollBarHPos)
          return (HorizontalScrollTo(xOffset - getmaxx(GetWINDOW()) / 2), true);
        else if (x >= scrollBarHPos + scrollBarWidth)
          return (HorizontalScrollTo(xOffset + getmaxx(GetWINDOW()) / 2), true);
        else
          draggingHScrollBar = true, dragOffset = x - scrollBarHPos;
      } else {
        // Have Scintilla handle the click.
        ButtonDownWithModifiers(Point(x, y), time,
                                ModifierFlags(shift, ctrl, alt));
        return true;
      }
    } else if (button == 4 || button == 5) {
      // Scroll the view.
      int lines = getmaxy(GetWINDOW()) / 4;
      if (lines < 1) lines = 1;
      if (button == 4) lines *= -1;
      return (ScrollTo(topLine + lines), true);
    }
    return false;
  }
  /**
   * Sends a mouse move event to Scintilla, returning whether or not Scintilla
   * handled the mouse event.
   * @param y The y coordinate of the mouse event relative to this window.
   * @param x The x coordinate of the mouse event relative to this window.
   * @param shift Flag indicating whether or not the shift modifier key is
   *   pressed.
   * @param ctrl Flag indicating whether or not the control modifier key is
   *   pressed.
   * @param alt Flag indicating whether or not the alt modifier key is pressed.
   * @return whether or not Scintilla handled the mouse event
   */
  bool MouseMove(int y, int x, bool shift, bool ctrl, bool alt) {
    GetWINDOW(); // ensure the curses `WINDOW` has been created
    if (!draggingVScrollBar && !draggingHScrollBar) {
      ButtonMoveWithModifiers(Point(x, y), 0, ModifierFlags(shift, ctrl, alt));
    } else if (draggingVScrollBar) {
      int maxy = getmaxy(GetWINDOW()) - scrollBarHeight, pos = y - dragOffset;
      if (pos >= 0 && pos <= maxy) ScrollTo(pos * MaxScrollPos() / maxy);
      return true;
    } else if (draggingHScrollBar) {
      int maxx = getmaxx(GetWINDOW()) - scrollBarWidth, pos = x - dragOffset;
      if (pos >= 0 && pos <= maxx)
        HorizontalScrollTo(pos * (scrollWidth - maxx - scrollBarWidth) / maxx);
      return true;
    }
    return HaveMouseCapture();
  }
  /**
   * Sends a mouse release event to Scintilla.
   * @param time The time in milliseconds of the mouse event.
   * @param y The y coordinate of the mouse event relative to this window.
   * @param x The x coordinate of the mouse event relative to this window.
   * @param ctrl Flag indicating whether or not the control modifier key is
   *   pressed.
   */
  void MouseRelease(int time, int y, int x, int ctrl) {
    GetWINDOW(); // ensure the curses `WINDOW` has been created
    if (draggingVScrollBar || draggingHScrollBar)
      draggingVScrollBar = false, draggingHScrollBar = false;
    else if (HaveMouseCapture()) {
      ButtonUpWithModifiers(Point(x, y), time,
                            ModifierFlags(ctrl, false, false));
      // TODO: ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
      // TODO: listbox->delegate->ListNotify(&event);
    }
  }
  /**
   * Copies the text of the internal clipboard, not the primary and/or secondary
   * X selections, into the given buffer and returns the size of the clipboard
   * text.
   * @param text The buffer to copy clipboard text to.
   * @return size of the clipboard text
   */
  int GetClipboard(char *buffer) {
    if (buffer) memcpy(buffer, clipboard.Data(), clipboard.Length() + 1);
    return clipboard.Length() + 1;
  }
};

// Link with C. Documentation in Scintilla.h.
extern "C" {
void *scintilla_new(void (*callback)(void *, int, void *, void *)) {
  return reinterpret_cast<void *>(new ScintillaCurses(callback));
}
WINDOW *scintilla_get_window(void *sci) {
  return reinterpret_cast<ScintillaCurses *>(sci)->GetWINDOW();
}
sptr_t scintilla_send_message(void *sci, unsigned int iMessage, uptr_t wParam,
                              sptr_t lParam) {
  return reinterpret_cast<ScintillaCurses *>(sci)->WndProc(iMessage, wParam,
                                                           lParam);
}
void scintilla_send_key(void *sci, int key, bool shift, bool ctrl, bool alt) {
  reinterpret_cast<ScintillaCurses *>(sci)->KeyPress(key, shift, ctrl, alt);
}
bool scintilla_send_mouse(void *sci, int event, unsigned int time, int button,
                          int y, int x, bool shift, bool ctrl, bool alt) {
  ScintillaCurses *scicurses = reinterpret_cast<ScintillaCurses *>(sci);
  WINDOW *w = scicurses->GetWINDOW();
  int begy = getbegy(w), begx = getbegx(w);
  int maxy = getmaxy(w), maxx = getmaxx(w);
  // Ignore most events outside the window.
  if ((x < begx || x > begx + maxx - 1 || y < begy || y > begy + maxy - 1) &&
      button != 4 && button != 5) return false;
  y = y - begy, x = x - begx;
  if (event == SCM_PRESS)
    return scicurses->MousePress(button, time, y, x, shift, ctrl, alt);
  else if (event == SCM_DRAG)
    return scicurses->MouseMove(y, x, shift, ctrl, alt);
  else if (event == SCM_RELEASE)
    return (scicurses->MouseRelease(time, y, x, ctrl), true);
  return false;
}
int scintilla_get_clipboard(void *sci, char *buffer) {
  return reinterpret_cast<ScintillaCurses *>(sci)->GetClipboard(buffer);
}
void scintilla_noutrefresh(void *sci) {
  reinterpret_cast<ScintillaCurses *>(sci)->NoutRefresh();
}
void scintilla_refresh(void *sci) {
  reinterpret_cast<ScintillaCurses *>(sci)->Refresh();
}
void scintilla_delete(void *sci) {
  delete reinterpret_cast<ScintillaCurses *>(sci);
}
}

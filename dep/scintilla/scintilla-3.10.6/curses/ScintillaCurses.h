// Copyright 2012-2019 Mitchell mitchell.att.foicica.com. See License.txt.
// Header for Scintilla in a curses (terminal) environment.

#ifndef SCINTILLACURSES_H
#define SCINTILLACURSES_H

#include <curses.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new Scintilla window.
 * Curses does not have to be initialized before calling this function.
 * @param callback A callback function for Scintilla notifications.
 */
void *scintilla_new(void (*callback)(void *sci, int iMessage, void *wParam,
                                     void *lParam));
/**
 * Returns the curses `WINDOW` associated with the given Scintilla window.
 * Curses must have been initialized prior to calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 * @return curses `WINDOW`.
 */
WINDOW *scintilla_get_window(void *sci);
/**
 * Sends the given message with parameters to the given Scintilla window.
 * Curses does not have to be initialized before calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 * @param iMessage The message ID.
 * @param wParam The first parameter.
 * @param lParam The second parameter.
 */
sptr_t scintilla_send_message(void *sci, unsigned int iMessage, uptr_t wParam,
                              sptr_t lParam);
/**
 * Sends the specified key to the given Scintilla window for processing.
 * If it is not consumed, an SCNotification will be emitted.
 * Curses does not have to be initialized before calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 * @param key The keycode of the key.
 * @param shift Flag indicating whether or not the shift modifier key is
 *   pressed.
 * @param ctrl Flag indicating whether or not the control modifier key is
 *   pressed.
 * @param alt Flag indicating whether or not the alt modifier key is pressed.
 */
void scintilla_send_key(void *sci, int key, bool shift, bool ctrl, bool alt);
/**
 * Sends the specified mouse event to the given Scintilla window for processing.
 * Curses must have been initialized prior to calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 * @param event The mouse event (`SCM_CLICK`, `SCM_DRAG`, or `SCM_RELEASE`).
 * @param time The time in milliseconds of the mouse event. This is only needed
 *   if double and triple clicks need to be detected.
 * @param button The button number pressed, or `0` if none.
 * @param y The absolute y coordinate of the mouse event.
 * @param x The absolute x coordinate of the mouse event.
 * @param shift Flag indicating whether or not the shift modifier key is
 *   pressed.
 * @param ctrl Flag indicating whether or not the control modifier key is
 *   pressed.
 * @param alt Flag indicating whether or not the alt modifier key is pressed.
 * @return whether or not Scintilla handled the mouse event
 */
bool scintilla_send_mouse(void *sci, int event, unsigned int time, int button,
                          int y, int x, bool shift, bool ctrl, bool alt);
/**
 * Copies the text of Scintilla's internal clipboard, not the primary and/or
 * secondary X selections, into the given buffer and returns the size of the
 * clipboard text.
 * Call with a `null` buffer first to get the size of the buffer needed to store
 * clipboard text.
 * Keep in mind clipboard text may contain null bytes.
 * Curses does not have to be initialized before calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 * @param buffer The buffer to copy clipboard text to.
 * @return size of the clipboard text.
 */
int scintilla_get_clipboard(void *sci, char *buffer);
/**
 * Refreshes the Scintilla window on the virtual screen.
 * This should be done along with the normal curses `noutrefresh()`, as the
 * virtual screen is updated when calling this function.
 * Curses must have been initialized prior to calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 */
void scintilla_noutrefresh(void *sci);
/**
 * Refreshes the Scintilla window on the physical screen.
 * This should be done along with the normal curses `refresh()`, as the physical
 * screen is updated when calling this function.
 * Curses must have been initialized prior to calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 */
void scintilla_refresh(void *sci);
/**
 * Deletes the given Scintilla window.
 * Curses must have been initialized prior to calling this function.
 * @param sci The Scintilla window returned by `scintilla_new()`.
 */
void scintilla_delete(void *sci);

/**
 * Returns the curses `COLOR_PAIR` for the given curses foreground and
 * background `COLOR`s.
 * This is used simply to enumerate every possible color combination.
 * Note: only 256 combinations are possible due to curses portability.
 * Note: This references the global curses variable `COLORS` and is
 * not a constant expression.
 * @param f The curses foreground `COLOR`.
 * @param b The curses background `COLOR`.
 * @return int number for defining a curses `COLOR_PAIR`.
 */
#define SCI_COLOR_PAIR(f, b) ((b) * ((COLORS < 16) ? 8 : 16) + (f) + 1)

#define IMAGE_MAX 31

#define SCM_PRESS 1
#define SCM_DRAG 2
#define SCM_RELEASE 3

#ifdef __cplusplus
}
#endif

#endif

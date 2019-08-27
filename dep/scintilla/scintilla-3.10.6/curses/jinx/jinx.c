// Copyright 2012-2019 Mitchell mitchell.att.foicica.com. See License.txt.

#include <locale.h>
#include <sys/time.h>
#include <curses.h>

#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaCurses.h"

#define SSM(m, w, l) scintilla_send_message(sci, m, w, l)

typedef void Scintilla;

void scnotification(Scintilla *view, int msg, void *lParam, void *wParam) {
  //struct SCNotification *scn = (struct SCNotification *)lParam;
  //printw("SCNotification received: %i", scn->nmhdr.code);
}

int main(int argc, char **argv) {
  setlocale(LC_CTYPE, ""); // for displaying UTF-8 characters properly
  initscr(), raw(), cbreak(), noecho(), start_color();
  Scintilla *sci = scintilla_new(scnotification);
  curs_set(0); // Scintilla draws its own cursor

  SSM(SCI_STYLESETFORE, STYLE_DEFAULT, 0xFFFFFF);
  SSM(SCI_STYLESETBACK, STYLE_DEFAULT, 0);
  SSM(SCI_STYLECLEARALL, 0, 0);
#if !LPEG_LEXER
  SSM(SCI_SETLEXER, SCLEX_CPP, 0);
  SSM(SCI_SETKEYWORDS, 0, (sptr_t)"int char");
  SSM(SCI_STYLESETFORE, SCE_C_COMMENT, 0x00FF00);
  SSM(SCI_STYLESETFORE, SCE_C_COMMENTLINE, 0x00FF00);
  SSM(SCI_STYLESETFORE, SCE_C_NUMBER, 0xFFFF00);
  SSM(SCI_STYLESETFORE, SCE_C_WORD, 0xFF0000);
  SSM(SCI_STYLESETFORE, SCE_C_STRING, 0xFF00FF);
  SSM(SCI_STYLESETBOLD, SCE_C_OPERATOR, 1);
#else
  SSM(SCI_SETLEXER, SCLEX_LPEG, 0);
  SSM(SCI_SETPROPERTY, (uptr_t)"lexer.lpeg.home", (sptr_t)"../../lexlua");
  SSM(SCI_SETPROPERTY, (uptr_t)"lexer.lpeg.color.theme", (sptr_t)"curses");
  SSM(SCI_PRIVATELEXERCALL, SCI_GETDIRECTFUNCTION,
      SSM(SCI_GETDIRECTFUNCTION, 0, 0));
  SSM(SCI_PRIVATELEXERCALL, SCI_SETDOCPOINTER, SSM(SCI_GETDIRECTPOINTER, 0, 0));
  SSM(SCI_PRIVATELEXERCALL, SCI_SETLEXERLANGUAGE, (sptr_t)"ansi_c");
#endif
  SSM(SCI_INSERTTEXT, 0, (sptr_t)
      "int main(int argc, char **argv) {\n"
      "    // Start up the gnome\n"
      "    gnome_init(\"stest\", \"1.0\", argc, argv);\n}");
  SSM(SCI_SETPROPERTY, (uptr_t)"fold", (sptr_t)"1");
  SSM(SCI_SETMARGINWIDTHN, 2, 1);
  SSM(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
  SSM(SCI_SETMARGINSENSITIVEN, 2, 1);
  SSM(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK, 0);
  SSM(SCI_SETFOCUS, 1, 0);
  scintilla_refresh(sci);

  printf("\033[?1000h"); // enable mouse press and release events
  //printf("\033[?1002h"); // enable mouse press, drag, and release events
  //printf("\033[?1003h"); // enable mouse move, press, drag, and release events
  mousemask(ALL_MOUSE_EVENTS, NULL);
  mouseinterval(0);

  // Non-UTF8 input.
  int c = 0;
  MEVENT mouse;
  WINDOW *win = scintilla_get_window(sci);
  while ((c = wgetch(win)) != 'q') {
    if (c != KEY_MOUSE) {
      if (c == KEY_UP) c = SCK_UP;
      else if (c == KEY_DOWN) c = SCK_DOWN;
      else if (c == KEY_LEFT) c = SCK_LEFT;
      else if (c == KEY_RIGHT) c = SCK_RIGHT;
      scintilla_send_key(sci, c, FALSE, FALSE, FALSE);
    } else if (getmouse(&mouse) == OK) {
      int event = SCM_DRAG, button = 0;
      if (mouse.bstate & BUTTON1_PRESSED)
        event = SCM_PRESS, button = 1;
      else if (mouse.bstate & BUTTON1_RELEASED)
        event = SCM_RELEASE, button = 1;
      struct timeval time = {0, 0};
      gettimeofday(&time, NULL);
      int millis = time.tv_sec * 1000 + time.tv_usec / 1000;
      scintilla_send_mouse(sci, event, millis, button, mouse.y, mouse.x,
                           mouse.bstate & BUTTON_SHIFT,
                           mouse.bstate & BUTTON_CTRL,
                           mouse.bstate & BUTTON_ALT);
    }
    scintilla_refresh(sci);
  }
  // UTF-8 input.
  //SSM(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
  //wint_t c = {0};
  //WINDOW *win = scintilla_get_window(sci);
  //while (c != 'q') {
  //  int status = wget_wch(win, &c);
  //  if (status == ERR)
  //    continue;
  //  else if (status == KEY_CODE_YES) {
  //    if (c == KEY_UP) c = SCK_UP;
  //    else if (c == KEY_DOWN) c = SCK_DOWN;
  //    else if (c == KEY_LEFT) c = SCK_LEFT;
  //    else if (c == KEY_RIGHT) c = SCK_RIGHT;
  //  }
  //  scintilla_send_key(sci, c, FALSE, FALSE, FALSE);
  //  scintilla_refresh(sci);
  //}

  printf("\033[?1000l"); // disable mouse press and release events
  //printf("\033[?1002l"); // disable mouse press, drag, and release events
  //printf("\033[?1003l"); // disable mouse move, press, drag, and release

  scintilla_delete(sci);
  endwin();

  return 0;
}

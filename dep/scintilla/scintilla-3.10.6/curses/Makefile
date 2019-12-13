# Copyright 2012-2019 Mitchell mitchell.att.foicica.com. See License.txt.

.SUFFIXES: .cxx .c .o .h .a

AR = ar
CC = gcc
CXX = g++
INCLUDEDIRS = -I ../include -I ../src -I ../lexlib
CFLAGS = -std=c99 -pedantic -Wall
CXXFLAGS = -std=c++11 -pedantic -DCURSES -DSCI_LEXER $(INCLUDEDIRS) -Wall
ifdef DEBUG
  CXXFLAGS += -DDEBUG -g
else
  CXXFLAGS += -DNDEBUG -Os
endif
CURSES_FLAGS =

scintilla = ../bin/scintilla.a
sci = AutoComplete.o CallTip.o CaseConvert.o CaseFolder.o Catalogue.o \
      CellBuffer.o CharacterCategory.o CharClassify.o ContractionState.o \
      DBCS.o Decoration.o Document.o EditModel.o Editor.o EditView.o \
      ExternalLexer.o Indicator.o KeyMap.o LineMarker.o MarginView.o PerLine.o \
      PositionCache.o RESearch.o RunStyles.o ScintillaBase.o Selection.o \
      Style.o UniConversion.o UniqueString.o ViewStyle.o XPM.o \
      Accessor.o CharacterSet.o DefaultLexer.o LexerBase.o LexerModule.o \
      LexerNoExceptions.o LexerSimple.o PropSetSimple.o StyleContext.o \
      WordList.o
lexers = $(addsuffix .o,$(basename $(sort $(notdir $(wildcard ../lexers/Lex*.cxx)))))
ifdef LPEG_LEXER
  CXXFLAGS += -DLPEG_LEXER -I ../lua/src
  LUA_CFLAGS = -I ../lua/src -DLUA_USE_POSIX -DLUA_USE_DLOPEN
  lua = lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o linit.o \
        llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o \
        ltable.o ltm.o lundump.o lvm.o lzio.o \
        lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o lmathlib.o \
        loadlib.o loslib.o lstrlib.o ltablib.o lutf8lib.o \
        lpcap.o lpcode.o lpprint.o lptree.o lpvm.o
endif

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

all: $(scintilla)
$(sci) $(lexers) ScintillaCurses.o: %.o: %.cxx
	$(CXX) $(CXXFLAGS) $(CURSES_FLAGS) -c $<
$(lua): %.o: ../lua/src/%.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -c $<
$(scintilla): $(sci) $(lexers) $(lua) ScintillaCurses.o
	$(AR) rc $@ $^
	touch $@
clean:
	rm -f *.o $(scintilla)

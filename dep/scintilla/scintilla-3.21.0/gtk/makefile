# Make file for Scintilla on Linux, macOS, or Windows
# @file makefile
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 4.8+ is used and changes will be needed to use other compilers.
# Clang 3.4+ can be used with CLANG=1 on command line.
# Builds for GTK+ 2 and 3. GTK 3 requires GTK3=1 on command line.
# Also works with ming32-make on Windows.

.PHONY: all clean analyze depend

.SUFFIXES: .cxx .c .o .h .a .list

srcdir ?= .
basedir = $(srcdir)/..

WARNINGS = -Wpedantic -Wall
ifdef CLANG
CXX = clang++
CC = clang
WARNINGS += -Wno-deprecated-register
ifdef windir
# Turn off some warnings that occur when Clang is being used on Windows where it
# is including Microsoft headers.
# incompatible-ms-struct is because more complex structs are not quite the same as MSVC
WARNINGS += -Wno-incompatible-ms-struct
# language-extension-token is because of __int64 in glib-2.0 glibconfig.h
WARNINGS += -Wno-language-extension-token
# register may be used in glib
# This produces a warning since -Wno-register is not valid for C files but it still works
WARNINGS += -Wno-register
DEFINES += -D_CRT_SECURE_NO_DEPRECATE
endif
# Can choose aspect to sanitize: address and undefined can simply change SANITIZE but for
# thread also need to create Position Independent Executable -> search online documentation
SANITIZE = address
#SANITIZE = undefined
BASE_FLAGS += -fsanitize=$(SANITIZE)
endif
ARFLAGS = rc
RANLIB ?= ranlib

GTK_VERSION = $(if $(GTK3),gtk+-3.0,gtk+-2.0)

# Environment variable windir always defined on Win32

# Enable Position Independent Code except on Windows where it is the default so the flag produces a warning
ifndef windir
BASE_FLAGS += -fPIC
endif

# Take care of changing Unix style '/' directory separator to '\' on Windows
normalize = $(if $(windir),$(subst /,\,$1),$1)

ifdef windir
CC = gcc
DEL = del /q
else
DEL = rm -f
endif
COMPLIB=$(basedir)/bin/scintilla.a

vpath %.h $(srcdir) $(basedir)/src $(basedir)/include $(basedir)/lexlib
vpath %.c $(srcdir)
vpath %.cxx $(srcdir) $(basedir)/src $(basedir)/lexlib $(basedir)/lexers

INCLUDES=-I $(basedir)/include -I $(basedir)/src -I $(basedir)/lexlib
DEFINES += -DGTK -DSCI_LEXER
BASE_FLAGS += $(WARNINGS)

ifdef NO_CXX11_REGEX
DEFINES += -DNO_CXX11_REGEX
endif

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-Os)

CXX_BASE_FLAGS =--std=c++11 $(BASE_FLAGS)
CXX_ALL_FLAGS =$(DEFINES) $(INCLUDES) $(CXX_BASE_FLAGS) $(CONFIG_FLAGS)
ifdef LPEG_LEXER
CXX_BASE_FLAGS += -DLPEG_LEXER -I ../lua/src
LUA_CFLAGS:=-fPIC -std=c99 -pedantic -Wall -I ../lua/src -DLUA_USE_POSIX -DLUA_USE_DLOPEN
LUA_OBJS:=lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o linit.o \
         llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o \
         ltable.o ltm.o lundump.o lvm.o lzio.o \
         lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o \
         lmathlib.o loadlib.o loslib.o lstrlib.o ltablib.o lutf8lib.o \
         lpcap.o lpcode.o lpprint.o lptree.o lpvm.o
endif


CONFIG_FLAGS:=$(shell pkg-config --cflags $(GTK_VERSION))
MARSHALLER=scintilla-marshal.o

all: $(COMPLIB)

clean:
	$(DEL) *.o $(call normalize,$(COMPLIB)) *.plist

%.o: %.cxx
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -c $<
%.o: %.c
	$(CC) $(DEFINES) $(INCLUDES) $(CONFIG_FLAGS) $(BASE_FLAGS) $(CFLAGS) -w -c $<
$(LUA_OBJS): %.o: ../lua/src/%.c
	$(CC) $(LUA_CFLAGS) $(CFLAGS) -c $<

GLIB_GENMARSHAL = glib-genmarshal
GLIB_GENMARSHAL_FLAGS = --prefix=scintilla_marshal

%.h: %.list
	$(GLIB_GENMARSHAL) --header $(GLIB_GENMARSHAL_FLAGS) $< > $@
%.c: %.list
	$(GLIB_GENMARSHAL) --body $(GLIB_GENMARSHAL_FLAGS) $< > $@

analyze:
	clang --analyze $(DEFINES) $(INCLUDES) $(CONFIG_FLAGS) $(CXX_BASE_FLAGS) $(CXXFLAGS) $(srcdir)/*.cxx $(basedir)/src/*.cxx $(basedir)/lexlib/*.cxx $(basedir)/lexers/*.cxx

depend deps.mak:
	python DepGen.py

LEX_OBJS:=$(addsuffix .o,$(basename $(sort $(notdir $(wildcard $(basedir)/lexers/Lex*.cxx)))))

# Required for base Scintilla
SRC_OBJS = \
	AutoComplete.o \
	CallTip.o \
	CaseConvert.o \
	CaseFolder.o \
	CellBuffer.o \
	CharacterCategory.o \
	CharacterSet.o \
	CharClassify.o \
	ContractionState.o \
	DBCS.o \
	Decoration.o \
	Document.o \
	EditModel.o \
	Editor.o \
	EditView.o \
	Indicator.o \
	KeyMap.o \
	LineMarker.o \
	MarginView.o \
	PerLine.o \
	PositionCache.o \
	RESearch.o \
	RunStyles.o \
	Selection.o \
	Style.o \
	UniConversion.o \
	UniqueString.o \
	ViewStyle.o \
	XPM.o

# Required by lexers
LEXLIB_OBJS = \
	Accessor.o \
	Catalogue.o \
	DefaultLexer.o \
	ExternalLexer.o \
	LexerBase.o \
	LexerModule.o \
	LexerSimple.o \
	PropSetSimple.o \
	StyleContext.o \
	WordList.o

GTK_OBJS = \
	ScintillaBase.o \
	PlatGTK.o \
	ScintillaGTK.o \
	ScintillaGTKAccessible.o

$(COMPLIB): $(SRC_OBJS) $(LEXLIB_OBJS) $(GTK_OBJS) $(MARSHALLER) $(LEX_OBJS) $(LUA_OBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

# Automatically generate header dependencies with "make deps"
include deps.mak

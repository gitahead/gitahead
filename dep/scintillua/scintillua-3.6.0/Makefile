# Copyright 2010-2015 Mitchell mitchell.att.foicica.com
# Make file for LexLPeg external lexer for Scintilla.

ifeq (win, $(findstring win, $(MAKECMDGOALS)))
  CC = i686-w64-mingw32-gcc
  CPP = i686-w64-mingw32-g++
  plat_flag =
  LUA_CFLAGS = -D_WIN32 -DWIN32
  LDFLAGS = -g -static -mwindows -s LexLPeg.def -Wl,--enable-stdcall-fixup
  lexer = lexers/LexLPeg.dll
  luadoc = luadoc_start.bat
else
  CC = gcc -fPIC
  CPP = g++ -fPIC
  plat_flag = -DGTK
  LUA_CFLAGS = -DLUA_USE_LINUX
  LDFLAGS = -g -Wl,-soname,liblexlpeg.so.0 -Wl,-fvisibility=hidden
  lexer = lexers/liblexlpeg.so
  luadoc = luadoc
endif

# Scintilla.
sci_flags = -g -pedantic $(plat_flag) -Iscintilla/include -Iscintilla/lexlib \
            -DSCI_LEXER -Wall
lex_objs = PropSetSimple.o WordList.o LexerModule.o LexerSimple.o LexerBase.o \
           Accessor.o

# Lua.
lua_objs = lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o \
           lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o \
           lundump.o lvm.o lzio.o \
           lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o ltablib.o \
           lstrlib.o loadlib.o loslib.o linit.o
lua_lib_objs = lpcap.o lpcode.o lpprint.o lptree.o lpvm.o

# Build.

all: $(lexer)
win32: $(lexer)
deps: scintilla lua lua/src/lib/lpeg doc/bombay

$(lex_objs): %.o: scintilla/lexlib/%.cxx ; $(CPP) $(sci_flags) -c $<
$(lua_objs): %.o: lua/src/%.c ; $(CC) -Os -Ilua/src $(LUA_CFLAGS) -c $<
$(lua_lib_objs): %.o: lua/src/lib/%.c ; $(CC) -Os -Ilua/src $(LUA_CFLAGS) -c $<
LexLPeg.o: LexLPeg.cxx
	$(CPP) $(sci_flags) $(LUA_CFLAGS) -DLPEG_LEXER_EXTERNAL -Ilua/src -c $<
$(lexer): $(lex_objs) $(lua_objs) $(lua_lib_objs) LexLPeg.o
	$(CPP) -shared $(LDFLAGS) -o $@ $^
clean: ; rm -f *.o

# Documentation.

doc: manual luadoc
manual: doc/*.md *.md | doc/bombay
	$| -d doc -t doc --title Scintillua $^
luadoc: lexers/lexer.lua scintillua.luadoc
	$(luadoc) -d doc -t doc --doclet doc/markdowndoc $^
cleandoc: ; rm -rf doc/manual.html doc/api.html

# Releases.

ifndef NIGHTLY
  basedir = scintillua_$(shell grep '^\#\#' CHANGELOG.md | head -1 | \
                               cut -d ' ' -f 2)
else
  basedir = scintillua_NIGHTLY_$(shell date +"%F")
endif

$(basedir): ; hg archive $@ -X ".hg*"
release: $(basedir)
	make deps doc
	make -j4
	make -j4 clean
	make -j4 CC=i586-mingw32msvc-gcc CPP=i586-mingw32msvc-g++ win32
	cp -r doc $<
	cp lexers/*.so lexers/*.dll $</lexers/
	zip -r /tmp/$<.zip $< && rm -rf $<

# External dependencies.

scintilla_tgz = scintilla360.tgz
lua_tgz = lua-5.1.4.tar.gz
lpeg_tgz = lpeg-0.12.2.tar.gz
bombay_zip = bombay.zip

$(scintilla_tgz): ; wget "http://prdownloads.sourceforge.net/scintilla/$@"
scintilla: | $(scintilla_tgz) ; mkdir $@ && tar xzf $| -C $@ && mv $@/*/* $@
$(lua_tgz): ; wget "http://www.lua.org/ftp/$@"
$(lpeg_tgz): ; wget "http://www.inf.puc-rio.br/~roberto/lpeg/$@"
lua: | $(lua_tgz) ; mkdir $@ && tar xzf $| -C $@ && mv $@/*/* $@
lua/src/lib/lpeg: | $(lpeg_tgz)
	mkdir -p $@ && tar xzf $| -C $@ && mv $@/*/*.c $@/*/*.h $(dir $@)
$(bombay_zip):
	wget "http://foicica.com/hg/bombay/archive/tip.zip" && mv tip.zip $@
doc/bombay: | $(bombay_zip)
	mkdir $(notdir $@) && unzip -d $(notdir $@) $| && \
		mv $(notdir $@)/*/* $(dir $@)

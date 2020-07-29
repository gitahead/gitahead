# Copyright 2018-2020 Mitchell mitchell.att.foicica.com. See License.txt.
# This makefile is used only for catching compile and test errors when
# backporting fixes and features from the main branch of Scintilla. It likely
# will not produce compiled targets that can be used by a Scintilla-based
# application.
# Usage: 
#   make -f check.mak            # check that everything compiles
#   make -f check.mak test       # run all tests
#   make -f check.mak bait       # run test GTK program
#   make -f check.mak dmapp      # run test Win32 program using WINE
#   make -f check.mak jinx       # run test curses program with Lua LPeg lexers
#   make -f check.mak gen        # update all version info and dates based on
#                                  version.txt and the current date
#   make -f check.mak zip        # make release archives
#   make -f check.mak upload     # upload HTML docs to sourceforge

.SUFFIXES: .cxx .c .o .h .a

INCLUDEDIRS = -Iinclude -Isrc -Ilexlib
CC = gcc
CXX = g++
AR = ar
CLANG_CC = clang --gcc-toolchain=$(shell pwd)/gcc/4.8.4
CLANG_CXX = clang++ --gcc-toolchain=$(shell pwd)/gcc/4.8.4
CFLAGS = -pedantic -Wall
CXXFLAGS = -std=c++11 -pedantic -pedantic-errors -DSCI_LEXER $(INCLUDEDIRS) \
           -DNDEBUG -Os -Wall
CROSS_WIN32 = i686-w64-mingw32-
CROSS_OSX = i386-apple-darwin9-
LINUX_CC = $(CC)-4.8
LINUX_CXX = $(CXX)-4.8
ifndef GTK3
  GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
else
  GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
endif

base_src_objs = AutoComplete.o CallTip.o CaseConvert.o CaseFolder.o \
                Catalogue.o CellBuffer.o CharClassify.o ContractionState.o \
                DBCS.o Decoration.o Document.o EditModel.o Editor.o EditView.o \
                ExternalLexer.o Indicator.o KeyMap.o LineMarker.o MarginView.o \
                PerLine.o PositionCache.o RESearch.o RunStyles.o \
                ScintillaBase.o Selection.o Style.o UniConversion.o \
                ViewStyle.o UniqueString.o XPM.o
base_lexlib_objs = Accessor.o CharacterCategory.o CharacterSet.o \
                   DefaultLexer.o LexerBase.o LexerModule.o \
                   LexerNoExceptions.o LexerSimple.o PropSetSimple.o \
                   StyleContext.o WordList.o
base_lexer_objs = $(addsuffix .o,$(basename $(sort $(notdir $(wildcard lexers/Lex*.cxx)))))

win32_src_objs = $(addprefix win32/, $(base_src_objs))
win32_lexlib_objs = $(addprefix win32/, $(base_lexlib_objs))
win32_lexer_objs = $(addprefix win32/, $(base_lexer_objs))
win32_plat_objs = win32/PlatWin.o win32/ScintillaWin.o win32/ScintillaDLL.o \
                  win32/HanjaDic.o
cocoa_src_objs = $(addprefix cocoa/, $(base_src_objs))
cocoa_lexlib_objs = $(addprefix cocoa/, $(base_lexlib_objs))
cocoa_lexer_objs = $(addprefix cocoa/, $(base_lexer_objs))
cocoa_plat_objs = cocoa/PlatCocoa.o cocoa/ScintillaCocoa.o cocoa/ScintillaView.o
gtk_src_objs = $(addprefix gtk/, $(base_src_objs))
gtk_lexlib_objs = $(addprefix gtk/, $(base_lexlib_objs))
gtk_lexer_objs = $(addprefix gtk/, $(base_lexer_objs))
gtk_plat_cc_objs = gtk/scintilla-marshal.o
gtk_plat_cxx_objs = gtk/PlatGTK.o gtk/ScintillaGTK.o \
                    gtk/ScintillaGTKAccessible.o
gtk_src_objs_clang = $(addprefix gtk/clang-, $(base_src_objs))
gtk_lexlib_objs_clang = $(addprefix gtk/clang-, $(base_lexlib_objs))
gtk_lexer_objs_clang = $(addprefix gtk/clang-, $(base_lexer_objs))
gtk_plat_cc_objs_clang = $(addprefix gtk/clang-, $(notdir $(gtk_plat_cc_objs)))
gtk_plat_cxx_objs_clang = $(addprefix gtk/clang-, $(notdir $(gtk_plat_cxx_objs)))
curses_src_objs = $(addprefix curses/, $(base_src_objs))
curses_lexlib_objs = $(addprefix curses/, $(base_lexlib_objs))
curses_lexer_objs = $(addprefix curses/, $(base_lexer_objs))
curses_plat_objs = curses/ScintillaCurses.o
curses_src_objs_clang = $(addprefix curses/clang-, $(base_src_objs))
curses_lexlib_objs_clang = $(addprefix curses/clang-, $(base_lexlib_objs))
curses_lexer_objs_clang = $(addprefix curses/clang-, $(base_lexer_objs))
curses_plat_objs_clang = $(addprefix curses/clang-, $(notdir $(curses_plat_objs)))

all: | /tmp/scintilla
	$(MAKE) -C $| -f check.mak -j4 bin/scintilla_win32.dll bin/scintilla_cocoa.a \
	  bin/scintilla_gtk.a bin/clang-scintilla_gtk.a bin/scintilla_curses.a \
		bin/clang-scintilla_curses.a qt qt-clang
/tmp/scintilla:
	cp -rs `pwd` $@
	cp -r $@/qt $@/qt-clang
	mkdir -p $@/gcc/4.8.4/include/c++/4.8.4
	cp -rs /usr/include/c++/4.8.4/* $@/gcc/4.8.4/include/c++/4.8.4
	cp -rs /usr/include/x86_64-linux-gnu/c++/4.8/* $@/gcc/4.8.4/include/c++/4.8.4
	mkdir -p $@/gcc/4.8.4/lib/gcc/x86_64-linux-gnu/4.8.4
	cp -rs /usr/lib/gcc/x86_64-linux-gnu/4.8.4/* \
	  $@/gcc/4.8.4/lib/gcc/x86_64-linux-gnu/4.8.4/

# Windows platform objects.
# Note: build a DLL to test linking.
bin/scintilla_win32.dll: $(win32_src_objs) $(win32_lexlib_objs) \
                         $(win32_lexer_objs) $(win32_plat_objs)
	$(CROSS_WIN32)$(CXX) -mwindows -shared -static -o $@ $^ -lgdi32 -luser32 \
		-limm32 -lole32 -luuid -loleaut32 -lmsimg32 -lstdc++
	touch $@
$(win32_src_objs): win32/%.o: src/%.cxx
$(win32_lexlib_objs): win32/%.o: lexlib/%.cxx
$(win32_lexer_objs): win32/%.o: lexers/%.cxx
$(win32_src_objs) $(win32_lexlib_objs) $(win32_lexer_objs):
	$(CROSS_WIN32)$(CXX) -c $(CXXFLAGS) $< -o $@
$(win32_plat_objs): win32/%.o: win32/%.cxx
	$(CROSS_WIN32)$(CXX) -c $(CXXFLAGS) $< -o $@

# MacOS platform objects.
bin/scintilla_cocoa.a: $(cocoa_src_objs) $(cocoa_lexlib_objs) \
                       $(cocoa_lexer_objs) #$(cocoa_plat_objs)
	$(CROSS_OSX)$(AR) rc $@ $^
	touch $@
$(cocoa_src_objs): cocoa/%.o: src/%.cxx
$(cocoa_lexlib_objs): cocoa/%.o: lexlib/%.cxx
$(cocoa_lexer_objs): cocoa/%.o: lexers/%.cxx
$(cocoa_src_objs) $(cocoa_lexlib_objs) $(cocoa_lexer_objs):
	$(CROSS_OSX)$(CXX) -c $(CXXFLAGS) $< -o $@
$(cocoa_plat_objs): cocoa/%.o: cocoa/%.mm
	$(CROSS_OSX)$(CXX)-gstdc++ -c $(CXXFLAGS) $< -o $@

# GTK platform objects.
bin/scintilla_gtk.a: $(gtk_src_objs) $(gtk_lexlib_objs) $(gtk_lexer_objs) \
                     $(gtk_plat_cc_objs) $(gtk_plat_cxx_objs)
bin/clang-scintilla_gtk.a: $(gtk_src_objs_clang) $(gtk_lexlib_objs_clang) \
                           $(gtk_lexer_objs_clang) $(gtk_plat_cc_objs_clang) \
                           $(gtk_plat_cxx_objs_clang)
$(gtk_src_objs): gtk/%.o: src/%.cxx
$(gtk_lexlib_objs): gtk/%.o: lexlib/%.cxx
$(gtk_lexer_objs): gtk/%.o: lexers/%.cxx
$(gtk_src_objs_clang): gtk/clang-%.o: src/%.cxx
$(gtk_lexlib_objs_clang): gtk/clang-%.o: lexlib/%.cxx
$(gtk_lexer_objs_clang): gtk/clang-%.o: lexers/%.cxx
$(gtk_plat_cc_objs): gtk/%.o: gtk/%.c
$(gtk_plat_cc_objs_clang): gtk/clang-%.o: gtk/%.c
$(gtk_plat_cxx_objs): gtk/%.o: gtk/%.cxx
$(gtk_plat_cxx_objs_clang): gtk/clang-%.o: gtk/%.cxx
$(gtk_src_objs) $(gtk_lexlib_objs) $(gtk_lexer_objs) $(gtk_plat_cxx_objs): CXX := $(LINUX_CXX)
$(gtk_plat_cc_objs): CC := $(LINUX_CC)
$(gtk_src_objs_clang) $(gtk_lexlib_objs_clang) $(gtk_lexer_objs_clang) $(gtk_plat_cxx_objs_clang): CXX := $(CLANG_CXX)
$(gtk_plat_cc_objs_clang): CC := $(CLANG_CC)
bin/scintilla_gtk.a bin/clang-scintilla_gtk.a:
	$(AR) rc $@ $^
	touch $@
$(gtk_src_objs) $(gtk_lexlib_objs) $(gtk_lexer_objs) \
$(gtk_src_objs_clang) $(gtk_lexlib_objs_clang) $(gtk_lexer_objs_clang):
	$(CXX) -c $(CXXFLAGS) -DGTK $< -o $@
$(gtk_plat_cc_objs) $(gtk_plat_cc_objs_clang):
	$(CC) -c $(CFLAGS) $(GTK_CFLAGS) $< -o $@
$(gtk_plat_cxx_objs) $(gtk_plat_cxx_objs_clang):
	$(CXX) -c $(CXXFLAGS) -DGTK $(GTK_CFLAGS) $< -o $@

# Curses platform objects.
bin/scintilla_curses.a: $(curses_src_objs) $(curses_lexlib_objs) \
                        $(curses_lexer_objs) $(curses_plat_objs)
bin/clang-scintilla_curses.a: $(curses_src_objs_clang) \
                              $(curses_lexlib_objs_clang) \
                              $(curses_lexer_objs_clang) \
                              $(curses_plat_objs_clang)
$(curses_src_objs): curses/%.o: src/%.cxx
$(curses_lexlib_objs): curses/%.o: lexlib/%.cxx
$(curses_lexer_objs): curses/%.o: lexers/%.cxx
$(curses_src_objs_clang): curses/clang-%.o: src/%.cxx
$(curses_lexlib_objs_clang): curses/clang-%.o: lexlib/%.cxx
$(curses_lexer_objs_clang): curses/clang-%.o: lexers/%.cxx
$(curses_plat_objs): curses/%.o: curses/%.cxx
$(curses_plat_objs_clang): curses/clang-%.o: curses/%.cxx
$(curses_src_objs) $(curses_lexlib_objs) $(curses_lexer_objs) $(curses_plat_objs): CXX := $(LINUX_CXX)
$(curses_src_objs_clang) $(curses_lexlib_objs_clang) $(curses_lexer_objs_clang) $(curses_plat_objs_clang): CXX := $(CLANG_CXX)
bin/scintilla_curses.a bin/clang-scintilla_curses.a:
	ar rc $@ $^
	touch $@
$(curses_src_objs) $(curses_lexlib_objs) $(curses_lexer_objs) \
$(curses_src_objs_clang) $(curses_lexlib_objs_clang) $(curses_lexer_objs_clang):
	$(CXX) -c $(CXXFLAGS) -DCURSES -DLPEG_LEXER $< -o $@
$(curses_plat_objs) $(curses_plat_objs_clang):
	$(CXX) -c $(CXXFLAGS) -DCURSES -Wno-unused-parameter $< -o $@

# Qt platform objects. (Note: requires libqt4-dev qt4-qmake.)
# Note: the Qt makefile produces shared libraries, so linking is tested.
.PHONY: qt qt-clang
qt: qt/ScintillaEditBase/Makefile
qt-clang: qt-clang/ScintillaEditBase/Makefile
qt qt-clang: ; $(MAKE) -C $(dir $<) -j8
qt/ScintillaEditBase/Makefile: ; cd qt/ScintillaEditBase && qmake
qt-clang/ScintillaEditBase/Makefile:
	cd $(dir $@) && qmake QMAKE_CC="$(CLANG_CC)" \
	  QMAKE_CXX="$(CLANG_CXX)"
	sed -i -e 's/libScintillaEditBase/clang-libScintillaEditBase/;' $@

# LexLPeg objects.
lua_objs = win32/LexLPeg.o cocoa/LexLPeg.o gtk/LexLPeg.o gtk/clang-LexLPeg.o \
           curses/LexLPeg.o curses/clang-LexLPeg.o
$(lua_objs): INCLUDEDIRS += -I/usr/include/lua5.1

deps: win32_deps cocoa_deps gtk_deps curses_deps
win32_deps: src/*.cxx lexlib/*.cxx lexers/*.cxx win32/*.cxx
	$(CROSS_WIN32)$(CXX) -MM $(CXXFLAGS) $^ | \
	  sed -e 's|^\([[:alnum:]-]\+\.o:\)|win32/\1|;' > checkdeps.mak
cocoa_deps: src/*.cxx lexlib/*.cxx lexers/*.cxx #cocoa/*.cxx
	$(CROSS_OSX)$(CXX) -MM $(CXXFLAGS) $^ | \
	  sed -e 's|^\([[:alnum:]-]\+\.o:\)|cocoa/\1|;' >> checkdeps.mak
gtk_deps: src/*.cxx lexlib/*.cxx lexers/*.cxx gtk/*.cxx
	$(CXX) -MM $(CXXFLAGS) $^ | \
	  sed -e 's|^\([[:alnum:]-]\+\.o:\)|gtk/\1|;' >> checkdeps.mak
curses_deps: src/*.cxx lexlib/*.cxx lexers/*.cxx curses/*.cxx
	$(CXX) -MM $(CXXFLAGS) $^ | \
	  sed -e 's|^\([[:alnum:]-]\+\.o:\)|curses/\1|;' >> checkdeps.mak

include checkdeps.mak

clean:
	rm -f bin/*.a bin/*.dll win32/*.o cocoa/*.o gtk/*.o curses/*.o
	make -f qt/ScintillaEditBase/Makefile clean
	make -f qt-clang/ScintillaEditBase/Makefile clean
	rm -rf /tmp/scintilla

.PHONY: test
test: | /tmp/scintilla
	make -C $|/test/unit CXX=$(LINUX_CXX) clean test
	cd $|/test && lua5.1 test_lexlua.lua

# Bait test program for GTK.
# Rebuild scintilla.a to ensure only GTK platform objects are inside.
bait: | /tmp/scintilla/gtk/bait
	rm -f $|/../../bin/scintilla.a
	make -C $(dir $|) -j4
	make -C $|
	$|/$@
/tmp/scintilla/gtk/bait: /tmp/scintilla/bait.zip | /tmp/scintilla
	mkdir $@
	unzip -d $@ $< && mv $@/bait/* $@ && rmdir $@/bait
	sed -i -e 's|scintilla|..|;' $@/Makefile
/tmp/scintilla/bait.zip: | /tmp/scintilla
	wget -O $@ https://www.scintilla.org/bait.zip

# dmapp test program for Win32.
# Since the test program is mean to be build on Windows, create a makefile for 
# cross-compiling from Linux.
dmapp_dlls = /tmp/scintilla/win32/dmapp/SciLexer.dll
define _dmapp_makefile
ALL: DMApp.exe
DMApp.o: DMApp.cxx ; i686-w64-mingw32-g++ -std=c++11 -I ../../include -c $< -o $@
DMApp_rc.o: DMApp.rc ; i686-w64-mingw32-windres $< $@
DMApp.exe: DMApp.o DMApp_rc.o ; i686-w64-mingw32-g++ $^ -o $@ -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinmm -lcomctl32 -ladvapi32 -limm32 -lshell32 -lole32 -lstdc++
clean: ; rm -f *.exe *.o *.res
endef
export dmapp_makefile = $(value _dmapp_makefile)
dmapp: /tmp/scintilla/win32/dmapp/DMApp.exe
	WINEPREFIX=$(dir $<)/wine WINEARCH=win32 wine $<
lexilla: /tmp/scintilla/win32/dmapp/DMApp.exe
	WINEPREFIX=$(dir $<)/wine WINEARCH=win32 wine $< X
/tmp/scintilla/win32/dmapp/DMApp.exe: $(dmapp_dlls) | /tmp/scintilla/win32/dmapp
	make -C $|
$(dmapp_dlls): | /tmp/scintilla/win32/dmapp
	rm -f $|/../Catalogue.o $|/../ScintillaBase.o
	make -C $(dir $|) CXX=/opt/mingw-w64/bin/i686-w64-mingw32-g++ \
		AR=/opt/mingw-w64/bin/i686-w64-mingw32-ar \
		RANLIB=/opt/mingw-w64/bin/i686-w64-mingw32-ranlib \
		WINDRES=/opt/mingw-w64/bin/i686-w64-mingw32-windres -j4
	cd $| && ln -sf ../../bin/*.dll .
/tmp/scintilla/win32/dmapp: /tmp/scintilla/dmapp.zip | /tmp/scintilla
	mkdir $@
	unzip -d $@ $<
	sed -i -e '/SCI_SETSTYLEBITS/d' $@/DMApp.cxx
	echo "$$dmapp_makefile" > $@/makefile
/tmp/scintilla/dmapp.zip: | /tmp/scintilla
	wget -O $@ https://www.scintilla.org/dmapp.zip

# jinx test program for curses.
jinx: | /tmp/scintilla/curses/jinx
	rm -f $|/../../bin/scintilla.a $|/../LexLPeg.o
	make -C $(dir $|) -j4 LPEG_LEXER=1 DEBUG=1
	make -C $| LPEG_LEXER=1 DEBUG=1
	cd $| && ./$@

version = $(shell grep -o '[0-9]\+' version.txt)
date = $(shell date +'%Y%m%d')
gen:
	@echo "Using version $(version) with date $(date)"
	sed -i -e 's/content="[0-9]\+"/content="$(date)"/;' doc/index.html
	cd scripts && python LexGen.py

releasedir = /tmp/scintilla$(version)
$(releasedir): ; hg archive $@
zip: $(releasedir)
	cd /tmp && tar czf $<.tgz $(notdir $<)
	cd /tmp && zip -r $<.zip $(notdir $<)
	rm -r $<

upload: LongTermDownload.html doc/ScintillaHistory.html doc/ScintillaDoc.html \
        doc/StyleMetadata.html doc/LPegLexer.html
	scp $^ foicica@web.sourceforge.net:/home/project-web/scintilla/htdocs/

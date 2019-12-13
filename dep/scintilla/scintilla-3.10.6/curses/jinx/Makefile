# Copyright 2012-2019 Mitchell mitchell.att.foicica.com. See License.txt.

CC = gcc
CXX = g++
INCLUDEDIRS = -I ../../include -I ../../src -I ../../lexlib -I ../
CFLAGS = -DCURSES -DSCI_LEXER -Wall $(INCLUDEDIRS)
CXXFLAGS = $(CFLAGS)
ifdef LPEG_LEXER
  CFLAGS += -DLPEG_LEXER -I ../src/lua
endif

scintilla = ../../bin/scintilla.a

all: jinx
jinx.o: jinx.c ; $(CC) $(CFLAGS) -c $<
jinx: jinx.o $(scintilla) ; $(CXX) $^ -o $@ -lncursesw
clean: ; rm -f jinx *.o

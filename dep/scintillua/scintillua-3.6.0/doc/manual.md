# Installation

## Drop-in External Lexer

The environments of Scintilla-based applications vary greatly, but as long as
external lexers are supported, Scintillua can be dropped into any existing
installation.

### Using Scintillua with SciTE

[SciTE][] is the SCIntilla based Text Editor. Scintillua can be easily dropped
into any SciTE installation of version 2.25 or higher with or without
administrative privilages.

[SciTE]: http://scintilla.org/SciTE.html

#### Installing for All Users

Installing Scintillua for all users will likely require administrator
privilages.

1. Locate your SciTE installation, typically `C:\Program Files\SciTE\` for
   Windows and `/usr/share/scite/` for Linux.
2. Unpack Scintillua to a temporary directory and move the `lexers/` directory
   to the root of your SciTE installation.
3. Add the following to the end of your `SciTEGlobal.properties`:

       import lexers/lpeg

#### Installing for One User

Installing Scintillua for one user will not require administrator privilages
provided you have access to the user's home directory. This is always the case
for the current user.

1. Locate your home directory, typically `C:\Documents and Settings\username\`
   for Windows XP, `C:\Users\username\` for Windows 7, and `/home/username/` for
   Linux. This is also likely stored in your operating system's `HOME`
   environment variable.
2. Unpack Scintillua to a temporary directory and move the `lexers/` directory
   to the root of your home directory or any other place you would prefer.
3. Add the following to the end of your `SciTEUser.properties` on Windows or
   `.SciTEUser.properties` on Linux:

       import lexers/lpeg

   but change `lexers` to the directory you put `lexers/` if not in the root of
   your home directory.
4. Open Scintillua's `lexers/lpeg.properties` file for editing and change the
   line:

       lexer.lpeg.home=$(SciteDefaultHome)/lexers

   to

       lexer.lpeg.home=$(SciteUserHome)/lexers

   or to wherever you preferred to put Scintillua's `lexers/` directory.
5. If you are running a 64-bit Linux operating system, you will have to rename
   `lexers/liblexlpeg.x86_64.so` to `lexers/liblexlpeg.so` so the 64-bit
   external lexer will be loaded.

#### Usage Notes

By default, SciTE will use LPeg lexers whenever possible, as indicated in
`lexers/lpeg.properties`, and fall back onto Scintilla's lexers when necessary.
If an LPeg lexer is loaded but you prefer to use the Scintilla lexer instead,
edit `lexers/lpeg.properties` by commenting out the desired lexer's
`file.pattern.lexer` and `lexer.$(file.pattern.lexer)` property lines.

Themes are located in the `lexers/themes/` folder and set in
`lexers/lpeg.properties`. You can define the overall fonts and styles used by
LPeg lexers using themes.

#### Troubleshooting

If you get incorrect or no syntax highlighting, please check the following:

1. Does the language in question have a Lua LPeg lexer in Scintillua's `lexers/`
   directory? If not, you will have to [write one][].
2. Does Scintillua's `lexers/lpeg.properties` have your language's file
   extension defined? If not, add it to the `file.patterns.lexer` property.
3. Does the file extension recognized in Scintillua's `lexers/lpeg.properties`
   correspond to the language in question? If not, add or re-assign it to the
   appropriate Lua LPeg lexer.

Please note file extensions are not an exact science so `lexers/lpeg.properties`
may not have the most up to date or appropriate extensions defined. Feel free
to [submit][] corrections or additions.

Any Scintilla lexer-specific features in SciTE do not work in LPeg lexers. These
include, but are not limited to:

* Style, keyword, and folding properties in `*.properties` files.
* Python colon matching.
* HTML/XML tag auto-completion.

[write one]: api.html#lexer
[submit]: README.html#Contact

### Using Scintillua with Other Apps

In order to use Scintillua with an instance of Scintilla, the following
[Scintilla properties][] *must* be set for the external lexer after
initialization:

* `lexer.lpeg.home`

  The directory containing the Lua LPeg lexers. For application developers, this
  is the path of where you included Scintillua's `lexers/` directory in your
  application's installation location. For end-users, this is where you chose to
  put Scintillua's `lexers/` folder. Please see the SciTE examples above for
  more information.

[Scintilla properties]: http://scintilla.org/ScintillaDoc.html#SCI_SETPROPERTY

The following properties are optional and may or may not be set:

* `lexer.lpeg.color.theme`

  The color theme to use. Color themes are located in the `lexers/themes/`
  directory. Currently supported themes are `light`, `dark`, and `scite`. Your
  application can define colors and styles manually through Scintilla
  properties. The theme files have examples.

* `fold.by.indentation`

  For LPeg lexers that do not have a folder, if `fold.by.indentation` is set to
  `1`, folding is done based on indentation level (like Python). The default is
  `0`.

* `fold.line.comments`

  If `fold.line.comments` is set to `1`, multiple, consecutive, line comments
  are folded and only the top-level comment is shown. There is a small
  performance penalty for large source files when this option and folding are
  enabled. The default is `0`.

* `fold.on.zero.sum.lines`

  If `fold.on.zero.sum.lines` is set to `1`, lines that contain both an ending
  and starting fold point are marked as fold points. For example, the C line
  `} else {` would be marked as a fold point. The default is `0`.

### Using Scintillua as a Lua Library

In order to use Scintillua as a Lua library, simply place the `lexers/`
directory in your Lua path (or modify Lua's `package.path` accordingly),
`require()` the library and [`load()`][] a lexer, and call that lexer's
[`lex()`][] function. Here is an example interactive Lua session doing this:

    $> lua
    Lua 5.1.4  Copyright (C) 1994-2008 Lua.org, PUC-Rio
    > lexer_path = '/home/mitchell/code/scintillua/lexers/?.lua'
    > package.path = package.path..';'..lexer_path
    > c = require('lexer').load('ansi_c')
    > tokens = c:lex('int void main() { return 0; }')
    > for i = 1, #tokens, 2 do print(tokens[i], tokens[i+1]) end
    type	4
    ansi_c_whitespace	5
    type	9
    ansi_c_whitespace	10
    identifier	14
    operator	15
    operator	16
    ansi_c_whitespace	17
    operator	18
    ansi_c_whitespace	19
    keyword	25
    ansi_c_whitespace	26
    number	27
    operator	28
    ansi_c_whitespace	29
    operator	30

[`load()`]: api.html#lexer.load
[`lex()`]: api.html#lexer.lex

## Compiling Scintillua with Scintilla

For native LPeg support, developers can add the included `LexLPeg.cxx` Scintilla
lexer and download and include [Lua][] and [LPeg][] sources files to their
toolchains for compiling Scintilla. Scintillua supports both Lua 5.1 and
Lua 5.2. A sample portion of a `Makefile` with Lua 5.2 is shown below.

    # Sample Makefile portion for compiling Scintillua with Scintilla

    # ...

    SCIFLAGS = [flags used to compile Scintilla]
    SCINTILLUA_LEXER = LexLPeg.o
    SCINTILLUA_SRC = LexLPeg.cxx
    LUAFLAGS = -Iscintillua/lua/src
    LUA_OBJS = lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o \
      linit.o llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o \
      ltable.o ltm.o lundump.o lvm.o lzio.o \
      lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o lmathlib.o \
      loadlib.o loslib.o ltablib.o lstrlib.o \
      lpeg.o
    LUA_SRCS = scintillua/lua/src/*.c scintillua/lua/src/lib/*.c
    $(SCINTILLUA_LEXER): $(SCINTILLUA_SRC)
    	g++ $(SCIFLAGS) $(LUAFLAGS) -DLPEG_LEXER -c $< -o $@
    $(LUA_OBJS): $(LUA_SRCS)
    	gcc $(LUAFLAGS) $(INCLUDEDIRS) -c $^

    # ...

    [your app]: [your dependencies] $(SCINTILLUA_LEXER) $(LUA_OBJS)

[Lua]: http://lua.org
[LPeg]: http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html

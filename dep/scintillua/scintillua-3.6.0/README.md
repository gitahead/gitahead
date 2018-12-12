# Scintillua

## Overview

Scintillua adds dynamic [Lua][] [LPeg][] lexers to [Scintilla][]. It is the
quickest way to add new or customized syntax highlighting and code folding for
programming languages to any Scintilla-based text editor or IDE. Scintillua was
designed to be dropped into or compiled with any Scintilla environment.

Scintillua may also be used as a Lua library for obtaining syntax highlighting
information of source code snippets.

[Lua]: http://lua.org
[LPeg]: http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
[Scintilla]: http://scintilla.org

## Features

* Drop-in installation in most Scintilla environments -- no modifications to
  Scintilla are necessary.
* Support for over 90 programming languages.
* Easy lexer embedding for multi-language lexers.
* Universal color themes.
* Comparable speed to native Scintilla lexers.
* Can be used as a Lua library (Scintilla is not required).

## Requirements

Scintillua only requires Scintilla 2.25 or greater. Lua and LPeg have been
pre-compiled into the external lexer and you can download their source files
using the links above should you choose to compile Scintillua directly into a
Scintilla-based application.

When used a Lua library, Scintillua requires Lua 5.1 or greater and
[LPeg][] 0.12 or greater.

[LPeg]: http://www.inf.puc-rio.br/~roberto/lpeg/

## Download

Download Scintillua from the project's [download page][] or from these quick
links:

Stable Builds

* [Win32 and Linux][]

Unstable Builds

* [Win32 and Linux Nightly][]

_Warning_: nightly builds are untested, may have bugs, and are the absolute
cutting-edge versions of Scintillua. Do not use them in production, but for
testing purposes only.

[download page]: http://foicica.com/scintillua/download
[Win32 and Linux]: download/scintillua_LATEST.zip
[Win32 and Linux Nightly]: download/scintillua_NIGHTLY.zip

## Installation and Usage

Scintillua comes with a manual and API documentation in the `doc/` directory.
They are also available [online][].

[online]: http://foicica.com/scintillua

## Contact

Contact me by email: mitchell.att.foicica.com.

There is also a [mailing list][].

[mailing list]: http://foicica.com/lists

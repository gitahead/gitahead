# Changelog

[Atom Feed][]

[Atom Feed]: feed

## 3.6.0-1 (03 Aug 2015)

Download:

* [Scintillua 3.6.0-1][]

Bugfixes:

* None.

Changes:

* Improved performance in some scripting-language lexers.
* Updated Python lexer.
* Updated to [Scintilla][]/[SciTE][] 3.6.0.

[Scintillua 3.6.0-1]: download/scintillua_3.6.0-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.7-1 (23 Jun 2015)

Download:

* [Scintillua 3.5.7-1][]

Bugfixes:

* None.

Changes:

* Added Windows Script File lexer.
* Updated to [Scintilla][]/[SciTE][] 3.5.7.

[Scintillua 3.5.7-1]: download/scintillua_3.5.7-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.6-1 (26 May 2015)

Download:

* [Scintillua 3.5.6-1][]

Bugfixes:

* Fixed ASP, Applescript, and Perl lexers.
* Fixed segfault in parsing some instances of style definitions.

Changes:

* Added Elixir lexer.
* Updated to [Scintilla][]/[SciTE][] 3.5.6.

[Scintillua 3.5.6-1]: download/scintillua_3.5.6-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.5-1 (18 Apr 2015)

Download:

* [Scintillua 3.5.5-1][]

Bugfixes:

* Fixed Perl lexer corner-case.
* VB lexer keywords are case-insensitive now.

Changes:

* Renamed Nimrod lexer to Nim.
* Added Rust lexer.
* Added TOML lexer.
* Lexers that fold by indentation should make use of [`_FOLDBYINDENTATION`][]
  field now.
* Added PowerShell lexer.
* Updated to [Scintilla][]/[SciTE][] 3.5.5.

[Scintillua 3.5.5-1]: download/scintillua_3.5.5-1.zip
[`_FOLDBYINDENTATION`]: api.html#lexer.Fold.by.Indentation
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.4-1 (09 Mar 2015)

Download:

* [Scintillua 3.5.4-1][]

Bugfixes:

* Improved `fold.by.indentation`.

Changes:

* Updated PHP and Python lexers.
* Added Fish lexer.
* Removed extinct B lexer.
* Updated to [LPeg][] 0.12.2.
* Updated to [Scintilla][]/[SciTE][] 3.5.4.

[Scintillua 3.5.4-1]: download/scintillua_3.5.4-1.zip
[LPeg]: http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.3-1 (20 Jan 2015)

Download:

* [Scintillua 3.5.3-1][]

Bugfixes:

* Fixed bug in overwriting fold levels set by custom fold functions.

Changes:

* Added vCard and Texinfo lexers.
* Updates to allow Scintillua to be compiled against Lua 5.3.
* Updated Lua lexer for Lua 5.3.
* Updated to [Scintilla][]/[SciTE][] 3.5.3.

[Scintillua 3.5.3-1]: download/scintillua_3.5.3-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.2-1 (10 Dec 2014)

Download:

* [Scintillua 3.5.2-1][]

Bugfixes:

* Improved folding by indentation.

Changes:

* Updated Tcl lexer.
* Added `fold.on.zero.sum.line` property for folding on `} else {`-style lines.
* Updated to [Scintilla][]/[SciTE][] 3.5.2.

[Scintillua 3.5.2-1]: download/scintillua_3.5.2-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.1-1 (01 Oct 2014)

Download:

* [Scintillua 3.5.1-1][]

Bugfixes:

* None.

Changes:

* Added Xtend lexer.
* Improved performance for lexers with no grammars and no fold rules.
* Updated to [Scintilla][]/[SciTE][] 3.5.1.

[Scintillua 3.5.1-1]: download/scintillua_3.5.1-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.5.0-1 (01 Sep 2014)

Download:

* [Scintillua 3.5.0-1][]

Bugfixes:

* None.

Changes:

* Updated to [LPeg][] 0.12.
* Updated to [Scintilla][]/[SciTE][] 3.5.0.

[Scintillua 3.5.0-1]: download/scintillua_3.5.0-1.zip
[LPeg]: http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.4.4-1 (04 Jul 2014)

Download:

* [Scintillua 3.4.4-1][]

Bugfixes:

* Fixed cases of incorrect Markdown header highlighting.
* Fixed some folding by indentation edge cases.
* Fixed `#RRGGBB` color interpretation for styles.
* Fixed Bash heredoc highlighting.

Changes:

* Added reST and YAML lexers.
* Updated D lexer.
* Updated to [Scintilla][]/[SciTE][] 3.4.4.

[Scintillua 3.4.4-1]: download/scintillua_3.4.4-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 3.3.9-1 (05 Feb 2014)

Download:

* [Scintillua 3.3.9-1][]

Bugfixes:

* None.

Changes:

* Updated HTML, LaTeX, and Go lexers.
* Enable Scintillua to be used as a stand-alone [Lua library][].
* Scintillua can accept and use [external Lua states][].

[Scintillua 3.3.9-1]: download/scintillua3.3.9-1.zip
[Lua library]: manual.html#Using.Scintillua.as.a.Lua.Library
[external Lua states]: api.html#SCI_CHANGELEXERSTATE

## 3.3.7-1 (21 Dec 2013)

Scintillua 3.3.7-1 is a major change from 3.3.2-1. It has a completely new
[theme implementation][] and many lexer structure and API changes. Custom lexers
and themes will need to be updated.

Download:

* [Scintillua 3.3.7-1][]

Bugfixes:

* Ensure the default style is not considered a whitespace style in
  multi-language lexers.
* Fixed occasional crash when getting the lexer name in a multi-language lexer.
* Disable folding when `fold` property is `0`.
* HTML and XML lexers maintain their states better.
* Fixed slowdown in processing long lines for folding.
* Fixed slowdown with large HTML files.

Changes:

* Completely new [theme implementation][]; removed `lexer.style()` and
  `lexer.color()` functions.
* Changed [`lexer._tokenstyles`][] to be a map instead of a list.
* Changed `lexer.get_fold_level()`, `lexer.get_indent_amount()`,
  `lexer.get_property()`, and `lexer.get_style_at()` functions to be
  [`lexer.fold_level`][], [`lexer.indent_amount`][], [`lexer.property`][], and
  [`lexer.style_at`][] tables, respectively.
* Added [`lexer.property_int`][] and [`lexer.property_expanded`][] tables.
* Changed API for [`lexer.delimited_range()`][] and [`lexer.nested_pair()`][].
* Only enable `fold.by.indentation` property by default in
  whitespace-significant languages.
* Updated D lexer.
* Added Nimrod lexer.
* Added additional parameter to [`lexer.load()`][] to allow child lexers to be
  embedded multiple times with different start/end tokens.
* Lexers do not need an "any\_char" [rule][] anymore; it is included by default.
* [Child lexers][] do not need an explicit `M._lexer = parent` declaration
  anymore; it is done automatically.
* Added NASM Assembly lexer.
* Separated C/C++ lexer into ANSI C and C++ lexers.
* Added Dart lexer.
* Renamed "hypertext" and "Io" lexers to "html" and "io\_lang" internally.

[theme implementation]: api.html#lexer.Styles.and.Styling
[Scintillua 3.3.7-1]: download/scintillua3.3.7-1.zip
[`lexer._tokenstyles`]: api.html#lexer.Token.Styles
[`lexer.fold_level`]: api.html#lexer.fold_level
[`lexer.indent_amount`]: api.html#lexer.indent_amount
[`lexer.property`]: api.html#lexer.property
[`lexer.style_at`]: api.html#lexer.style_at
[`lexer.property_int`]: api.html#lexer.property_int
[`lexer.property_expanded`]: api.html#lexer.property_expanded
[`lexer.delimited_range()`]: api.html#lexer.delimited_range
[`lexer.nested_pair()`]: api.html#lexer.nested_pair
[`lexer.load()`]: api.html#lexer.load
[rule]: api.html#lexer.Rules
[Child lexers]: api.html#lexer.Child.Lexer

## 3.3.2-1 (25 May 2013)

Download:

* [Scintillua 3.3.2-1][]

Bugfixes:

* None.

Changes:

* No need for '!' in front of font faces in GTK anymore.
* Scintillua supports multiple curses platforms, not just ncurses.
* [SCI\_GETLEXERLANGUAGE][] returns "lexer/current" for multi-lang lexers.
* Updated D lexer.

[Scintillua 3.3.2-1]: download/scintillua3.3.2-1.zip
[SCI\_GETLEXERLANGUAGE]: api.html#SCI_GETLEXERLANGUAGE

## 3.3.0-1 (31 Mar 2013)

Download:

* [Scintillua 3.3.0-1][]

Bugfixes:

* Fixed crash when attempting to load a non-existant lexer.
* Fixed CSS preprocessor styling.

Changes:

* Added Less, Literal Coffeescript, and Sass lexers.

[Scintillua 3.3.0-1]: download/scintillua3.3.0-1.zip

## 3.2.4-1 (18 Jan 2013)

Download:

* [Scintillua 3.2.4-1][]

Bugfixes:

* Fixed some operators in Bash lexer.

Changes:

* Rewrote Makefile lexer.
* Rewrote documentation.
* Improved speed and memory usage of lexers.

[Scintillua 3.2.4-1]: download/scintillua3.2.4-1.zip

## 3.2.3-1 (22 Oct 2012)

Download:

* [Scintillua 3.2.3-1][]

Bugfixes:

* Include `_` as identifier char in Desktop lexer.

Changes:

* Copied `container` lexer to a new `text` lexer for containers that prefer to
  use the latter.
* Added SciTE usage note on themes.

[Scintillua 3.2.3-1]: download/scintillua3.2.3-1.zip

## 3.2.2-1 (31 Aug 2012)

Download:

* [Scintillua 3.2.2-1][]

Bugfixes:

* Fixed bug with `$$` variables in Perl lexer.

Changes:

* Added support for ncurses via [scinterm][].
* Added `__DATA__` and `__END__` markers to Perl lexer.
* Added new [`lexer.last_char_includes()`][] function for better regex
  detection.
* Updated AWK lexer.

[Scintillua 3.2.2-1]: download/scintillua3.2.2-1.zip
[scinterm]: http://foicica.com/scinterm
[`lexer.last_char_includes()`]: api.html#lexer.last_char_includes

## 3.2.1-1 (15 Jul 2012)

Download:

* [Scintillua 3.2.1-1][]

Bugfixes:

* None.

Changes:

* Updated AWK lexer.
* Updated HTML lexer to recognize HTML5 'script' and 'style' tags.

[Scintillua 3.2.1-1]: download/scintillua3.2.1-1.zip

## 3.2.0-1 (01 Jun 2012)

Download:

* [Scintillua 3.2.0-1][]

Bugfixes:

* Fixed bug with SciTE italic and underlined style properties.

Changes:

* Identify more file extensions.
* Updated Batch lexer.

[Scintillua 3.2.0-1]: download/scintillua3.2.0-1.zip

## 3.1.0-1 (23 Apr 2012)

Download:

* [Scintillua 3.1.0-1][]

Bugfixes:

* Fixed bug with Python lexer identification in SciTE.

Changes:

* Improved the speed of simple code folding.
* Check for lexer grammar before lexing.

[Scintillua 3.1.0-1]: download/scintillua3.1.0-1.zip

## 3.0.4-1 (11 Mar 2012)

Download:

* [Scintillua 3.0.4-1][]

Bugfixes:

* None.

Changes:

* Allow container styling.
* Updated VB and VBScript lexers.
* All new documentation in the `doc/` directory.

[Scintillua 3.0.4-1]: download/scintillua3.0.4-1.zip

## 3.0.3-1 (28 Jan 2012)

Download:

* [Scintillua 3.0.3-1][]

Bugfixes:

* Fixed bug in Matlab lexer for operators.

Changes:

* Removed unused Apache conf lexer.
* Updated D lexer.
* Added ChucK lexer.

[Scintillua 3.0.3-1]: download/scintillua3.0.3-1.zip

## 3.0.2-1 (08 Dec 2011)

Download:

* [Scintillua 3.0.2-1][]

Bugfixes:

* Detect and use Scala lexer.
* Fixed bug with folding line comments.
* Fixed multi-line delimited and token strings in D lexer.
* Detect and use XML lexer.
* Fixed highlighting of variables in Bash.

Changes:

* Added `l.REGEX` and `l.LABEL` [tokens][].
* All lexer `_tokenstyles` tables use standard styles.
* Removed `l.style_char` style.
* All new light and dark themes.
* Added Lua libraries and library functions to Lua lexer.
* Updated lexers and [API documentation][] to [Lua 5.2][].

[Scintillua 3.0.2-1]: download/scintillua3.0.2-1.zip
[tokens]: api.html#lexer.Tokens
[API documentation]: api.html#lexer
[Lua 5.2]: http://www.lua.org/manual/5.2/

## 3.0.0-1 (01 Nov 2011)

Download:

* [Scintillua 3.0.0-1][]

Bugfixes:

* None.

Changes:

* None.

[Scintillua 3.0.0-1]: download/scintillua3.0.0-1.zip

## 2.29-1 (19 Sep 2011)

Download:

* [Scintillua 2.29-1][]

Bugfixes:

* Fixed Lua long comment folding bug.
* Fixed a segfault when `props` is `null` (C++ containers).
* Fixed Markdown lexer styles.
* Fixed bug in folding single HTML/XML tags.
* Fixed some general bugs in folding.
* Fixed Scala symbol highlighting.

Changes:

* Updated Coffeescript lexer.
* Added HTML5 data attributes to HTML lexer.
* Multiple single-line comments can be folded with the `fold.line.comments`
  property set to 1.
* Added ConTeXt lexer.
* Updated LaTeX and TeX lexers.
* Added `l.style_embedded` to `themes/scite.lua` theme.

[Scintillua 2.29-1]: download/scintillua229-1.zip

## 2.27-1 (20 Jun 2011)

Download:

* [Scintillua 2.27-1][]

Bugfixes:

* Colors are now styled correctly in the Properties lexer.

Changes:

* Added Scala lexer.

[Scintillua 2.27-1]: download/scintillua227-1.zip

## 2.26-1 (10 Jun 2011)

Download:

* [Scintillua 2.26-1][]

Bugfixes:

* Fixed bug in `fold.by.indentation`.

Changes:

* [`get_style_at()`][] returns a string, not an integer.
* Added regex support for Coffeescript lexer.
* Embed Coffeescript lexer in HTML lexer.
* Writing custom folding for lexers is much [easier][] now.
* Added native folding for more than 60% of existing lexers. The rest still use
  folding by indentation by default.

[Scintillua 2.26-1]: download/scintillua226-1.zip
[`get_style_at()`]: api.html#lexer.style_at
[easier]: api.html#lexer.Code.Folding

## 2.25-1 (20 Mar 2011)

Download:

* [Scintillua 2.25-1][]

Bugfixes:

* LPeg lexer restores properly for SciTE.
* Fixed bug with nested embedded lexers.
* Re-init immediately upon setting `lexer.name` property.

Changes:

* Added primitive classes as types in Java lexer.
* Updated BibTeX lexer.
* Added Ruby on Rails lexer, use it instead of Ruby lexer in RHTML lexer.
* Updated `lpeg.properties` file with SciTE changes.

[Scintillua 2.25-1]: download/scintillua225-1.zip

## 2.24-1 (03 Feb 2011)

Download:

* [Scintillua 2.24-1][]

Bugfixes:

* Fixed comment bug in CAML lexer.

Changes:

* Added Markdown, BibTeX, CMake, CUDA, Desktop Entry, F#, GLSL, and Nemerle
  lexers.
* HTML lexer is more flexible.
* Update Lua functions and constants to Lua 5.1.

[Scintillua 2.24-1]: download/scintillua224-1.zip

## 2.23-1 (07 Dec 2010)

Download:

* [Scintillua 2.23-1][]

Bugfixes:

* Fixed bug in Tcl lexer with comments.

Changes:

* Renamed `MAC` flag to `OSX`.
* Removed unused Errorlist and Maxima lexers.

[Scintillua 2.23-1]: download/scintillua223-1.zip

## 2.22-1 (27 Oct 2010)

Download:

* [Scintillua 2.22-1][]

Bugfixes:

* Comments do not need to begin the line in Properties lexer.
* Fixed bug caused by not properly resetting styles.

Changes:

* Added coffeescript lexer.
* Updated D and Java lexers.
* Multi-language lexers are as fast as single language lexers.
* Added JSP lexer.
* Updated XML lexer.
* Scintillua can be dropped into a [SciTE][] install.

[Scintillua 2.22-1]: download/scintillua222-1.zip
[SciTE]: http://scintilla.org/SciTE.html

## 2.22-pre-1 (13 Sep 2010)

Download:

* [Scintillua 2.22-pre-1][]

Bugfixes:

* Do not crash if LexLPeg properties are not set correctly.

Changes:

* No need to modify parent `_RULES` from child lexer.
* Renamed `lexers/ocaml.lua` to `lexers/caml.lua` and `lexers/postscript.lua` to
  `lexers/ps.lua` to conform to Scintilla names.

[Scintillua 2.22-pre-1]: download/scintillua222-pre-1.zip

## 2.21-1 (01 Sep 2010)

Bugfixes:

* Handle strings properly in Groovy and Vala lexers.

Changes:

* `LexLPeg.cxx` can be compiled as an external lexer.

## 2.20-1 (17 Aug 2010)

Download:

* [Scintillua 2.20-1][]

Bugfixes:

* Fixed bug with child's main lexer not having a `_tokenstyles` table.

Changes:

* Added Gtkrc, Prolog, and Go lexers.
* CSS lexer is more flexible.
* Diff lexer is more accurate.
* Updated TeX lexer.
* Only highlight C/C++ preprocessor words, not the whole line.
* Updated to [Scintilla][]/[SciTE][] 2.20.

[Scintillua 2.20-1]: download/scintillua220-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 2.12-1 (15 Jun 2010)

Download:

* [Scintillua 2.12-1][]

Bugfixes:

* Differentiate between division and regex in Javascript lexer.

Changes:

* Added `enum` keyword to Java lexer.
* Updated D lexer.
* Updated to [Scintilla][]/[SciTE][] 2.12.

[Scintillua 2.12-1]: download/scintillua212-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 2.11-1 (30 Apr 2010)

Download:

* [Scintillua 2.11-1][]

Bugfixes:

* Fixed bug in multi-language lexer detection.
* Close `lua_State` on lexer load error.
* Fixed bug with style metatables.
* Fixed bug with XML namespaces.
* Added Java annotations to Java lexer.

Changes:

* Updated Haskell lexer.
* Added Matlab/Octave lexer.
* Improve speed by using `SCI_GETCHARACTERPOINTER` instead of copying strings.
* Updated D lexer.
* Renamed `lexers/b.lua` to `lexers/b_lang.lua`and `lexers/r.lua` to
  `lexers/rstats.lua`.
* Allow multiple character escape sequences.
* Added Inform lexer.
* Added Lilypond and NSIS lexers.
* Updated LaTeX lexer.
* Updated to [Scintilla][]/[SciTE][] 2.11.

[Scintillua 2.11-1]: download/scintillua211-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 2.03-1 (22 Feb 2010)

Download:

* [Scintillua 2.03-1][]

Bugfixes:

* Various bugfixes.
* Fixed bug with fonts for files open on command line.

Changes:

* Updated to [Scintilla][]/[SciTE][] 2.03.

[Scintillua 2.03-1]: download/scintillua203-1.zip
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 2.02-1 (26 Jan 2010)

Download:

* [Scintillua 2.02-1][]

Bugfixes:

* None.

Changes:

* Renamed `lexers/io.lua` to `lexers/Io.lua`.
* Rearranged tokens in various lexers for speed.
* Allow for [MinGW][] compilation on Windows.
* Call `ruby.LoadStyles()` from RHTML lexer.
* Updated to [Scintilla][]/[SciTE][] 2.02.

[Scintillua 2.02-1]: download/scintillua202-1.zip
[MinGW]: http://mingw.org
[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

## 2.01-1 (13 Jan 2010)

* Initial release for [Scintilla][]/[SciTE][] 2.01.

[Scintilla]: http://scintilla.org
[SciTE]: http://scintilla.org/SciTE.html

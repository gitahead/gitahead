-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Swift LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'swift'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, P('L')^-1 * l.delimited_range('"', true))

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Preprocessor.
local preproc_word = word_match{
  'available', 'column', 'else', 'elseif', 'endif', 'file', 'function', 'if',
  'line', 'selector'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- declarations
  'associatedtype', 'class', 'deinit', 'enum', 'extension', 'func', 'import',
  'init', 'inout', 'internal', 'let', 'operator', 'private', 'protocol',
  'public', 'static', 'struct', 'subscript', 'typealias', 'var',
  -- statements
  'break', 'case', 'continue', 'default', 'defer', 'do', 'else', 'fallthrough',
  'for', 'guard', 'if', 'in', 'repeat', 'return', 'switch', 'where', 'while',
  -- expressions
  'as', 'catch', 'dynamicType', 'false', 'is', 'nil', 'rethrows', 'super',
  'self', 'Self', 'throw', 'throws', 'true', 'try',
  -- context-sensitive
  'associativity', 'convenience', 'dynamic', 'didSet', 'final', 'get', 'infix',
  'indirect', 'lazy', 'left', 'mutating', 'none', 'nonmutating', 'optional',
  'override', 'postfix', 'precedence', 'prefix', 'Protocol', 'required',
  'right', 'set', 'Type', 'unowned', 'weak', 'willSet'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=&|?:;,.()[]{}'))

-- Attributes.
local attribute = token(l.TYPE, '@' * l.word)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
  {'attribute', attribute},
}

M._foldsymbols = {
  _patterns = {'%l+', '[{}]', '/%*', '%*/', '//'},
  [l.PREPROCESSOR] = {
    ['if'] = 1, endif = -1
  },
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M

#!/usr/bin/lua

local format, concat = string.format, table.concat

-- Do not glob these files. (e.g. *.foo)
local noglobs = {
  GNUmakefile = true,
  Makefile = true,
  makefile = true,
  Rakefile = true,
  ['Rout.save'] = true,
  ['Rout.fail'] = true,
}

local alt_name = {
  actionscript = 'flash',
  ansi_c = 'c',
  dmd = 'd',
  javascript = 'js',
  python = 'py',
  rstats = 'r',
  ruby = 'rb',
}

-- Process file patterns and lexer definitions from Textadept.
local f = io.open('../textadept/modules/textadept/file_types.lua')
local definitions = f:read('*all'):match('M%.extensions = (%b{})')
f:close()

local output = {'# Lexer definitions ('}
local lexer, ext, last_lexer
local exts = {}
for ext, lexer in definitions:gmatch("([^,'%]]+)'?%]?='([%w_]+)'") do
  if lexer ~= last_lexer and #exts > 0 then
    local name = alt_name[last_lexer] or last_lexer
    output[#output + 1] = format('file.patterns.%s=%s', name,
                                 concat(exts, ';'))
    output[#output + 1] = format('lexer.$(file.patterns.%s)=lpeg_%s', name,
                                 last_lexer)
    exts = {}
  end
  exts[#exts + 1] = not noglobs[ext] and '*.'..ext or ext
  last_lexer = lexer
end
local name = alt_name[last_lexer] or last_lexer
output[#output + 1] = format('file.patterns.%s=%s', name, concat(exts, ';'))
output[#output + 1] = format('lexer.$(file.patterns.%s)=lpeg_%s', name,
                             last_lexer)
output[#output + 1] = '# )'

-- Write to lpeg.properties.
f = io.open('lexers/lpeg.properties')
local text = f:read('*all')
text = text:gsub('# Lexer definitions %b()', table.concat(output, '\n'), 1)
f:close()
f = io.open('lexers/lpeg.properties', 'wb')
f:write(text)
f:close()

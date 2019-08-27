-- Copyright 2017-2018 Mitchell mitchell.att.foicica.com. See License.txt.
-- Unit tests for Lua LPeg lexers, but without using the Scintilla lexer.

package.path = '../lexlua/?.lua;'..package.path

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local lpeg = require('lpeg')
-- The Scintilla LPeg lexer normally defines these.
lexer.FOLD_BASE, lexer.FOLD_HEADER, lexer.FOLD_BLANK = 0x400, 0x2000, 0x1000

-- Helper assert functions.

-- Asserts the given lexer contains the default LPeg lexer and Scintilla styles,
-- and that those styles are correctly numbered. LPeg lexer style numbers start
-- at 0 while Scintilla styles start at 32.
-- Note: the style tables used are copied from lexer.lua since they are local to
-- that file.
-- @param lex The lexer to style-check.
function assert_default_styles(lex)
  local default_styles = {
    'nothing', 'whitespace', 'comment', 'string', 'number', 'keyword',
    'identifier', 'operator', 'error', 'preprocessor', 'constant', 'variable',
    'function', 'class', 'type', 'label', 'regex', 'embedded'
  }
  for i = 1, #default_styles do
    local style = default_styles[i]
    assert(lex._TOKENSTYLES[style],
           string.format("style '%s' does not exist", style))
    assert(lex._TOKENSTYLES[style] == i - 1, 'default styles out of order')
  end
  local predefined_styles = {
    'default', 'linenumber', 'bracelight', 'bracebad', 'controlchar',
    'indentguide', 'calltip', 'folddisplaytext'
  }
  for i = 1, #predefined_styles do
    local style = predefined_styles[i]
    assert(lex._TOKENSTYLES[style],
           string.format("style '%s' does not exist", style))
    assert(lex._TOKENSTYLES[style] == i + 31, 'predefined styles out of order')
  end
end

-- Asserts the given lexer has the given style assigned to the given style name.
-- @param lex The lexer to style-check.
-- @param style_name The name of the style to check for.
-- @param style The style's expected Scintilla style string.
function assert_style(lex, style_name, style)
  assert(lex._TOKENSTYLES[style_name],
         string.format("style '%s' does not exist", style_name))
  assert(lex._EXTRASTYLES[style_name] == style,
         string.format("'%s' ~= '%s'", lex._EXTRASTYLES[style_name], style))
end

-- Asserts the given lexer contains the given ordered list of rules.
-- @param lex The lexer to rule-check.
-- @param rules The ordered list of rule names the lexer should have.
function assert_rules(lex, rules)
  local j = 1
  for i = 1, #lex._RULEORDER do
    assert(lex._RULES[rules[j]],
           string.format("rule '%s' does not exist", rules[j]))
    assert(lex._RULEORDER[i] == rules[j],
           string.format("'%s' ~= '%s'", lex._RULEORDER[i], rules[i] or ''))
    j = j + 1
  end
  if #lex._RULEORDER ~= #rules then
    error(string.format("'%s' rule not found", rules[j]))
  end
end

-- Asserts the given lexer contains the given set of extra styles in addition to
-- its defaults.
-- @param lex The lexer to style-check.
-- @param styles The list of extra style names the lexer should have.
function assert_extra_styles(lex, styles)
  for i = 1, #styles do
    assert(lex._TOKENSTYLES[styles[i]],
           string.format("'%s' not found", styles[i]))
    assert(lex._EXTRASTYLES[styles[i]],
           string.format("'%s' not found", styles[i]))
  end
end

-- Asserts the given lexer contains the given set of child lexer names.
-- @param lex The lexer to child-check.
-- @param children The list of child lexer names the lexer should have.
function assert_children(lex, children)
  local j = 1
  for i = 1, #lex._CHILDREN do
    assert(lex._CHILDREN[i]._NAME == children[j],
           string.format("'%s' ~= '%s'", lex._CHILDREN[i]._NAME,
                         children[j] or ''))
    j = j + 1
  end
  if #lex._CHILDREN ~= #children then
    error(string.format("child '%s' not found", children[j]))
  end
end

-- Asserts the given lexer produces the given tokens after lexing the given
-- code.
-- @param lex The lexer to use.
-- @param code The string code to lex.
-- @param expected_tokens The list of expected tokens from the lexer. Each token
--   is a table that contains the token's name followed by the substring of code
--   matched. Whitespace tokens are ignored for the sake of simplicity. Do not
--   include them.
-- @param initial_style Optional current style. This is used for determining
--   which language to start in in a multiple-language lexer.
-- @usage assert_lex(lua, "print('hi')", {{'function', 'print'},
--   {'operator', '('}, {'string', "'hi'"}, {'operator', ')'}})
function assert_lex(lex, code, expected_tokens, initial_style)
  if lex._lexer then lex = lex._lexer end -- note: lexer.load() does this
  local tokens = lex:lex(code, initial_style or
                               lex._TOKENSTYLES[lex._NAME..'_whitespace'])
  local j = 1
  for i = 1, #tokens, 2 do
    if not tokens[i]:find('whitespace$') then
      local token = tokens[i]
      local text = code:sub(tokens[i - 1] or 0, tokens[i + 1] - 1)
      assert(token == expected_tokens[j][1] and text == expected_tokens[j][2],
             string.format("('%s', '%s') ~= ('%s', '%s')", token, text,
                           expected_tokens[j][1], expected_tokens[j][2]))
      j = j + 1
    end
  end
  if j - 1 ~= #expected_tokens then
    error(string.format("('%s', '%s') not found", expected_tokens[j][1],
                        expected_tokens[j][2]))
  end
end

-- Asserts the given lexer produces the given fold points after lexing the
-- given code.
-- @param lex The lexer to use.
-- @param code The string code to fold.
-- @param expected_fold_points The list of expected fold points from the lexer.
--   Each fold point is just a line number, starting from 1.
-- @param initial_style Optional current style. This is used for determining
--   which language to start in in a multiple-language lexer.
-- @return fold levels for any further analysis
-- @usage assert_fold_points(lua, "if foo then\n  bar\nend", {1})
function assert_fold_points(lex, code, expected_fold_points, initial_style)
  if lex._lexer then lex = lex._lexer end -- note: lexer.load() does this
  -- Since `M.style_at()` is provided by Scintilla and not available for tests,
  -- create it, using data from `lexer.lex()`.
  local tokens = lex:lex(code, initial_style or
                               lex._TOKENSTYLES[lex._NAME..'_whitespace'])
  lexer.style_at = setmetatable({}, {__index = function(self, pos)
    for i = 2, #tokens, 2 do
      if pos < tokens[i] then return tokens[i - 1] end
    end
  end})
  if not lexer.property then -- Scintilla normally creates this
    lexer.property, lexer.property_int = {}, setmetatable({}, {
      __index = function(t, k) return tonumber(lexer.property[k]) or 0 end,
      __newindex = function() error('read-only property') end
    })
  end
  lexer.property['fold'] = 1
  local levels = lex:fold(code, 0, 1, lexer.FOLD_BASE)
  local j = 1
  for i = 1, #levels do
    if i == expected_fold_points[j] then
      assert(levels[i] >= lexer.FOLD_HEADER,
             string.format("line %i not a fold point", i))
      j = j + 1
    else
      assert(levels[i] <= lexer.FOLD_HEADER,
             string.format("line %i is a fold point", i))
    end
  end
  assert(j - 1 == #expected_fold_points,
         string.format("line %i is not a fold point", j))
  return levels
end

-- Unit tests.

-- Tests a basic lexer with a few simple rules and no custom styles.
function test_basics()
  local lex = lexer.new('test')
  assert_default_styles(lex)
  lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[foo bar baz]]))
  lex:add_rule('string', token(lexer.STRING, lexer.delimited_range('"')))
  lex:add_rule('number', token(lexer.NUMBER, lexer.integer))
  local code = [[foo bar baz "foo bar baz" 123]]
  local tokens = {
    {lexer.KEYWORD, 'foo'},
    {lexer.KEYWORD, 'bar'},
    {lexer.KEYWORD, 'baz'},
    {lexer.STRING, '"foo bar baz"'},
    {lexer.NUMBER, '123'}
  }
  assert_lex(lex, code, tokens)
end

-- Tests that lexer rules are added in an ordered sequence and that
-- modifying rules in place works as expected.
function test_rule_order()
  local lex = lexer.new('test')
  lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))
  lex:add_rule('keyword', token(lexer.KEYWORD, lpeg.P('foo')))
  local code = [[foo bar]]
  local tokens = {
    {lexer.IDENTIFIER, 'foo'},
    {lexer.IDENTIFIER, 'bar'}
  }
  assert_lex(lex, code, tokens)

  -- Modify the identifier rule to not catch keywords.
  lex:modify_rule('identifier', token(lexer.IDENTIFIER,
                                      -lpeg.P('foo') * lexer.word))
  tokens = {
    {lexer.KEYWORD, 'foo'},
    {lexer.IDENTIFIER, 'bar'},
  }
  assert_lex(lex, code, tokens)
end

-- Tests a basic lexer with a couple of simple rules and a custom style.
function test_add_style()
  local lex = lexer.new('test')
  assert_default_styles(lex)
  lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  lex:add_rule('keyword', token('custom', word_match[[foo bar baz]]))
  lex:add_style('custom', lexer.STYLE_KEYWORD)
  assert_default_styles(lex)
  assert_style(lex, 'custom', lexer.STYLE_KEYWORD)
  local code = [[foo bar baz]]
  local tokens = {
    {'custom', 'foo'},
    {'custom', 'bar'},
    {'custom', 'baz'}
  }
  assert_lex(lex, code, tokens)
end

-- Tests a simple parent lexer embedding a simple child lexer.
-- Ensures the child's custom styles are also copied over.
function test_embed()
  -- Create the parent lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local parent = lexer.new('parent')
  assert_default_styles(parent)
  lexer.WHITESPACE = parent._NAME..'_whitespace'
  parent:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  assert_style(parent, parent._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  parent:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  parent:add_rule('identifier', token('parent', lexer.word))
  parent:add_style('parent', lexer.STYLE_IDENTIFIER)
  assert_style(parent, 'parent', lexer.STYLE_IDENTIFIER)

  -- Create the child lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local child = lexer.new('child')
  assert_default_styles(child)
  lexer.WHITESPACE = child._NAME..'_whitespace'
  child:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  assert_style(child, child._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  child:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  child:add_rule('number', token('child', lexer.integer))
  child:add_style('child', lexer.STYLE_NUMBER)
  assert_style(child, 'child', lexer.STYLE_NUMBER)

  -- Assert the child's styles are not embedded in the parent yet.
  assert(not parent._TOKENSTYLES[child._NAME..'_whitespace'])
  assert(not parent._EXTRASTYLES[child._NAME..'_whitespace'])
  assert(not parent._TOKENSTYLES['child'])
  assert(not parent._EXTRASTYLES['child'])

  -- Embed the child into the parent and verify the child's styles were copied
  -- over.
  local start_rule = token('transition', lpeg.P('['))
  local end_rule = token('transition', lpeg.P(']'))
  parent:embed(child, start_rule, end_rule)
  parent:add_style('transition', lexer.STYLE_EMBEDDED)
  assert_default_styles(parent)
  assert_style(parent, parent._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  assert_style(parent, 'parent', lexer.STYLE_IDENTIFIER)
  assert_style(parent, 'transition', lexer.STYLE_EMBEDDED)
  assert_style(parent, child._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  assert_style(parent, 'child', lexer.STYLE_NUMBER)

  -- Lex some parent -> child -> parent code.
  local code = [[foo [1, 2, 3] bar]]
  local tokens = {
    {'parent', 'foo'},
    {'transition', '['},
    {'child', '1'},
    {lexer.DEFAULT, ','},
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  assert_lex(parent, code, tokens)

  -- Lex some child -> parent code, starting from within the child.
  code = [[2, 3] bar]]
  tokens = {
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  local initial_style = parent._TOKENSTYLES[child._NAME..'_whitespace']
  assert_lex(parent, code, tokens, initial_style)
end

-- Tests a simple child lexer embedding itself within a simple parent lexer.
-- Ensures the child's custom styles are also copied over.
function test_embed_into()
  -- Create the child lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local child = lexer.new('child')
  lexer.WHITESPACE = child._NAME..'_whitespace'
  child:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  child:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  child:add_rule('number', token('child', lexer.integer))
  child:add_style('child', lexer.STYLE_NUMBER)

  -- Create the parent lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local parent = lexer.new('parent')
  lexer.WHITESPACE = parent._NAME..'_whitespace'
  parent:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  parent:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  parent:add_rule('identifier', token('parent', lexer.word))
  parent:add_style('parent', lexer.STYLE_IDENTIFIER)

  -- Embed the child within the parent and verify the child's custom styles were
  -- copied over.
  local start_rule = token('transition', lpeg.P('['))
  local end_rule = token('transition', lpeg.P(']'))
  parent:embed(child, start_rule, end_rule)
  parent:add_style('transition', lexer.STYLE_EMBEDDED)
  assert_default_styles(parent)
  assert_style(parent, parent._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  assert_style(parent, 'parent', lexer.STYLE_IDENTIFIER)
  assert_style(parent, 'transition', lexer.STYLE_EMBEDDED)
  assert_style(parent, child._NAME..'_whitespace', lexer.STYLE_WHITESPACE)
  assert_style(parent, 'child', lexer.STYLE_NUMBER)

  -- Verify any subsequent style additions to the child are copied to the
  -- parent.
  child:add_style('extra_style', lexer.STYLE_COMMENT)
  assert_style(parent, 'extra_style', lexer.STYLE_COMMENT)

  -- Verify any subsequent fold point additions to the child are copied to the
  -- parent.
  child:add_fold_point('transition', '[', ']')
  assert(parent._FOLDPOINTS['transition']['['] == 1)
  assert(parent._FOLDPOINTS['transition'][']'] == -1)

  -- Lex some parent -> child -> parent code.
  local code = [[foo [1, 2, 3] bar]]
  local tokens = {
    {'parent', 'foo'},
    {'transition', '['},
    {'child', '1'},
    {lexer.DEFAULT, ','},
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  assert_lex(child, code, tokens)

  -- Lex some child -> parent code, starting from within the child.
  code = [[2, 3] bar]]
  tokens = {
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  local initial_style = parent._TOKENSTYLES[child._NAME..'_whitespace']
  assert_lex(child, code, tokens, initial_style)

  -- Fold some code.
  code = [[
    foo [
      1, 2, 3
    ] bar
    baz
  ]]
  local folds = {1}
  local levels = assert_fold_points(child, code, folds)
  assert(levels[3] > levels[4]) -- verify ']' is fold end point
end

-- Tests a proxy lexer that inherits from a simple parent lexer and embeds a
-- simple child lexer.
-- Ensures both the proxy's and child's custom styles are also copied over.
function test_proxy()
  -- Create the parent lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local parent = lexer.new('parent')
  lexer.WHITESPACE = parent._NAME..'_whitespace'
  parent:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  parent:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  parent:add_rule('identifier', token('parent', lexer.word))
  parent:add_style('parent', lexer.STYLE_IDENTIFIER)

  -- Create the child lexer.
  -- Note: lexer.load() sets lexer.WHITESPACE and adds the custom whitespace
  -- style.
  local child = lexer.new('child')
  lexer.WHITESPACE = child._NAME..'_whitespace'
  child:add_style(lexer.WHITESPACE, lexer.STYLE_WHITESPACE)
  child:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  child:add_rule('number', token('child', lexer.integer))
  child:add_style('child', lexer.STYLE_NUMBER)

  -- Create the proxy lexer.
  local proxy = lexer.new('proxy', {inherit = parent})

  -- Embed the child into the parent and verify the proxy's custom style was
  -- copied over.
  local start_rule = token('transition', lpeg.P('['))
  local end_rule = token('transition', lpeg.P(']'))
  proxy:embed(child, start_rule, end_rule)
  proxy:add_style('transition', lexer.STYLE_EMBEDDED)
  assert_style(parent, 'transition', lexer.STYLE_EMBEDDED)

  -- Verify any subsequent style additions to the proxy are copied to the
  -- parent.
  proxy:add_style('extra_style', lexer.STYLE_COMMENT)
  assert_style(parent, 'extra_style', lexer.STYLE_COMMENT)

  -- Lex some parent -> child -> parent code.
  local code = [[foo [1, 2, 3] bar]]
  local tokens = {
    {'parent', 'foo'},
    {'transition', '['},
    {'child', '1'},
    {lexer.DEFAULT, ','},
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  assert_lex(proxy, code, tokens)

  -- Lex some child -> parent code, starting from within the child.
  code = [[ 2, 3] bar]]
  tokens = {
    {'child', '2'},
    {lexer.DEFAULT, ','},
    {'child', '3'},
    {'transition', ']'},
    {'parent', 'bar'}
  }
  local initial_style = parent._TOKENSTYLES[child._NAME..'_whitespace']
  assert_lex(proxy, code, tokens, initial_style)

  -- Verify any subsequent fold point additions to the proxy are copied to
  -- the parent.
  proxy:add_fold_point('transition', '[', ']')
  assert(parent._FOLDPOINTS['transition']['['] == 1)
  assert(parent._FOLDPOINTS['transition'][']'] == -1)

  -- Fold some code.
  code = [[
    foo [
      1, 2, 3
    ] bar
    baz
  ]]
  local folds = {1}
  local levels = assert_fold_points(proxy, code, folds)
  assert(levels[3] > levels[4]) -- verify ']' is fold end point
end

-- Tests a lexer that inherits from another one.
function test_inherits_rules()
  local lex = lexer.new('test')
  lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
  lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[foo bar baz]]))

  -- Verify inherited rules are used.
  local sublexer = lexer.new('test2', {inherit = lex})
  local code = [[foo bar baz]]
  local tokens = {
    {lexer.KEYWORD, 'foo'},
    {lexer.KEYWORD, 'bar'},
    {lexer.KEYWORD, 'baz'}
  }
  assert_lex(sublexer, code, tokens)

  -- Verify subsequently added rules are also used.
  sublexer:add_rule('keyword2', token(lexer.KEYWORD, lpeg.P('quux')))
  code = [[foo bar baz quux]]
  tokens = {
    {lexer.KEYWORD, 'foo'},
    {lexer.KEYWORD, 'bar'},
    {lexer.KEYWORD, 'baz'},
    {lexer.KEYWORD, 'quux'}
  }
  assert_lex(sublexer, code, tokens)
end

-- Tests that fold words are folded properly, even if fold words are substrings
-- of others (e.g. "if" and "endif").
function test_fold_words()
  local lex = lexer.new('test')
  lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[if endif]]))
  lex:add_fold_point(lexer.KEYWORD, 'if', 'endif')

  local code = [[
    if foo
      bar
    endif
    ifbaz
    quuxif
  ]]
  local folds = {1}
  local levels = assert_fold_points(lex, code, folds)
  assert(levels[2] == lexer.FOLD_BASE + 1)
  assert(levels[3] == lexer.FOLD_BASE + 1)
  assert(levels[4] == lexer.FOLD_BASE)
end

-- Tests folding by indentation.
function test_fold_by_indentation()
  local lex = lexer.new('test', {fold_by_indentation = true})
  local code = [[
    if foo:
      bar
    else:
      baz
  ]]
  lexer.fold_level = {[0] = lexer.FOLD_BASE} -- Scintilla normally creates this
  lexer.indent_amount = {[0] = 0} -- Scintilla normally creates this
  local folds = {1, 3}
  assert_fold_points(lex, code, folds)
end

function test_legacy()
  local lex = {_NAME = 'test'}
  lex._rules = {
    {'whitespace', token(lexer.WHITESPACE, lexer.space^1)},
    {'keyword', token(lexer.KEYWORD, word_match{'foo', 'bar', 'baz'})},
    {'custom', token('custom', lpeg.P('quux'))}
  }
  lex._tokenstyles = {custom = lexer.STYLE_CONSTANT}
  lex._foldsymbols = {
    _patterns = {'%l+'},
    [lexer.KEYWORD] = {foo = 1, baz = -1}
  }
  -- The following comes from `process_legacy_lexer()`.
  local default = {
    'nothing', 'whitespace', 'comment', 'string', 'number', 'keyword',
    'identifier', 'operator', 'error', 'preprocessor', 'constant', 'variable',
    'function', 'class', 'type', 'label', 'regex', 'embedded'
  }
  local predefined = {
    'default', 'linenumber', 'bracelight', 'bracebad', 'controlchar',
    'indentguide', 'calltip', 'folddisplaytext'
  }
  local token_styles = {}
  for i = 1, #default do token_styles[default[i]] = i - 1 end
  for i = 1, #predefined do token_styles[predefined[i]] = i + 31 end
  lex._TOKENSTYLES, lex._numstyles = token_styles, #default
  lex._EXTRASTYLES = {}
  assert_default_styles(lex)
  setmetatable(lex, getmetatable(lexer.new('')))
  for i = 1, #lex._rules do lex:add_rule(lex._rules[i][1], lex._rules[i][2]) end

  local code = [[
    foo
      bar
    baz
    quux
  ]]
  local tokens = {
    {'keyword', 'foo'},
    {'keyword', 'bar'},
    {'keyword', 'baz'},
    {'custom', 'quux'}
  }
  assert_lex(lex, code, tokens)
end

-- Tests that all lexers load and lex text.
function test_loads()
  local p = io.popen('ls -1 ../lexlua/*.lua')
  local files = p:read('*a')
  p:close()
  for file in files:gmatch('[^\n]+') do
    local lex_name = file:match('^%.%./lexlua/(.+)%.lua$')
    if lex_name ~= 'lexer' then
      local lex = lexer.load(lex_name, nil, true)
      assert_default_styles(lex)
      local tokens = lex:lex('test')
      assert(#tokens >= 2)
    end
  end
end

-- Tests the Lua lexer.
function test_lua()
  local lua = lexer.load('lua')
  assert(lua._NAME == 'lua')
  assert_default_styles(lua)
  local rules = {
    'whitespace', 'keyword', 'function', 'constant', 'library', 'identifier',
    'string', 'comment', 'number', 'label', 'operator'
  }
  assert_rules(lua, rules)
  local styles = {
    'deprecated_function', 'library', 'deprecated_library', 'longstring',
    'lua_whitespace' -- language-specific whitespace for multilang lexers
  }
  assert_extra_styles(lua, styles)

  -- Lexing tests.
  local code = [=[
    -- Comment.
    ::begin::
    local a = 1 + 2.0e3 - 0x40
    local b = "two"..[[three]]
    _G.print(a, string.upper(b))
  ]=]
  local tokens = {
    {lexer.COMMENT, '-- Comment.'},
    {lexer.LABEL, '::begin::'},
    {lexer.KEYWORD, 'local'},
    {lexer.IDENTIFIER, 'a'},
    {lexer.OPERATOR, '='},
    {lexer.NUMBER, '1'},
    {lexer.OPERATOR, '+'},
    {lexer.NUMBER, '2.0e3'},
    {lexer.OPERATOR, '-'},
    {lexer.NUMBER, '0x40'},
    {lexer.KEYWORD, 'local'},
    {lexer.IDENTIFIER, 'b'},
    {lexer.OPERATOR, '='},
    {lexer.STRING, '"two"'},
    {lexer.OPERATOR, '..'},
    {'longstring', '[[three]]'},
    {lexer.CONSTANT, '_G'},
    {lexer.OPERATOR, '.'},
    {lexer.FUNCTION, 'print'},
    {lexer.OPERATOR, '('},
    {lexer.IDENTIFIER, 'a'},
    {lexer.OPERATOR, ','},
    {'library', 'string.upper'},
    {lexer.OPERATOR, '('},
    {lexer.IDENTIFIER, 'b'},
    {lexer.OPERATOR, ')'},
    {lexer.OPERATOR, ')'}
  }
  assert_lex(lua, code, tokens)

  -- Folding tests.
  code = [=[
    if foo then
      bar
    end
    for k, v in pairs(foo) do
      bar
    end
    function foo(bar)
      baz
    end
    repeat
      foo
    until bar
    --[[
      foo
    ]]
    (foo,
     bar,
     baz)
    {foo,
     bar,
     baz}
  ]=]
  local folds = {1, 4, 7, 10, 13, 16, 19}
  assert_fold_points(lua, code, folds)
end

-- Tests the C lexer.
function test_c()
  local c = lexer.load('ansi_c')
  assert(c._NAME == 'ansi_c')
  assert_default_styles(c)

  -- Lexing tests.
  local code = ([[
    /* Comment. */
    #include <stdlib.h>
    #include "lua.h"
    int main(int argc, char **argv) {
      if (NULL);
      return 0;
    }
  ]]):gsub('    ', '') -- strip indent
  local tokens = {
    {lexer.COMMENT, '/* Comment. */'},
    {lexer.PREPROCESSOR, '#include'},
    {lexer.STRING, '<stdlib.h>'},
    {lexer.PREPROCESSOR, '#include'},
    {lexer.STRING, '"lua.h"'},
    {lexer.TYPE, 'int'},
    {lexer.IDENTIFIER, 'main'},
    {lexer.OPERATOR, '('},
    {lexer.TYPE, 'int'},
    {lexer.IDENTIFIER, 'argc'},
    {lexer.OPERATOR, ','},
    {lexer.TYPE, 'char'},
    {lexer.OPERATOR, '*'},
    {lexer.OPERATOR, '*'},
    {lexer.IDENTIFIER, 'argv'},
    {lexer.OPERATOR, ')'},
    {lexer.OPERATOR, '{'},
    {lexer.KEYWORD, 'if'},
    {lexer.OPERATOR, '('},
    {lexer.CONSTANT, 'NULL'},
    {lexer.OPERATOR, ')'},
    {lexer.OPERATOR, ';'},
    {lexer.KEYWORD, 'return'},
    {lexer.NUMBER, '0'},
    {lexer.OPERATOR, ';'},
    {lexer.OPERATOR, '}'}
  }
  assert_lex(c, code, tokens)

  -- Folding tests.
  code = ([[
    if (foo) {
      bar;
    }
    /**
     * foo
     */
    #ifdef foo
      bar;
    #endif
  ]]):gsub('    ', '') -- strip indent
  local folds = {1, 4, 7}
  assert_fold_points(c, code, folds)
end

-- Tests the HTML lexer and its embedded languages.
function test_html()
  local html = lexer.load('html')
  assert(html._NAME == 'html')
  assert_default_styles(html)
  local rules = {
    'whitespace', 'comment', 'doctype', 'element', 'tag_close', 'attribute',
    --[['equals',]] 'string', 'number', 'entity'
  }
  assert_rules(html, rules)
  local styles = {
    'doctype', 'element', 'unknown_element', 'attribute', 'unknown_attribute',
    'entity', 'html_whitespace',
    'value', 'color', 'unit', 'at_rule', 'css_whitespace', -- CSS
    'javascript_whitespace', -- JS
    'coffeescript_whitespace' -- CoffeeScript
  }
  assert_extra_styles(html, styles)
  assert_children(html, {'css', 'javascript', 'coffeescript'})

  -- Lexing tests.
  local code = [[
    <!DOCTYPE html>
    <!-- Comment. -->
    <html>
      <head>
        <style type="text/css">
          /* Another comment. */
          h1:hover {
            color: red;
            border: 1px solid #0000FF;
          }
        </style>
        <script type="text/javascript">
          /* A third comment. */
          var a = 1 + 2.0e3 - 0x40;
          var b = "two" + `three`;
          var c = /pattern/i;
        //</script>
      </head>
      <bod/>
    </html>
  ]]
  local tokens = {
    {'doctype', '<!DOCTYPE html>'},
    {lexer.COMMENT, '<!-- Comment. -->'},
    {'element', '<html'},
    {'element', '>'},
    {'element', '<head'},
    {'element', '>'},
    {'element', '<style'},
    {'attribute', 'type'},
    {lexer.OPERATOR, '='},
    {lexer.STRING, '"text/css"'},
    {'element', '>'},
    {lexer.COMMENT, '/* Another comment. */'},
    {lexer.IDENTIFIER, 'h1'},
    {'pseudoclass', ':hover'},
    {lexer.OPERATOR, '{'},
    {'property', 'color'},
    {lexer.OPERATOR, ':'},
    {'value', 'red'},
    {lexer.OPERATOR, ';'},
    {'property', 'border'},
    {lexer.OPERATOR, ':'},
    {lexer.NUMBER, '1'},
    {'unit', 'px'},
    {'value', 'solid'},
    {'color', '#0000FF'},
    {lexer.OPERATOR, ';'},
    {lexer.OPERATOR, '}'},
    {'element', '</style'},
    {'element', '>'},
    {'element', '<script'},
    {'attribute', 'type'},
    {lexer.OPERATOR, '='},
    {lexer.STRING, '"text/javascript"'},
    {'element', '>'},
    {lexer.COMMENT, '/* A third comment. */'},
    {lexer.KEYWORD, 'var'},
    {lexer.IDENTIFIER, 'a'},
    {lexer.OPERATOR, '='},
    {lexer.NUMBER, '1'},
    {lexer.OPERATOR, '+'},
    {lexer.NUMBER, '2.0e3'},
    {lexer.OPERATOR, '-'},
    {lexer.NUMBER, '0x40'},
    {lexer.OPERATOR, ';'},
    {lexer.KEYWORD, 'var'},
    {lexer.IDENTIFIER, 'b'},
    {lexer.OPERATOR, '='},
    {lexer.STRING, '"two"'},
    {lexer.OPERATOR, '+'},
    {lexer.STRING, '`three`'},
    {lexer.OPERATOR, ';'},
    {lexer.KEYWORD, 'var'},
    {lexer.IDENTIFIER, 'c'},
    {lexer.OPERATOR, '='},
    {lexer.REGEX, '/pattern/i'},
    {lexer.OPERATOR, ';'},
    {lexer.COMMENT, '//'},
    {'element', '</script'},
    {'element', '>'},
    {'element', '</head'},
    {'element', '>'},
    {'unknown_element', '<bod'},
    {'element', '/>'},
    {'element', '</html'},
    {'element', '>'}
  }
  assert_lex(html, code, tokens)

  -- Folding tests.
  local symbols = {'<', '<!--', '-->', '{', '}', '/*', '*/', '//'}
  for i = 1, #symbols do assert(html._FOLDPOINTS._SYMBOLS[symbols[i]]) end
  assert(html._FOLDPOINTS['element']['<'])
  assert(html._FOLDPOINTS['unknown_element']['<'])
  assert(html._FOLDPOINTS[lexer.COMMENT]['<!--'])
  assert(html._FOLDPOINTS[lexer.COMMENT]['-->'])
  assert(html._FOLDPOINTS[lexer.OPERATOR]['{'])
  assert(html._FOLDPOINTS[lexer.OPERATOR]['}'])
  assert(html._FOLDPOINTS[lexer.COMMENT]['/*'])
  assert(html._FOLDPOINTS[lexer.COMMENT]['*/'])
  assert(html._FOLDPOINTS[lexer.COMMENT]['//'])
  code = [[
    <html>
      foo
    </html>
    <body/>
    <style type="text/css">
      h1 {
        foo;
      }
    </style>
    <script type="text/javascript">
      function foo() {
        bar;
      }
    </script>
    h1 {
      foo;
    }
    function foo() {
      bar;
    }
  ]]
  local folds = {1, 5, 6, 10, 11}
  local levels = assert_fold_points(html, code, folds)
  assert(levels[3] > levels[4]) -- </html> is ending fold point
end

-- Tests the PHP lexer.
function test_php()
  local php = lexer.load('php')
  assert(php._NAME == 'php')
  assert_default_styles(php)
  assert_extra_styles(php, {'php_whitespace', 'php_tag'})

  -- Lexing tests
  -- Starting in HTML.
  local code = [[<h1><?php echo "hi"; ?></h1>]]
  local tokens = {
    {'element', '<h1'},
    {'element', '>'},
    {'php_tag', '<?php '},
    {lexer.KEYWORD, 'echo'},
    {lexer.STRING, '"hi"'},
    {lexer.OPERATOR, ';'},
    {'php_tag', '?>'},
    {'element', '</h1'},
    {'element', '>'}
  }
  local initial_style = php._TOKENSTYLES['html_whitespace']
  assert_lex(php, code, tokens, initial_style)
  -- Starting in PHP.
  code = [[echo "hi";]]
  initial_style = php._TOKENSTYLES['php_whitespace']
  tokens = {
    {lexer.KEYWORD, 'echo'},
    {lexer.STRING, '"hi"'},
    {lexer.OPERATOR, ';'},
  }
  assert_lex(php, code, tokens, initial_style)

  -- Folding tests.
  local symbols = {'<?', '?>', '/*', '*/', '//', '#', '{', '}', '(', ')'}
  for i = 1, #symbols do assert(php._FOLDPOINTS._SYMBOLS[symbols[i]]) end
  assert(php._FOLDPOINTS['php_tag']['<?'])
  assert(php._FOLDPOINTS['php_tag']['?>'])
  assert(php._FOLDPOINTS[lexer.COMMENT]['/*'])
  assert(php._FOLDPOINTS[lexer.COMMENT]['*/'])
  assert(php._FOLDPOINTS[lexer.COMMENT]['//'])
  assert(php._FOLDPOINTS[lexer.COMMENT]['#'])
  assert(php._FOLDPOINTS[lexer.OPERATOR]['{'])
  assert(php._FOLDPOINTS[lexer.OPERATOR]['}'])
  assert(php._FOLDPOINTS[lexer.OPERATOR]['('])
  assert(php._FOLDPOINTS[lexer.OPERATOR][')'])
end

-- Tests the Ruby lexer.
function test_ruby()
  local ruby = lexer.load('ruby')

  -- Lexing tests.
  local code = [[
    # Comment.
    require "foo"
    $a = 1 + 2.0e3 - 0x40 if true
    b = "two" + %q[three]
    puts :c
  ]]
  local tokens = {
    {lexer.COMMENT, '# Comment.'},
    {lexer.FUNCTION, 'require'},
    {lexer.STRING, '"foo"'},
    {lexer.VARIABLE, '$a'},
    {lexer.OPERATOR, '='},
    {lexer.NUMBER, '1'},
    {lexer.OPERATOR, '+'},
    {lexer.NUMBER, '2.0e3'},
    {lexer.OPERATOR, '-'},
    {lexer.NUMBER, '0x40'},
    {lexer.KEYWORD, 'if'},
    {lexer.KEYWORD, 'true'},
    {lexer.IDENTIFIER, 'b'},
    {lexer.OPERATOR, '='},
    {lexer.STRING, '"two"'},
    {lexer.OPERATOR, '+'},
    {lexer.STRING, '%q[three]'},
    {lexer.FUNCTION, 'puts'},
    {'symbol', ':c'}
  }
  assert_lex(ruby, code, tokens)

  -- Folding tests.
  local fold_keywords = {
    begin = 1, class = 1, def = 1, ['do'] = 1, ['for'] = 1, ['module'] = 1,
    case = 1, ['if'] = function() end, ['while'] = function() end,
    ['unless'] = function() end, ['until'] = function() end, ['end'] = -1
  }
  for k, v in pairs(fold_keywords) do
    assert(ruby._FOLDPOINTS._SYMBOLS[k])
    if type(v) == 'number' then
      assert(ruby._FOLDPOINTS[lexer.KEYWORD][k] == v)
    else
      assert(type(ruby._FOLDPOINTS[lexer.KEYWORD][k]) == 'function')
    end
  end
  local fold_operators = { '(', ')', '[', ']', '{', '}'}
  for i = 1, #fold_operators do
    assert(ruby._FOLDPOINTS._SYMBOLS[fold_operators[i]])
    assert(ruby._FOLDPOINTS[lexer.OPERATOR][fold_operators[i]])
  end
  code = [=[
    class Foo
      bar
    end
    foo.each do |v|
      bar
    end
    def foo(bar)
      baz
    end
    =begin
      foo
    =end
    (foo,
     bar,
     baz)
    [foo,
     bar,
     baz]
    {foo,
     bar,
     baz}
  ]=]
  local folds = {1, 4, 7, 10, 13, 16, 19}
  assert_fold_points(ruby, code, folds)
end

-- Tests the Ruby and Rails lexers and tests lexer caching and lack of caching.
-- The Rails lexer inherits from Ruby and modifies some of its rules. Verify
-- the Ruby lexer is unaffected.
function test_ruby_and_rails()
  local ruby = lexer.load('ruby', nil, true)
  local rails = lexer.load('rails', nil, true)
  local code = [[
    class Foo < ActiveRecord::Base
      has_one :bar
    end
  ]]
  local ruby_tokens = {
    {lexer.KEYWORD, 'class'},
    {lexer.IDENTIFIER, 'Foo'},
    {lexer.OPERATOR, '<'},
    {lexer.IDENTIFIER, 'ActiveRecord'},
    {lexer.OPERATOR, ':'},
    {lexer.OPERATOR, ':'},
    {lexer.IDENTIFIER, 'Base'},
    {lexer.IDENTIFIER, 'has_one'},
    {'symbol', ':bar'},
    {lexer.KEYWORD, 'end'}
  }
  assert_lex(ruby, code, ruby_tokens)

  local rails_tokens = {
    {lexer.KEYWORD, 'class'},
    {lexer.IDENTIFIER, 'Foo'},
    {lexer.OPERATOR, '<'},
    {lexer.IDENTIFIER, 'ActiveRecord'},
    {lexer.OPERATOR, ':'},
    {lexer.OPERATOR, ':'},
    {lexer.IDENTIFIER, 'Base'},
    {lexer.FUNCTION, 'has_one'},
    {'symbol', ':bar'},
    {lexer.KEYWORD, 'end'}
  }
  assert_lex(rails, code, rails_tokens)

  -- Load from the cache.
  local ruby2 = lexer.load('ruby', nil, true)
  assert_lex(ruby, code, ruby_tokens)
  assert(ruby == ruby2)

  -- Load without a cache and perform the same validations.
  ruby = lexer.load('ruby')
  assert_lex(ruby, code, ruby_tokens)
  rails = lexer.load('rails')
  assert_lex(rails, code, rails_tokens)
  ruby2 = lexer.load('ruby')
  assert_lex(ruby, code, ruby_tokens)
  assert(ruby ~= ruby2)
end

-- Tests the RHTML lexer, which is a proxy for HTML and Rails.
function test_rhtml()
  local rhtml = lexer.load('rhtml')

  -- Lexing tests.
  -- Start in HTML.
  local code = [[<h1><% puts "hi" %></h1>]]
  local rhtml_tokens = {
    {'element', '<h1'},
    {'element', '>'},
    {'rhtml_tag', '<%'},
    {lexer.FUNCTION, 'puts'},
    {lexer.STRING, '"hi"'},
    {'rhtml_tag', '%>'},
    {'element', '</h1'},
    {'element', '>'}
  }
  local initial_style = rhtml._TOKENSTYLES['html_whitespace']
  assert_lex(rhtml, code, rhtml_tokens, initial_style)
  -- Start in Ruby.
  code = [[puts "hi"]]
  rhtml_tokens = {
    {lexer.FUNCTION, 'puts'},
    {lexer.STRING, '"hi"'}
  }
  initial_style = rhtml._TOKENSTYLES['rails_whitespace']
  assert_lex(rhtml, code, rhtml_tokens, initial_style)
end

-- Run tests.
print('Starting test suite.')
local tests = {}
if #arg == 0 then
  for k, v in pairs(_G) do
    if k:find('^test_') and type(v) == 'function' then
      tests[#tests + 1] = k
    end
  end
else
  for i = 1, #arg do
    if type(_G[arg[i]]) == 'function' then tests[#tests + 1] = arg[i] end
  end
end
table.sort(tests)
local failed = 0
for i = 1, #tests do
  print(string.format('Running %s.', tests[i]))
  local ok, errmsg = xpcall(_G[tests[i]], function(errmsg)
    print(string.format('Failed! %s', debug.traceback(errmsg, 3)))
    failed = failed + 1
  end)
end
print(string.format('%d/%d tests passed', #tests - failed, #tests))

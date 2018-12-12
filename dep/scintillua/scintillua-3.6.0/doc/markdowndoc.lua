-- Copyright 2007-2015 Mitchell mitchell.att.foicica.com. See LICENSE.

local ipairs, type = ipairs, type
local io_open, io_popen = io.open, io.popen
local string_format, string_rep = string.format, string.rep
local table_concat = table.concat

-- Markdown doclet for Luadoc.
-- Requires Discount (http://www.pell.portland.or.us/~orc/Code/discount/).
-- @usage luadoc -d [output_path] -doclet path/to/markdowndoc [file(s)]
local M = {}

local NAVFILE = '1. [%s](%s)\n'
local SCINTILLUA = '<a id="%s"></a>\n# %s\n\n'
local MODULE = '<a id="%s"></a>\n# The `%s` Module\n\n'
local FIELD = '<a id="%s"></a>\n### `%s` %s\n\n'
local FUNCTION = '<a id="%s"></a>\n### `%s` (%s)\n\n'
local DESCRIPTION = '%s\n\n'
local LIST_TITLE = '%s:\n\n'
local PARAM = '* `%s`: %s\n'
local USAGE = '* `%s`\n'
local RETURN = '* %s\n'
local SEE = '* [`%s`](#%s)\n'
local TABLE = '<a id="%s"></a>\n### `%s`\n\n'
local TFIELD = '* `%s`: %s\n'
local HTML = [[
  <!doctype html>
  <html>
    <head>
      <title>%(title)</title>
      <link rel="stylesheet" href="style.css" type="text/css" />
      <link rel="icon" href="icon.png" type="image/png" />
      <meta charset="utf-8" />
    </head>
    <body>
      <div id="content">
        <div id="header">
          %(header)
        </div>
        <div id="main">
          <h1>Scintillua API Documentation</h1>
          <p><strong>Modules</strong></p>
          %(toc)
          <hr />
          %(main)
        </div>
        <div id="footer">
          %(footer)
        </div>
      </div>
    </body>
  </html>
]]
local titles = {
  [PARAM] = 'Parameters', [USAGE] = 'Usage', [RETURN] = 'Return',
  [SEE] = 'See also', [TFIELD] = 'Fields'
}

-- Writes a LuaDoc description to the given file.
-- @param f The markdown file being written to.
-- @param description The description.
-- @param name The name of the module the description belongs to. Used for
--   headers in module descriptions.
local function write_description(f, description, name)
  if name then
    -- Add anchors for module description headers.
    description = description:gsub('\n(#+%s+([^\n]+))', function(header, text)
      return string_format("\n\n<a id=\"%s.%s\"></a>\n\n%s", name,
                           text:gsub(' ', '.'), header)
    end)
  end
  -- Substitute custom [`code`]() link convention with [`code`](#code) links.
  local self_link = '(%[`([^`(]+)%(?%)?`%])%(%)'
  description = description:gsub(self_link, function(link, id)
    return string_format("%s(#%s)", link, id:gsub(':', '.'))
  end)
  f:write(string_format(DESCRIPTION, description))
end

-- Writes a LuaDoc list to the given file.
-- @param f The markdown file being written to.
-- @param fmt The format of a list item.
-- @param list The LuaDoc list.
-- @param name The name of the module the list belongs to. Used for @see.
local function write_list(f, fmt, list, name)
  if not list or #list == 0 then return end
  if type(list) == 'string' then list = {list} end
  f:write(string_format(LIST_TITLE, titles[fmt]))
  for _, value in ipairs(list) do
    if fmt == SEE and name ~= 'Scintillua' then
      -- Prepend module name to identifier if necessary.
      if not value:find('%.') then value = name..'.'..value end
    end
    f:write(string_format(fmt, value, value))
  end
  f:write('\n')
end

-- Writes a LuaDoc hashmap to the given file.
-- @param f The markdown file being written to.
-- @param fmt The format of a hashmap item.
-- @param list The LuaDoc hashmap.
local function write_hashmap(f, fmt, hashmap)
  if not hashmap or #hashmap == 0 then return end
  f:write(string_format(LIST_TITLE, titles[fmt]))
  for _, name in ipairs(hashmap) do
    f:write(string_format(fmt, name, hashmap[name] or ''))
  end
  f:write('\n')
end

-- Called by LuaDoc to process a doc object.
-- @param doc The LuaDoc doc object.
function M.start(doc)
  local template = {title = '', header = '', toc = '', main = '', footer = ''}
  local modules, files = doc.modules, doc.files

  -- Create the header and footer, if given a template.
  if M.options.template_dir ~= 'luadoc/doclet/html/' then
    local p = io.popen('markdown "'..M.options.template_dir..'.header.md"')
    template.header = p:read('*a')
    p:close()
    p = io.popen('markdown "'..M.options.template_dir..'.footer.md"')
    template.footer = p:read('*a')
    p:close()
  end

  -- Create the table of contents.
  local tocfile = M.options.output_dir..'/.api_toc.md'
  local f = io_open(tocfile, 'wb')
  for _, name in ipairs(modules) do
    f:write(string_format(NAVFILE, name, '#'..name))
  end
  f:close()
  local p = io_popen('markdown "'..tocfile..'"')
  local toc = p:read('*a')
  p:close()
  os.remove(tocfile)

  -- Create a map of doc objects to file names so their Markdown doc comments
  -- can be extracted.
  local filedocs = {}
  for _, name in ipairs(files) do filedocs[files[name].doc] = name end

  -- Loop over modules, creating Markdown documents.
  local mdfile = M.options.output_dir..'/api.md'
  local f = io_open(mdfile, 'wb')
  for _, name in ipairs(modules) do
    local module = modules[name]
    local filename = filedocs[module.doc]

    -- Write the header and description.
    if name == 'Scintillua' then
      f:write(string_format(SCINTILLUA, name, name))
    else
      f:write(string_format(MODULE, name, name))
    end
    f:write('- - -\n\n')
    write_description(f, module.description, name)

    -- Write fields.
    if module.doc[1].class == 'module' then
      local fields = module.doc[1].field
      if fields and #fields > 0 then
        table.sort(fields)
        f:write('## Fields defined by `', name, '`\n\n')
        for _, field in ipairs(fields) do
          local type, description = fields[field]:match('^(%b())%s*(.+)$')
          if not field:find('%.') then field = name..'.'..field end
          f:write(string_format(FIELD, field, field, type or ''))
          write_description(f, description or fields[field])
        end
        f:write('\n')
      end
    end

    -- Write functions.
    local funcs = module.functions
    if #funcs > 0 then
      f:write('## Functions defined by `', name, '`\n\n')
      for _, fname in ipairs(funcs) do
        local func = funcs[fname]
        local params = table_concat(func.param, ', '):gsub('_', '\\_')
        if name == 'Scintillua' then
          f:write(string_format(FUNCTION, func.name, 'SCI_PRIVATELEXERCALL',
                                params))
        else
          if not func.name:find('%.') then func.name = name..'.'..func.name end
          f:write(string_format(FUNCTION, func.name, func.name, params))
        end
        write_description(f, func.description)
        write_hashmap(f, PARAM, func.param)
        write_list(f, USAGE, func.usage)
        write_list(f, RETURN, func.ret)
        write_list(f, SEE, func.see, name)
      end
      f:write('\n')
    end

    -- Write tables.
    local tables = module.tables
    if #tables > 0 then
      f:write('## Tables defined by `', name, '`\n\n')
      for _, tname in ipairs(tables) do
        local tbl = tables[tname]
        if not tbl.name:find('%.') then tbl.name = name..'.'..tbl.name end
        f:write(string_format(TABLE, tbl.name, tbl.name))
        write_description(f, tbl.description)
        write_hashmap(f, TFIELD, tbl.field)
        write_list(f, USAGE, tbl.usage)
        write_list(f, SEE, tbl.see, name)
      end
    end
    f:write('- - -\n\n')
  end

  -- Write HTML.
  template.title = 'Scintillua API'
  template.toc = toc
  local p = io_popen('markdown "'..mdfile..'"')
  template.main = p:read('*a')
  p:close()
  f = io_open(M.options.output_dir..'/api.html', 'wb')
  local html = HTML:gsub('%%%(([^)]+)%)', template)
  f:write(html)
  f:close()
end

return M

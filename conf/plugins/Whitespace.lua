function options (opts)
  local list = {"In indentation", "Everywhere"}
  opts:define_list("tabs", "Disallow tabs", list, 1)
  opts:define_boolean("blank", "Ignore blank lines", false)
end

function kinds (kinds, opts)
  kinds:define_error(
    "tabs",
    "Tabs",
    "tab character added",
    "Tab characters are not allowed."
  )

  kinds:define_error(
    "trailing",
    "Trailing",
    "trailing whitespace added",
    "Trailing whitespace is not allowed.",
    true -- enabled by default
  )
end

function hunk (hunk, opts)
  if hunk:lexer() == "null" then
    return
  end

  local blank = opts:value("blank")
  local indent = opts:value("tabs") == 1
  for _, line in ipairs(hunk:lines()) do
    local lexemes = line:lexemes()
    local _, first = next(lexemes)
    if line:origin() == "+" and first then
      -- Check for tabs.
      local text = indent and first:text() or line:text()
      if text:find("\t") then
        local replacement = text:gsub("\t", string.rep(" ", hunk:tab_width()))
        line:add_error("tabs", 1, text:len(), replacement)
      end

      -- Check for trailing whitespace.
      local last = lexemes[#lexemes]
      if last:is_kind("whitespace") and (not blank or first ~= last) then
        local len = last:text():len()
        local line_len = line:text():gsub("[\r\n]+$", ""):len()
        line:add_error("trailing", line_len - len + 1, len, "")
      end
    end
  end
end

function options (opts)
  opts:define_integer("soft", "Soft limit", 80)
  opts:define_integer("hard", "Hard limit", 85)
end

function kinds (kinds, opts)
  kinds:define_warning(
    "soft",
    "Soft Limit",
    "line exceeds length limit",
    string.format("Lines should not extend past column %d.", opts:value("soft"))
  )

  kinds:define_error(
    "hard",
    "Hard Limit",
    "line exceeds length limit",
    string.format("Lines must not extend past column %d.", opts:value("hard"))
  )
end

function hunk (hunk, opts)
  if hunk:lexer() == "null" then
    return
  end

  local soft = opts:value("soft")
  local hard = opts:value("hard")
  for _, line in ipairs(hunk:lines()) do
    if line:origin() == "+" then
      -- Get tab-expanded column.
      local len = #line:text():gsub("[\r\n]+$", "")
      if len > 0 then
        local column = line:column(len)
        if column > hard then
          -- Report range from soft limit.
          local pos = line:column_pos(soft)
          line:add_error("hard", pos + 1, len - pos)
        elseif column > soft then
          local pos = line:column_pos(soft)
          line:add_error("soft", pos + 1, len - pos)
        end
      end
    end
  end
end

function options (opts)
  opts:define_string("keywords", "Unfinished keywords", "TODO FIXME TBD")
end

function kinds (kinds, opts)
  kinds:define_note(
    "unfinished",
    "Unfinished",
    "unfinished comment keyword added",
    "Flag comments that contain 'TODO', 'FIXME', 'TBD', etc."
  )
end

function hunk (hunk, opts)
  if hunk:lexer() == "null" then
    return
  end

  -- Split keywords.
  local keywords = {}
  for keyword in opts:value("keywords"):gmatch("%w+") do
    keywords[keyword] = true
  end

  for _, line in ipairs(hunk:lines()) do
    if line:origin() == "+" then
      for _, lexeme in ipairs(line:lexemes()) do
        if lexeme:is_kind("comment") then
          local text = lexeme:text()
          local i, j = text:find("%w+")
          while i do
            if keywords[text:sub(i, j)] then
              line:add_error("unfinished", lexeme:pos() + i - 1, j - i + 1)
            end

            i, j = text:find("%w+", j + 1)
          end
        end
      end
    end
  end
end

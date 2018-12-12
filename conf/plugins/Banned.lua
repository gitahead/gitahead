function options (opts)
  local dir = opts:script_dir()
  opts:define_string("file", "Banned words file", dir .. "/banned.txt")
end

function kinds (kinds, opts)
  kinds:define_error(
    "comment",
    "Comment",
    "banned word used in comment",
    "Banned words are not allowed in comments."
  )

  kinds:define_error(
    "literal",
    "String",
    "banned word used in string literal",
    "Banned words are not allowed in string literals."
  )

  kinds:define_error(
    "identifier",
    "Identifier",
    "banned word used as identifier",
    "Banned words are not allowed as identifiers."
  )
end

function hunk (hunk, opts)
  local words = {}
  for word in io.lines(opts:value("file")) do
    words[word] = true
  end

  for _, line in ipairs(hunk:lines()) do
    if line:origin() == "+" then
      for _, lexeme in ipairs(line:lexemes()) do
        if lexeme:is_kind("comment") or lexeme:is_kind("string") then
          local text = lexeme:text()
          local i, j = text:find("%w+")
          while i do
            if words[text:sub(i, j)] then
              local kind = "comment"
              if lexeme:is_kind("string") then
                kind = "literal"
              end

              local len = j - i + 1
              local replacement = string.sub("@#$%&*!", 1, len)
              line:add_error(kind, lexeme:pos() + i - 1, len, replacement)
            end

            i, j = text:find("%w+", j + 1)
          end

        elseif lexeme:is_kind("identifier") then
          local text = lexeme:text()
          if words[text] then
            line:add_error("identifier", lexeme:pos(), #text)
          end
        end
      end
    end
  end
end

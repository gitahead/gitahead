//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

// Based largely on LexLPeg.cxx.

#include "LPegLexer.h"
#include <QFileInfo>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
LUALIB_API int luaopen_lpeg(lua_State *L);
}

LPegLexer::LPegLexer(
  const QByteArray &home,
  const QByteArray &lexer,
  QObject *parent)
  : Lexer(parent), mL(luaL_newstate(), lua_close), mName(lexer)
{
  Q_ASSERT(!home.isEmpty() && !lexer.isEmpty());

  lua_State *L = mL.data();

  luaL_openlibs(L);
  luaL_requiref(L, "lpeg", luaopen_lpeg, 1), lua_pop(L, 1);

  // Modify `package.path` to find lexers.
  lua_getglobal(L, "package"), lua_getfield(L, -1, "path");
  int orig_path = luaL_ref(L, LUA_REGISTRYINDEX); // restore later
  lua_pushstring(L, home), lua_pushstring(L, "/?.lua"), lua_concat(L, 2);
  lua_setfield(L, -2, "path"), lua_pop(L, 1); // package

  // Load the lexer module.
  lua_getglobal(L, "require");
  lua_pushstring(L, "lexer");
  lua_pcall(L, 1, 1, 0);

  // Restore `package.path`.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path"), lua_setfield(L, -3, "LEXERPATH");
  lua_rawgeti(L, LUA_REGISTRYINDEX, orig_path), lua_setfield(L, -2, "path");
  luaL_unref(L, LUA_REGISTRYINDEX, orig_path), lua_pop(L, 1); // package

  // Load lexer from absolute path.
  QFileInfo info(lexer);
  if (info.isAbsolute()) {
    lua_pushstring(L, info.absoluteFilePath().toUtf8());
    lua_setfield(L, -2, "LEXERPATH");
    mName = info.baseName().toUtf8();
  }

  // Load the language lexer.
  lua_getfield(L, -1, "load");
  lua_pushstring(L, mName);
  lua_pcall(L, 1, 1, 0);

  // Leave lexer object on top of stack.
  lua_remove(L, -2); // lexer module
}

bool LPegLexer::lex(const QByteArray &buffer)
{
  lua_State *L = mL.data();
  if (!lua_checkstack(L, 4))
    return false;

  // Initialize lex data.
  mIndex = 1;
  mLength = 0;
  mStartPos = 0;
  mBuffer = buffer;

  // Lex the buffer.
  lua_getfield(L, -1, "lex");
  lua_pushvalue(L, -2); // lexer object
  lua_pushlstring(L, buffer, buffer.length());
  lua_pushinteger(L, Nothing); // initial state
  lua_pcall(L, 3, 1, 0);

  // Bail out if lex didn't return a table.
  if (!lua_istable(L, -1))
    return false;

  mLength = lua_rawlen(L, -1);
  lua_getfield(L, -2, "_TOKENSTYLES");
  return true;
}

bool LPegLexer::hasNext()
{
  return (mIndex < mLength);
}

Lexer::Lexeme LPegLexer::next()
{
  lua_State *L = mL.data();

  lua_rawgeti(L, -2, mIndex), lua_rawget(L, -2); // _TOKENSTYLES[token]
  int token = !lua_isnil(L, -1) ? lua_tointeger(L, -1) : Nothing;
  lua_pop(L, 1); // _TOKENSTYLES[token]

  lua_rawgeti(L, -2, mIndex + 1); // endPos
  int endPos = lua_tointeger(L, -1) - 1;
  QByteArray text = mBuffer.mid(mStartPos, endPos - mStartPos);
  lua_pop(L, 1); // endPos

  mStartPos = endPos;
  mIndex += 2;

  if (!hasNext())
    lua_pop(L, 2); // _TOKENSTYLES and token table

  return {static_cast<Token>(token), text};
}

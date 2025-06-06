/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "lua.h"
#include "lua_privileges.h"
#include "ldo.h"

/* A copy of lua_load passing privileged to luaD_protectedparser */
LUA_API int lua_load_privileged (lua_State *L, lua_Reader reader, void *data,
                      const char *chunkname) {
  ZIO z;
  int status;
  lua_lock(L);
  if (!chunkname) chunkname = "?";
  luaZ_init(L, &z, reader, data);
  status = luaD_protectedparser(L, &z, chunkname, true);
  lua_unlock(L);
  return status;
}



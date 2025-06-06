/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef lua_privileges_h
#define lua_privileges_h

LUALIB_API int (luaL_loadbuffer_privileged) (lua_State *L, const char *buff, size_t sz,
                                  const char *name, bool privileged);
LUA_API int   (lua_load_privileged) (lua_State *L, lua_Reader reader, void *dt,
                                        const char *chunkname);

#endif // lua_privileges_h

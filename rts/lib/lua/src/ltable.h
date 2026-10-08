/*
** $Id: ltable.h,v 2.10.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#ifndef ltable_h
#define ltable_h

#include "lobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	(&(n)->i_key.tvk)


LUAI_FUNC const TValue *luaH_getnum (Table *t, int key);
LUAI_FUNC const TValue *luaH_getnumhash (Table *t, int key);
LUAI_FUNC TValue *luaH_setnum (lua_State *L, Table *t, int key);
LUAI_FUNC const TValue *luaH_getstr (Table *t, TString *key);
LUAI_FUNC TValue *luaH_setstr (lua_State *L, Table *t, TString *key);
LUAI_FUNC const TValue *luaH_get (Table *t, const TValue *key);
LUAI_FUNC TValue *luaH_set (lua_State *L, Table *t, const TValue *key);
LUAI_FUNC Table *luaH_new (lua_State *L, int narray, int lnhash);
LUAI_FUNC void luaH_resizearray (lua_State *L, Table *t, int nasize);
LUAI_FUNC void luaH_free (lua_State *L, Table *t);
LUAI_FUNC int luaH_next (lua_State *L, Table *t, StkId key);
LUAI_FUNC int luaH_getn (Table *t);


#if defined(LUA_CORE)
/*
** Recoil: inline versions of the hot lookups, for the VM fast paths.
** They do exactly the same probes as luaH_getstr / luaH_getnum / luaH_get
** (same main position, same chain walk, same comparisons) and return the
** same pointers; they only avoid the out-of-line calls and the generic
** type switch. Hashing and node layout are untouched.
*/
static inline const TValue *luaH_getstr_inl (Table *t, TString *key) {
  Node *n = gnode(t, lmod(key->tsv.hash, sizenode(t)));  /* hashstr */
  do {  /* check whether `key' is somewhere in the chain */
    if (ttisstring(gkey(n)) && rawtsvalue(gkey(n)) == key)
      return gval(n);  /* that's it */
    else n = gnext(n);
  } while (n);
  return luaO_nilobject;
}

static inline const TValue *luaH_getnum_inl (Table *t, int key) {
  /* (1 <= key && key <= t->sizearray) */
  if (lua_cast(unsigned int, key) - 1u < lua_cast(unsigned int, t->sizearray))
    return &t->array[key-1];
  return luaH_getnumhash(t, key);
}

static inline const TValue *luaH_get_inl (Table *t, const TValue *key) {
  switch (ttype(key)) {
    case LUA_TSTRING: return luaH_getstr_inl(t, rawtsvalue(key));
    case LUA_TNUMBER: {
      int k;
      lua_Number n = nvalue(key);
      lua_number2int(k, n);
      if (luai_numeq(cast_num(k), nvalue(key))) /* index is int? */
        return luaH_getnum_inl(t, k);  /* use specialized version */
      return luaH_get(t, key);  /* non-integral number: generic lookup */
    }
    case LUA_TNIL: return luaO_nilobject;
    default: return luaH_get(t, key);
  }
}
#endif


#if defined(LUA_DEBUG)
LUAI_FUNC Node *luaH_mainposition (const Table *t, const TValue *key);
LUAI_FUNC int luaH_isdummy (Node *n);
#endif


#endif

/*
** $Id: lvm.c,v 2.63.1.5 2011/08/17 20:43:11 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//SPRING
#include <algorithm>
#include "streflop_cond.h"

#define lvm_c
#define LUA_CORE

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lvm.h"



/* limit for table tag-method chains (to avoid loops) */
#define MAXTAGLOOP	100


const TValue *luaV_tonumber (const TValue *obj, TValue *n) {
  lua_Number num;
  if (ttisnumber(obj)) return obj;
  if (ttisstring(obj) && luaO_str2d(svalue(obj), &num)) {
    setnvalue(n, num);
    return n;
  }
  else
    return NULL;
}


int luaV_tostring (lua_State *L, StkId obj) {
  if (!ttisnumber(obj))
    return 0;
  else {
    char s[LUAI_MAXNUMBER2STR];
    lua_Number n = nvalue(obj);
    // SPRING -- synced safety change
    //        -- need a custom number formatter?
    if (math::isfinite(n)) {
      lua_number2str(s, n);
    }
    else {
      if (math::isnan(n)) {
        strcpy(s, "nan");
      }
      else {
        const int inf_type = math::isinf(n);
        if (inf_type == 1) {
          strcpy(s, "+inf");
        } else if (inf_type == -1) {
          strcpy(s, "-inf");
        } else {
          strcpy(s, "weird_number");
        }
      }
    } 
    setsvalue2s(L, obj, luaS_new(L, s));
    return 1;
  }
}


static void traceexec (lua_State *L, const Instruction *pc) {
  lu_byte mask = L->hookmask;
  const Instruction *oldpc = L->savedpc;
  L->savedpc = pc;
  if ((mask & LUA_MASKCOUNT) && L->hookcount == 0) {
    resethookcount(L);
    luaD_callhook(L, LUA_HOOKCOUNT, -1);
  }
  if (mask & LUA_MASKLINE) {
    Proto *p = ci_func(L->ci)->l.p;
    int npc = pcRel(pc, p);
    int newline = getline(p, npc);
    /* call linehook when enter a new function, when jump back (loop),
       or when enter a new line */
    if (npc == 0 || pc <= oldpc || newline != getline(p, pcRel(oldpc, p)))
      luaD_callhook(L, LUA_HOOKLINE, newline);
  }
}


static void callTMres (lua_State *L, StkId res, const TValue *f,
                        const TValue *p1, const TValue *p2) {
  ptrdiff_t result = savestack(L, res);
  setobj2s(L, L->top, f);  /* push function */
  setobj2s(L, L->top+1, p1);  /* 1st argument */
  setobj2s(L, L->top+2, p2);  /* 2nd argument */
  luaD_checkstack(L, 3);
  L->top += 3;
  luaD_call(L, L->top - 3, 1);
  res = restorestack(L, result);
  L->top--;
  setobjs2s(L, res, L->top);
}



static void callTM (lua_State *L, const TValue *f, const TValue *p1,
                    const TValue *p2, const TValue *p3) {
  setobj2s(L, L->top, f);  /* push function */
  setobj2s(L, L->top+1, p1);  /* 1st argument */
  setobj2s(L, L->top+2, p2);  /* 2nd argument */
  setobj2s(L, L->top+3, p3);  /* 3th argument */
  luaD_checkstack(L, 4);
  L->top += 4;
  luaD_call(L, L->top - 4, 0);
}


/*
** the `gettable' loop, starting at iteration `loop' (luaV_gettable starts
** at 0; luaV_gettable_tm continues at 1 after the VM did iteration 0)
*/
static void gettable_loop (lua_State *L, const TValue *t, TValue *key,
                           StkId val, int loop) {
  for (; loop < MAXTAGLOOP; loop++) {
    const TValue *tm;
    if (ttistable(t)) {  /* `t' is a table? */
      Table *h = hvalue(t);
      const TValue *res = luaH_get_inl(h, key); /* do a primitive get */
      if (!ttisnil(res) ||  /* result is no nil? */
          (tm = fasttm(L, h->metatable, TM_INDEX)) == NULL) { /* or no TM? */
        setobj2s(L, val, res);
        return;
      }
      /* else will try the tag method */
    }
    else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_INDEX)))
      luaG_typeerror(L, t, "index");
    if (ttisfunction(tm)) {
      callTMres(L, val, tm, t, key);
      return;
    }
    t = tm;  /* else repeat with `tm' */
  }
  luaG_runerror(L, "loop in gettable");
}


void luaV_gettable (lua_State *L, const TValue *t, TValue *key, StkId val) {
  gettable_loop(L, t, key, val, 0);
}


/*
** Recoil: rest of luaV_gettable's first iteration for a table `t' whose
** primitive get returned nil and whose metatable has the __index entry
** `tm' (as found by fasttm); used by the VM's inline fast paths.
*/
static void luaV_gettable_tm (lua_State *L, const TValue *t, TValue *key,
                              StkId val, const TValue *tm) {
  if (ttisfunction(tm)) {
    callTMres(L, val, tm, t, key);
    return;
  }
  gettable_loop(L, tm, key, val, 1);  /* else repeat with `tm' */
}


/*
** Recoil: luaH_set with the inline lookup. luaH_set clears the flags and
** returns the existing slot, or inserts the key; the insertion (and its
** checks) is left to luaH_set itself, which repeats the (pure) lookup.
*/
static inline TValue *luaH_set_inl (lua_State *L, Table *h, const TValue *key) {
  const TValue *p = luaH_get_inl(h, key);
  if (p != luaO_nilobject) {
    h->flags = 0;
    return lua_cast(TValue *, p);
  }
  return luaH_set(L, h, key);
}


void luaV_settable (lua_State *L, const TValue *t, TValue *key, StkId val) {
  int loop;
  TValue temp;
  for (loop = 0; loop < MAXTAGLOOP; loop++) {
    const TValue *tm;
    if (ttistable(t)) {  /* `t' is a table? */
      Table *h = hvalue(t);
      TValue *oldval = luaH_set_inl(L, h, key); /* do a primitive set */
      if (!ttisnil(oldval) ||  /* result is no nil? */
          (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) { /* or no TM? */
        setobj2t(L, oldval, val);
        h->flags = 0;
        luaC_barriert(L, h, val);
        return;
      }
      /* else will try the tag method */
    }
    else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_NEWINDEX)))
      luaG_typeerror(L, t, "index");
    if (ttisfunction(tm)) {
      callTM(L, tm, t, key, val);
      return;
    }
    /* else repeat with `tm' */
    setobj(L, &temp, tm);  /* avoid pointing inside table (may rehash) */
    t = &temp;
  }
  luaG_runerror(L, "loop in settable");
}


static int call_binTM (lua_State *L, const TValue *p1, const TValue *p2,
                       StkId res, TMS event) {
  const TValue *tm = luaT_gettmbyobj(L, p1, event);  /* try first operand */
  if (ttisnil(tm))
    tm = luaT_gettmbyobj(L, p2, event);  /* try second operand */
  if (ttisnil(tm)) return 0;
  callTMres(L, res, tm, p1, p2);
  return 1;
}


static const TValue *get_compTM (lua_State *L, Table *mt1, Table *mt2,
                                  TMS event) {
  const TValue *tm1 = fasttm(L, mt1, event);
  const TValue *tm2;
  if (tm1 == NULL) return NULL;  /* no metamethod */
  if (mt1 == mt2) return tm1;  /* same metatables => same metamethods */
  tm2 = fasttm(L, mt2, event);
  if (tm2 == NULL) return NULL;  /* no metamethod */
  if (luaO_rawequalObj(tm1, tm2))  /* same metamethods? */
    return tm1;
  return NULL;
}


static int call_orderTM (lua_State *L, const TValue *p1, const TValue *p2,
                         TMS event) {
  const TValue *tm1 = luaT_gettmbyobj(L, p1, event);
  const TValue *tm2;
  if (ttisnil(tm1)) return -1;  /* no metamethod? */
  tm2 = luaT_gettmbyobj(L, p2, event);
  if (!luaO_rawequalObj(tm1, tm2))  /* different metamethods? */
    return -1;
  callTMres(L, L->top, tm1, p1, p2);
  return !l_isfalse(L->top);
}


static int l_strcmp (const TString *ls, const TString *rs) {
  const char *l = getstr(ls);
  size_t ll = ls->tsv.len;
  const char *r = getstr(rs);
  size_t lr = rs->tsv.len;
  //SPRING
  const size_t n = std::min(ll, lr) + 1;
  //SPRING
  for (;;) {
    //SPRING
    //int temp = strcoll(l, r);
    int temp = strncmp(l, r, n);
    //SPRING
    if (temp != 0) return temp;
    else {  /* strings are equal up to a `\0' */
      size_t len = strlen(l);  /* index of first `\0' in both strings */
      if (len == lr)  /* r is finished? */
        return (len == ll) ? 0 : 1;
      else if (len == ll)  /* l is finished? */
        return -1;  /* l is smaller than r (because r is not finished) */
      /* both strings longer than `len'; go on comparing (after the `\0') */
      len++;
      l += len; ll -= len; r += len; lr -= len;
    }
  }
}


int luaV_lessthan (lua_State *L, const TValue *l, const TValue *r) {
  int res;
  if (ttype(l) != ttype(r))
    return luaG_ordererror(L, l, r);
  else if (ttisnumber(l))
    return luai_numlt(nvalue(l), nvalue(r));
  else if (ttisstring(l))
    return l_strcmp(rawtsvalue(l), rawtsvalue(r)) < 0;
  else if ((res = call_orderTM(L, l, r, TM_LT)) != -1)
    return res;
  return luaG_ordererror(L, l, r);
}


static int lessequal (lua_State *L, const TValue *l, const TValue *r) {
  int res;
  if (ttype(l) != ttype(r))
    return luaG_ordererror(L, l, r);
  else if (ttisnumber(l))
    return luai_numle(nvalue(l), nvalue(r));
  else if (ttisstring(l))
    return l_strcmp(rawtsvalue(l), rawtsvalue(r)) <= 0;
  else if ((res = call_orderTM(L, l, r, TM_LE)) != -1)  /* first try `le' */
    return res;
  else if ((res = call_orderTM(L, r, l, TM_LT)) != -1)  /* else try `lt' */
    return !res;
  return luaG_ordererror(L, l, r);
}


int luaV_equalval (lua_State *L, const TValue *t1, const TValue *t2) {
  const TValue *tm;
  lua_assert(ttype(t1) == ttype(t2));
  switch (ttype(t1)) {
    case LUA_TNIL: return 1;
    case LUA_TNUMBER: return luai_numeq(nvalue(t1), nvalue(t2));
    case LUA_TBOOLEAN: return bvalue(t1) == bvalue(t2);  /* true must be 1 !! */
    case LUA_TLIGHTUSERDATA: return pvalue(t1) == pvalue(t2);
    case LUA_TUSERDATA: {
      if (uvalue(t1) == uvalue(t2)) return 1;
      tm = get_compTM(L, uvalue(t1)->metatable, uvalue(t2)->metatable,
                         TM_EQ);
      break;  /* will try TM */
    }
    case LUA_TTABLE: {
      if (hvalue(t1) == hvalue(t2)) return 1;
      tm = get_compTM(L, hvalue(t1)->metatable, hvalue(t2)->metatable, TM_EQ);
      break;  /* will try TM */
    }
    default: return gcvalue(t1) == gcvalue(t2);
  }
  if (tm == NULL) return 0;  /* no TM? */
  callTMres(L, L->top, tm, t1, t2);  /* call TM */
  return !l_isfalse(L->top);
}


void luaV_concat (lua_State *L, int total, int last) {
  do {
    StkId top = L->base + last + 1;
    int n = 2;  /* number of elements handled in this pass (at least 2) */
    if (!(ttisstring(top-2) || ttisnumber(top-2)) || !tostring(L, top-1)) {
      if (!call_binTM(L, top-2, top-1, top-2, TM_CONCAT))
        luaG_concaterror(L, top-2, top-1);
    } else if (tsvalue(top-1)->len == 0)  /* second op is empty? */
      (void)tostring(L, top - 2);  /* result is first op (as string) */
    else {
      /* at least two string values; get as many as possible */
      size_t tl = tsvalue(top-1)->len;
      char *buffer;
      int i;
      /* collect total length */
      for (n = 1; n < total && tostring(L, top-n-1); n++) {
        size_t l = tsvalue(top-n-1)->len;
        if (l >= MAX_SIZET - tl) luaG_runerror(L, "string length overflow");
        tl += l;
      }
      buffer = luaZ_openspace(L, &G(L)->buff, tl);
      tl = 0;
      for (i=n; i>0; i--) {  /* concat all strings */
        size_t l = tsvalue(top-i)->len;
        memcpy(buffer+tl, svalue(top-i), l);
        tl += l;
      }
      setsvalue2s(L, top-n, luaS_newlstr(L, buffer, tl));
    }
    total -= n-1;  /* got `n' strings to create 1 new */
    last -= n-1;
  } while (total > 1);  /* repeat until only 1 result left */
}


static void Arith (lua_State *L, StkId ra, const TValue *rb,
                   const TValue *rc, TMS op) {
  TValue tempb, tempc;
  const TValue *b, *c;
  if ((b = luaV_tonumber(rb, &tempb)) != NULL &&
      (c = luaV_tonumber(rc, &tempc)) != NULL) {
    lua_Number nb = nvalue(b), nc = nvalue(c);
    switch (op) {
      case TM_ADD: setnvalue(ra, luai_numadd(nb, nc)); break;
      case TM_SUB: setnvalue(ra, luai_numsub(nb, nc)); break;
      case TM_MUL: setnvalue(ra, luai_nummul(nb, nc)); break;
      case TM_DIV: setnvalue(ra, luai_numdiv(nb, nc)); break;
      case TM_MOD: setnvalue(ra, luai_nummod(nb, nc)); break;
      case TM_POW: setnvalue(ra, luai_numpow(nb, nc)); break;
      case TM_UNM: setnvalue(ra, luai_numunm(nb)); break;
      default: lua_assert(0); break;
    }
  }
  else if (!call_binTM(L, rb, rc, ra, op))
    luaG_aritherror(L, rb, rc);
}



/*
** Recoil: luaD_poscall for the case without a return hook (the callers
** test L->hookmask & LUA_MASKRET and use luaD_poscall otherwise).
*/
static inline int poscall_nohook (lua_State *L, StkId firstResult) {
  StkId res;
  int wanted, i;
  CallInfo *ci = L->ci--;
  res = ci->func;  /* res == final position of 1st result */
  wanted = ci->nresults;
  L->base = (ci - 1)->base;  /* restore base */
  L->savedpc = (ci - 1)->savedpc;  /* restore savedpc */
  /* move results to correct place */
  for (i = wanted; i != 0 && firstResult < L->top; i--)
    setobjs2s(L, res++, firstResult++);
  while (i-- > 0)
    setnilvalue(res++);
  L->top = res;
  return (wanted - LUA_MULTRET);  /* 0 iff wanted == LUA_MULTRET */
}


/*
** Recoil: true when luaD_precall would call a C function at L->top
** without growing the CallInfo array or the stack and without a call
** hook; its C branch is then exactly precall_C below.
*/
#define canprecallC(L) \
  (!((L)->hookmask & LUA_MASKCALL) && (L)->ci != (L)->end_ci && \
   (char *)(L)->stack_last - (char *)(L)->top > LUA_MINSTACK*(int)sizeof(TValue))


/*
** Recoil: luaD_precall's C-function branch for that case, followed by
** its luaD_poscall. Returns the C function's result count; n < 0 means
** it yielded (PCRYIELD: no poscall).
*/
static inline int precall_C (lua_State *L, StkId func, lua_CFunction f,
                             int nresults) {
  CallInfo *ci;
  int n;
  L->ci->savedpc = L->savedpc;
  ci = ++L->ci;  /* now `enter' new function */
  ci->func = func;
  L->base = ci->base = func + 1;
  ci->top = L->top + LUA_MINSTACK;
  ci->nresults = nresults;
  lua_unlock(L);
  n = (*f)(L);  /* do the actual call */
  lua_lock(L);
  if (n >= 0) {
    if (L->hookmask & LUA_MASKRET)
      luaD_poscall(L, L->top - n);
    else
      poscall_nohook(L, L->top - n);
  }
  return n;
}


/*
** Recoil: luaD_call with the C-function case done by precall_C (generic
** `for' iterators such as next and ipairs); same C-stack checks, same
** order of steps, same GC step afterwards.
*/
static void vm_call (lua_State *L, StkId func, int nResults) {
  if (++L->nCcalls >= LUAI_MAXCCALLS) {
    if (L->nCcalls == LUAI_MAXCCALLS)
      luaG_runerror(L, "C stack overflow");
    else if (L->nCcalls >= (LUAI_MAXCCALLS + (LUAI_MAXCCALLS>>3)))
      luaD_throw(L, LUA_ERRERR);  /* error while handing stack error */
  }
  if (iscfunction(func) && canprecallC(L))
    (void)precall_C(L, func, clvalue(func)->c.f, nResults);
  else if (luaD_precall(L, func, nResults) == PCRLUA)  /* is a Lua function? */
    luaV_execute(L, 1);  /* call it */
  L->nCcalls--;
  luaC_checkGC(L);
}


/*
** some macros for common tasks in `luaV_execute'
*/

#define runtime_check(L, c)	{ if (!(c)) vmbreak; }

#define RA(i)	(base+GETARG_A(i))
/* to be used after possible stack reallocation */
#define RB(i)	check_exp(getBMode(GET_OPCODE(i)) == OpArgR, base+GETARG_B(i))
#define RC(i)	check_exp(getCMode(GET_OPCODE(i)) == OpArgR, base+GETARG_C(i))
#define RKB(i)	check_exp(getBMode(GET_OPCODE(i)) == OpArgK, \
	ISK(GETARG_B(i)) ? k+INDEXK(GETARG_B(i)) : base+GETARG_B(i))
#define RKC(i)	check_exp(getCMode(GET_OPCODE(i)) == OpArgK, \
	ISK(GETARG_C(i)) ? k+INDEXK(GETARG_C(i)) : base+GETARG_C(i))
#define KBx(i)	check_exp(getBMode(GET_OPCODE(i)) == OpArgK, k+GETARG_Bx(i))


#define dojump(L,pc,i)	{(pc) += (i); luai_threadyield(L);}


/*
** Recoil: instruction dispatch. With GCC/Clang the opcode handlers are
** reached through a table of label addresses ("computed goto") and every
** handler ends with its own fetch + dispatch, instead of jumping back to
** a single switch. Fetch, hook check and `ra' computation are the same
** code as the loop head of the switch version (vmfetch); an opcode
** outside the table (impossible for valid code) skips the instruction,
** as an unmatched switch value did. Define LUA_USE_SWITCH_DISPATCH to
** get the plain switch.
*/
#define vmfetch() { \
    i = *pc++; \
    if ((L->hookmask & (LUA_MASKLINE | LUA_MASKCOUNT)) && \
        (--L->hookcount == 0 || L->hookmask & LUA_MASKLINE)) { \
      traceexec(L, pc); \
      if (L->status == LUA_YIELD) {  /* did hook yield? */ \
        L->savedpc = pc - 1; \
        return; \
      } \
      base = L->base; \
    } \
    /* warning!! several calls may realloc the stack and invalidate `ra' */ \
    ra = RA(i); \
    lua_assert(base == L->base && L->base == L->ci->base); \
    lua_assert(base <= L->top && L->top <= L->stack + L->stacksize); \
    lua_assert(L->top == L->ci->top || luaG_checkopenop(i)); \
  }

#if defined(__GNUC__) && !defined(LUA_USE_SWITCH_DISPATCH)
#define LUAV_COMPUTED_GOTO	1
#define vmdispatch(o)	goto *disptab[o];
#define vmcase(l)	L_##l:
#define vmbreak		{ vmfetch(); vmdispatch(GET_OPCODE(i)); }
#else
#define LUAV_COMPUTED_GOTO	0
#define vmdispatch(o)	switch (o)
#define vmcase(l)	case l:
#define vmbreak		continue
#endif


#define Protect(x)	{ L->savedpc = pc; {x;}; base = L->base; }


#define arith_op(op,tm) { \
        TValue *rb = RKB(i); \
        TValue *rc = RKC(i); \
        if (ttisnumber(rb) && ttisnumber(rc)) { \
          lua_Number nb = nvalue(rb), nc = nvalue(rc); \
          setnvalue(ra, op(nb, nc)); \
        } \
        else \
          Protect(Arith(L, ra, rb, rc, tm)); \
      }



void luaV_execute (lua_State *L, int nexeccalls) {
  LClosure *cl;
  StkId base;
  TValue *k;
  const Instruction *pc;
 reentry:  /* entry point */
  lua_assert(isLua(L->ci));
  pc = L->savedpc;
  cl = &clvalue(L->ci->func)->l;
  base = L->base;
  k = cl->p->k;
#if LUAV_COMPUTED_GOTO
  /* ORDER OP; opcodes are 6 bits wide, the rest of the table is unused */
  static const void *const disptab[1 << SIZE_OP] = {
    &&L_OP_MOVE, &&L_OP_LOADK, &&L_OP_LOADBOOL, &&L_OP_LOADNIL,
    &&L_OP_GETUPVAL, &&L_OP_GETGLOBAL, &&L_OP_GETTABLE, &&L_OP_SETGLOBAL,
    &&L_OP_SETUPVAL, &&L_OP_SETTABLE, &&L_OP_NEWTABLE, &&L_OP_SELF,
    &&L_OP_ADD, &&L_OP_SUB, &&L_OP_MUL, &&L_OP_DIV,
    &&L_OP_MOD, &&L_OP_POW, &&L_OP_UNM, &&L_OP_NOT,
    &&L_OP_LEN, &&L_OP_CONCAT, &&L_OP_JMP, &&L_OP_EQ,
    &&L_OP_LT, &&L_OP_LE, &&L_OP_TEST, &&L_OP_TESTSET,
    &&L_OP_CALL, &&L_OP_TAILCALL, &&L_OP_RETURN, &&L_OP_FORLOOP,
    &&L_OP_FORPREP, &&L_OP_TFORLOOP, &&L_OP_SETLIST, &&L_OP_CLOSE,
    &&L_OP_CLOSURE, &&L_OP_VARARG,
    &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown,
    &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown,
    &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown,
    &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown, &&L_unknown,
    &&L_unknown, &&L_unknown
  };
  static_assert(NUM_OPCODES == 38 && SIZE_OP == 6, "update disptab");
#endif
  /* main loop of interpreter */
  for (;;) {
    Instruction i;
    StkId ra;
    vmfetch();
    vmdispatch (GET_OPCODE(i)) {
      vmcase(OP_MOVE) {
        setobjs2s(L, ra, RB(i));
        vmbreak;
      }
      vmcase(OP_LOADK) {
        setobj2s(L, ra, KBx(i));
        vmbreak;
      }
      vmcase(OP_LOADBOOL) {
        setbvalue(ra, GETARG_B(i));
        if (GETARG_C(i)) pc++;  /* skip next instruction (if C) */
        vmbreak;
      }
      vmcase(OP_LOADNIL) {
        TValue *rb = RB(i);
        do {
          setnilvalue(rb--);
        } while (rb >= ra);
        vmbreak;
      }
      vmcase(OP_GETUPVAL) {
        int b = GETARG_B(i);
        setobj2s(L, ra, cl->upvals[b]->v);
        vmbreak;
      }
      /*
      ** Recoil: GETGLOBAL/GETTABLE/SELF/SETGLOBAL/SETTABLE do the first
      ** iteration of luaV_gettable/luaV_settable inline when the indexed
      ** object is a table (same lookup, same fasttm calls in the same
      ** order); metamethods, new keys and non-tables take the generic path.
      */
      vmcase(OP_GETGLOBAL) {
        TValue *rb = KBx(i);
        Table *h = cl->env;
        const TValue *res, *tm;
        lua_assert(ttisstring(rb));
        res = luaH_getstr_inl(h, rawtsvalue(rb));
        if (!ttisnil(res) || (tm = fasttm(L, h->metatable, TM_INDEX)) == NULL) {
          setobj2s(L, ra, res);
        }
        else {
          TValue g;
          sethvalue(L, &g, h);
          Protect(luaV_gettable_tm(L, &g, rb, ra, tm));
        }
        vmbreak;
      }
      vmcase(OP_GETTABLE) {
        TValue *rb = RB(i);
        TValue *rc = RKC(i);
        if (ttistable(rb)) {
          Table *h = hvalue(rb);
          const TValue *res = luaH_get_inl(h, rc);
          const TValue *tm;
          if (!ttisnil(res) || (tm = fasttm(L, h->metatable, TM_INDEX)) == NULL) {
            setobj2s(L, ra, res);
          }
          else Protect(luaV_gettable_tm(L, rb, rc, ra, tm));
        }
        else Protect(luaV_gettable(L, rb, rc, ra));
        vmbreak;
      }
      vmcase(OP_SETGLOBAL) {
        TValue *rb = KBx(i);
        Table *h = cl->env;
        TValue *slot;
        lua_assert(ttisstring(rb));
        slot = lua_cast(TValue *, luaH_getstr_inl(h, rawtsvalue(rb)));
        if (slot != luaO_nilobject) {  /* existing key: luaH_set's result */
          const TValue *tm;
          h->flags = 0;  /* as luaH_set */
          if (!ttisnil(slot) || (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) {
            setobj2t(L, slot, ra);
            h->flags = 0;
            luaC_barriert(L, h, ra);
            vmbreak;
          }
        }
        {
          TValue g;
          sethvalue(L, &g, h);
          Protect(luaV_settable(L, &g, rb, ra));
        }
        vmbreak;
      }
      vmcase(OP_SETUPVAL) {
        UpVal *uv = cl->upvals[GETARG_B(i)];
        setobj(L, uv->v, ra);
        luaC_barrier(L, uv, ra);
        vmbreak;
      }
      vmcase(OP_SETTABLE) {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttistable(ra)) {
          Table *h = hvalue(ra);
          TValue *slot = lua_cast(TValue *, luaH_get_inl(h, rb));
          if (slot != luaO_nilobject) {  /* existing key: luaH_set's result */
            const TValue *tm;
            h->flags = 0;  /* as luaH_set */
            if (!ttisnil(slot) || (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) {
              setobj2t(L, slot, rc);
              h->flags = 0;
              luaC_barriert(L, h, rc);
              vmbreak;
            }
          }
        }
        Protect(luaV_settable(L, ra, rb, rc));
        vmbreak;
      }
      vmcase(OP_NEWTABLE) {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        sethvalue(L, ra, luaH_new(L, luaO_fb2int(b), luaO_fb2int(c)));
        Protect(luaC_checkGC(L));
        vmbreak;
      }
      vmcase(OP_SELF) {
        StkId rb = RB(i);
        TValue *rc = RKC(i);
        setobjs2s(L, ra+1, rb);
        if (ttistable(rb)) {
          Table *h = hvalue(rb);
          const TValue *res = luaH_get_inl(h, rc);
          const TValue *tm;
          if (!ttisnil(res) || (tm = fasttm(L, h->metatable, TM_INDEX)) == NULL) {
            setobj2s(L, ra, res);
          }
          else Protect(luaV_gettable_tm(L, rb, rc, ra, tm));
        }
        else Protect(luaV_gettable(L, rb, rc, ra));
        vmbreak;
      }
      vmcase(OP_ADD) {
        arith_op(luai_numadd, TM_ADD);
        vmbreak;
      }
      vmcase(OP_SUB) {
        arith_op(luai_numsub, TM_SUB);
        vmbreak;
      }
      vmcase(OP_MUL) {
        arith_op(luai_nummul, TM_MUL);
        vmbreak;
      }
      vmcase(OP_DIV) {
        arith_op(luai_numdiv, TM_DIV);
        vmbreak;
      }
      vmcase(OP_MOD) {
        arith_op(luai_nummod, TM_MOD);
        vmbreak;
      }
      vmcase(OP_POW) {
        arith_op(luai_numpow, TM_POW);
        vmbreak;
      }
      vmcase(OP_UNM) {
        TValue *rb = RB(i);
        if (ttisnumber(rb)) {
          lua_Number nb = nvalue(rb);
          setnvalue(ra, luai_numunm(nb));
        }
        else {
          Protect(Arith(L, ra, rb, rb, TM_UNM));
        }
        vmbreak;
      }
      vmcase(OP_NOT) {
        int res = l_isfalse(RB(i));  /* next assignment may change this value */
        setbvalue(ra, res);
        vmbreak;
      }
      vmcase(OP_LEN) {
        const TValue *rb = RB(i);
        switch (ttype(rb)) {
          case LUA_TTABLE: {
            setnvalue(ra, cast_num(luaH_getn(hvalue(rb))));
            break;
          }
          case LUA_TSTRING: {
            setnvalue(ra, cast_num(tsvalue(rb)->len));
            break;
          }
          default: {  /* try metamethod */
            Protect(
              if (!call_binTM(L, rb, luaO_nilobject, ra, TM_LEN))
                luaG_typeerror(L, rb, "get length of");
            )
          }
        }
        vmbreak;
      }
      vmcase(OP_CONCAT) {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        Protect(luaV_concat(L, c-b+1, c); luaC_checkGC(L));
        setobjs2s(L, RA(i), base+b);
        vmbreak;
      }
      vmcase(OP_JMP) {
        dojump(L, pc, GETARG_sBx(i));
        vmbreak;
      }
      /*
      ** Recoil: EQ/LT/LE decide the cases that cannot involve metamethods
      ** inline, with the same expressions as luaV_equalval/luaV_lessthan/
      ** lessequal (different types are never equal; nil, booleans, numbers,
      ** light userdata and the pointer-compared types; number ordering).
      ** Everything else calls the generic functions as before.
      */
      vmcase(OP_EQ) {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        int res;
        if (ttype(rb) != ttype(rc))
          res = 0;
        else {
          switch (ttype(rb)) {
            case LUA_TNIL: res = 1; break;
            case LUA_TNUMBER: res = luai_numeq(nvalue(rb), nvalue(rc)); break;
            case LUA_TBOOLEAN: res = bvalue(rb) == bvalue(rc); break;
            case LUA_TLIGHTUSERDATA: res = pvalue(rb) == pvalue(rc); break;
            case LUA_TUSERDATA:
            case LUA_TTABLE: {
              if (gcvalue(rb) == gcvalue(rc)) { res = 1; break; }
              Protect(res = luaV_equalval(L, rb, rc));  /* may try __eq */
              break;
            }
            default: res = gcvalue(rb) == gcvalue(rc); break;
          }
        }
        if (res == GETARG_A(i))
          dojump(L, pc, GETARG_sBx(*pc));
        pc++;
        vmbreak;
      }
      vmcase(OP_LT) {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          if (luai_numlt(nvalue(rb), nvalue(rc)) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        }
        else Protect(
          if (luaV_lessthan(L, rb, rc) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        )
        pc++;
        vmbreak;
      }
      vmcase(OP_LE) {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          if (luai_numle(nvalue(rb), nvalue(rc)) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        }
        else Protect(
          if (lessequal(L, rb, rc) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        )
        pc++;
        vmbreak;
      }
      vmcase(OP_TEST) {
        if (l_isfalse(ra) != GETARG_C(i))
          dojump(L, pc, GETARG_sBx(*pc));
        pc++;
        vmbreak;
      }
      vmcase(OP_TESTSET) {
        TValue *rb = RB(i);
        if (l_isfalse(rb) != GETARG_C(i)) {
          setobjs2s(L, ra, rb);
          dojump(L, pc, GETARG_sBx(*pc));
        }
        pc++;
        vmbreak;
      }
      vmcase(OP_CALL) {
        int b = GETARG_B(i);
        int nresults = GETARG_C(i) - 1;
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->savedpc = pc;
        /*
        ** Recoil: luaD_precall (and for C functions luaD_poscall) inline
        ** for the common cases: a function value, no call hook, and enough
        ** stack and CallInfo space so that luaD_precall would not reallocate
        ** anything (luaD_checkstack/inc_ci). Same steps in the same order;
        ** everything else (vararg functions, __call, hooks, growth) goes
        ** through luaD_precall.
        */
        if (ttisfunction(ra) && !(L->hookmask & LUA_MASKCALL) &&
            L->ci != L->end_ci) {
          Closure *ncl = clvalue(ra);
          if (!ncl->c.isC) {  /* Lua function */
            Proto *p = ncl->l.p;
            if (!p->is_vararg && (char *)L->stack_last - (char *)L->top >
                                 p->maxstacksize*(int)sizeof(TValue)) {
              CallInfo *ci;
              StkId st, nbase = ra + 1;
              L->ci->savedpc = pc;
              if (L->top > nbase + p->numparams)
                L->top = nbase + p->numparams;
              ci = ++L->ci;  /* now `enter' new function */
              ci->func = ra;
              L->base = ci->base = nbase;
              ci->top = nbase + p->maxstacksize;
              L->savedpc = p->code;  /* starting point */
              ci->tailcalls = 0;
              ci->nresults = nresults;
              for (st = L->top; st < ci->top; st++)
                setnilvalue(st);
              L->top = ci->top;
              nexeccalls++;
              goto reentry;  /* restart luaV_execute over new Lua function */
            }
          }
          else if ((char *)L->stack_last - (char *)L->top >
                   LUA_MINSTACK*(int)sizeof(TValue)) {  /* C function */
            if (precall_C(L, ra, ncl->c.f, nresults) < 0)
              return;  /* yield */
            /* adjust results */
            if (nresults >= 0) L->top = L->ci->top;
            base = L->base;
            vmbreak;
          }
        }
        switch (luaD_precall(L, ra, nresults)) {
          case PCRLUA: {
            nexeccalls++;
            goto reentry;  /* restart luaV_execute over new Lua function */
          }
          case PCRC: {
            /* it was a C function (`precall' called it); adjust results */
            if (nresults >= 0) L->top = L->ci->top;
            base = L->base;
            vmbreak;
          }
          default: {
            return;  /* yield */
          }
        }
      }
      vmcase(OP_TAILCALL) {
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->savedpc = pc;
        lua_assert(GETARG_C(i) - 1 == LUA_MULTRET);
        switch (luaD_precall(L, ra, LUA_MULTRET)) {
          case PCRLUA: {
            /* tail call: put new frame in place of previous one */
            CallInfo *ci = L->ci - 1;  /* previous frame */
            int aux;
            StkId func = ci->func;
            StkId pfunc = (ci+1)->func;  /* previous function index */
            if (L->openupval) luaF_close(L, ci->base);
            L->base = ci->base = ci->func + ((ci+1)->base - pfunc);
            for (aux = 0; pfunc+aux < L->top; aux++)  /* move frame down */
              setobjs2s(L, func+aux, pfunc+aux);
            ci->top = L->top = func+aux;  /* correct top */
            lua_assert(L->top == L->base + clvalue(func)->l.p->maxstacksize);
            ci->savedpc = L->savedpc;
            ci->tailcalls++;  /* one more call lost */
            L->ci--;  /* remove new frame */
            goto reentry;
          }
          case PCRC: {  /* it was a C function (`precall' called it) */
            base = L->base;
            vmbreak;
          }
          default: {
            return;  /* yield */
          }
        }
      }
      vmcase(OP_RETURN) {
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b-1;
        if (L->openupval) luaF_close(L, base);
        L->savedpc = pc;
        b = (L->hookmask & LUA_MASKRET) ? luaD_poscall(L, ra) :
                                          poscall_nohook(L, ra);
        if (--nexeccalls == 0)  /* was previous function running `here'? */
          return;  /* no: return */
        else {  /* yes: continue its execution */
          if (b) L->top = L->ci->top;
          lua_assert(isLua(L->ci));
          lua_assert(GET_OPCODE(*((L->ci)->savedpc - 1)) == OP_CALL);
          goto reentry;
        }
      }
      vmcase(OP_FORLOOP) {
        lua_Number step = nvalue(ra+2);
        lua_Number idx = luai_numadd(nvalue(ra), step); /* increment index */
        lua_Number limit = nvalue(ra+1);
        if (luai_numlt(0, step) ? luai_numle(idx, limit)
                                : luai_numle(limit, idx)) {
          dojump(L, pc, GETARG_sBx(i));  /* jump back */
          setnvalue(ra, idx);  /* update internal index... */
          setnvalue(ra+3, idx);  /* ...and external index */
        }
        vmbreak;
      }
      vmcase(OP_FORPREP) {
        const TValue *init = ra;
        const TValue *plimit = ra+1;
        const TValue *pstep = ra+2;
        L->savedpc = pc;  /* next steps may throw errors */
        if (!tonumber(init, ra))
          luaG_runerror(L, LUA_QL("for") " initial value must be a number");
        else if (!tonumber(plimit, ra+1))
          luaG_runerror(L, LUA_QL("for") " limit must be a number");
        else if (!tonumber(pstep, ra+2))
          luaG_runerror(L, LUA_QL("for") " step must be a number");
        setnvalue(ra, luai_numsub(nvalue(ra), nvalue(pstep)));
        dojump(L, pc, GETARG_sBx(i));
        vmbreak;
      }
      vmcase(OP_TFORLOOP) {
        StkId cb = ra + 3;  /* call base */
        setobjs2s(L, cb+2, ra+2);
        setobjs2s(L, cb+1, ra+1);
        setobjs2s(L, cb, ra);
        L->top = cb+3;  /* func. + 2 args (state and index) */
        Protect(vm_call(L, cb, GETARG_C(i)));
        L->top = L->ci->top;
        cb = RA(i) + 3;  /* previous call may change the stack */
        if (!ttisnil(cb)) {  /* continue loop? */
          setobjs2s(L, cb-1, cb);  /* save control variable */
          dojump(L, pc, GETARG_sBx(*pc));  /* jump back */
        }
        pc++;
        vmbreak;
      }
      vmcase(OP_SETLIST) {
        int n = GETARG_B(i);
        int c = GETARG_C(i);
        int last;
        Table *h;
        if (n == 0) {
          n = cast_int(L->top - ra) - 1;
          L->top = L->ci->top;
        }
        if (c == 0) c = cast_int(*pc++);
        runtime_check(L, ttistable(ra));
        h = hvalue(ra);
        last = ((c-1)*LFIELDS_PER_FLUSH) + n;
        if (last > h->sizearray)  /* needs more space? */
          luaH_resizearray(L, h, last);  /* pre-alloc it at once */
        for (; n > 0; n--) {
          TValue *val = ra+n;
          setobj2t(L, luaH_setnum(L, h, last--), val);
          luaC_barriert(L, h, val);
        }
        vmbreak;
      }
      vmcase(OP_CLOSE) {
        luaF_close(L, ra);
        vmbreak;
      }
      vmcase(OP_CLOSURE) {
        Proto *p;
        Closure *ncl;
        int nup, j;
        p = cl->p->p[GETARG_Bx(i)];
        nup = p->nups;
        ncl = luaF_newLclosure(L, nup, cl->env);
        ncl->l.p = p;
        for (j=0; j<nup; j++, pc++) {
          if (GET_OPCODE(*pc) == OP_GETUPVAL)
            ncl->l.upvals[j] = cl->upvals[GETARG_B(*pc)];
          else {
            lua_assert(GET_OPCODE(*pc) == OP_MOVE);
            ncl->l.upvals[j] = luaF_findupval(L, base + GETARG_B(*pc));
          }
        }
        setclvalue(L, ra, ncl);
        Protect(luaC_checkGC(L));
        vmbreak;
      }
      vmcase(OP_VARARG) {
        int b = GETARG_B(i) - 1;
        int j;
        CallInfo *ci = L->ci;
        int n = cast_int(ci->base - ci->func) - cl->p->numparams - 1;
        if (b == LUA_MULTRET) {
          Protect(luaD_checkstack(L, n));
          ra = RA(i);  /* previous call may change the stack */
          b = n;
          L->top = ra + n;
        }
        for (j = 0; j < b; j++) {
          if (j < n) {
            setobjs2s(L, ra + j, ci->base - n + j);
          }
          else {
            setnilvalue(ra + j);
          }
        }
        vmbreak;
      }
#if LUAV_COMPUTED_GOTO
      L_unknown: vmbreak;  /* unused opcode */
#endif
    }
  }
}

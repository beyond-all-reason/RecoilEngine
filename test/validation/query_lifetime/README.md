# OpenGL query lifetime regression

Query handles are now opaque full userdata rather than lightuserdata pointers to
unrooted memory. Unreachable handles release their GL resource through `__gc`;
explicit deletion remains optional and idempotent. Running or reading a deleted
handle remains a no-op or returns nil. Deleting or reading the currently active
query raises an error. Callers should retain the returned handle, not depend on
its former pointer representation.

Run with a graphical engine, BAR checkout and development assets:

```sh
python3 test/validation/query_lifetime/run.py build-amd64-linux/spring \
  --bar /path/to/Beyond-All-Reason --assets /path/to/bar-dev/data \
  --results /tmp/query-fixed-results
```

Add `--baseline` with an unmodified engine to safely reproduce double deletion
invalidating another live query and the non-owning lightuserdata handle. The
baseline stops GC until all tracked queries are deleted; it does not deliberately
dereference freed query memory or reproduce a native crash.

The fixed run covers retained handles across GC, collectible handles, independent
deletion, invalid inputs, callback arguments/errors, nested queries, active-query
deletion/read rejection, callback-induced map growth and LuaUI shutdown. When
unsafe debug metatable access is available, it also checks finalizers, unrelated
userdata forged as queries and shutdown cleanup after finalizer removal. Those
optional checks are skipped when the independent debug hardening fix is present.

The driver links development assets into an isolated temporary overlay, disables
unrelated LuaRules/LuaGaia code, preserves logs and removes the overlay afterward.
It requires occlusion-query support and is not an ASan test.

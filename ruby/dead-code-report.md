# Dead Code Report — Ice for Ruby (`ruby/`)

Audit of dead and potentially-dead code in the Ice for Ruby language mapping. Like Python, Ruby is a **two-layer** mapping: a C++ extension (`ruby/src/IceRuby/`, ~8.9K lines) that bridges to the Ice C++ runtime, and a hand-written pure-Ruby layer (`ruby/ruby/Ice/`, 12 files, ~700 lines) that decorates it. Both layers were audited with the same methodology as the Python report.

All permalinks are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487).

---

## Summary

| Category | Count | Removable? |
|---|---|---|
| **A. Truly dead** (no caller anywhere) | **1** | Yes |
| **B. Public-API, unused in-repo** | ~15 | Review-only (it's API) |
| **C. Conditional / obsolete-platform code** | 0 | — |

Ruby is the cleanest mapping audited so far. The single removable item is one orphaned C++ type-predicate helper. The pure-Ruby layer has **no dead code at all**. Everything else flagged is legitimate public Ice API that the (small) Ruby test suite simply doesn't exercise.

> **Ruby is a client-only mapping.** Unlike C++/C#/Java/Python, IceRuby exposes no `ObjectAdapter`, servants, or server-side dispatch — only proxies, communicators, connections, properties, and value unmarshaling. Consequently the cross-language "unused `ObjectAdapter.findByProxy`" finding **does not apply** to Ruby (there is no such method), and the entire server-side surface is simply absent rather than dead.

---

## Scope & method

Identical technique to the Python audit (see `python/dead-code-report.md`); only the bridge specifics differ (Ruby C-API instead of CPython).

**C++ extension (`ruby/src/IceRuby/`).** Built with `-Wall -Wextra -Wconversion …` and **0 compiler warnings** (the single build warning is a benign linker note: macOS version skew against `libruby.3.4.dylib`). Because `-Wall` includes `-Wunused-function`, unused file-local (`static`) functions — which is what the Ruby method implementations are — are **provably nil**. That leaves two surfaces the compiler cannot prove:

1. **Exported (`extern`) helper functions** declared in IceRuby headers. Enumerated all 37 `IceRuby::` free functions and counted call sites across **both `.cpp` and `.h`** (the `callRuby` machinery is header-inline templates, so headers must be included — a `.cpp`-only search falsely clears them). → Category A.
2. **Ruby-facing methods** registered via `rb_define_method` / `rb_define_module_function` (117 names). Cross-referenced against the Ruby caller corpus. → Category B.

A symbol-table (`nm`) pass over the 26 objects was used only as an overload cross-check; as in Python it is **not authoritative** here (IceRuby is one-`.cpp`-per-class, so intra-TU calls are invisible to `nm`, and `-O2` inlining erases cross-TU references). Source-level reachability is the authority.

**Pure-Ruby layer (`ruby/ruby/Ice/`).** No compiler dead-code detection for Ruby — static reachability of every `def` (60 methods across 12 files) against the full corpus. Small enough to verify exhaustively by hand.

### Caller corpus

- The 12 hand-written `Ice/*.rb` files + `Ice.rb` loader + the `Glacier2`/`IceBox`/`IceGrid`/`IceStorm` loader stubs.
- **Generated** Slice→Ruby code: the generated `Ice/*.rb` (e.g. `Locator.rb`, `BuiltinSequences.rb`, `Metrics.rb`) and the generated `Glacier2`/`IceBox`/`IceGrid`/`IceStorm` packages.
- Tests under `ruby/test/`.
- The `scripts/slice2rb` launcher (calls `Ice.compile`).
- The IceRuby C++ extension itself (calls back into Ruby by name — see keepers).

> **Tooling caveat (the Java/Python trap):** the generated corpus is **git-ignored** (confirmed via `git check-ignore`). All searches used `command grep` (git-ignore-blind), never bare `ripgrep`/`git grep`.

### Live-by-reflection keepers (considered, excluded)

The C++ extension drives the Ruby layer by *name*, so these symbols are live despite having no static Ruby caller:

- **Local exceptions** — `IceRuby::convertException` ([`Util.cpp:474`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Util.cpp#L474)) constructs the matching Ruby exception class by name via `callRuby(rb_path2class, className)` ([`Util.cpp:456`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Util.cpp#L456)), falling back to `Ice::LocalException`. The whole `LocalExceptions.rb` hierarchy (and its constructors) is therefore live.
- **Classes looked up via `rb_path2class`** — `Ice::Identity`, `Ice::EncodingVersion`, `Ice::CompressBatch`, `Ice::EndpointSelectionType`, `Ice::InitializationData`, `Ice::LocatorPrx`, `Ice::RouterPrx`, `Ice::SlicedData`, `Ice::SliceInfo`.
- **Methods invoked from C++** (`callRuby`/`rb_funcall` + `rb_intern`) — `ice_preMarshal`, `ice_postUnmarshal`, `ice_id`, `from_int`, `each`, `newInstance`, `_enumerators`, `to_i`, `to_s`, `inspect`, `to_hash`, `to_ary`, `to_str`.
- **Generated-code-facing module functions** — `Ice::__defineEnum`, `__defineException`, `__defineStruct`, `__defineSequence`, `__defineDictionary`, `__defineOperation`, `__declareClass`, `__declareProxy`, `__stringify*` — all called from generated Slice→Ruby code (verified, e.g. 7× `Ice::__defineException`).
- **Ruby protocol methods** — `to_s`, `hash`, `<=>`, `eql?`, `inspect`, `each` are invoked implicitly by the interpreter / `Comparable` / `Hash`.

---

## Category A — Truly dead (removable)

### C++ extension — exported helper declared in an internal header, never called

| Function | Definition | Declaration |
|---|---|---|
| `IceRuby::isHash(VALUE)` → `bool` | [`Util.cpp:133`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Util.cpp#L133) | [`Util.h:57`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Util.h#L57) |

A type-predicate (is this Ruby `VALUE` a Hash?) with exactly two occurrences in the entire IceRuby tree — its definition and its declaration — and no call site or address-of anywhere. It escaped `-Wunused-function` only because it is `extern` (header-declared), not `static`. It is the **orphan of its family**: the sibling predicates `isString` and `isArray` are both used (4 occurrences each = def + decl + call sites); `isHash` is not.

### Pure Ruby

**None.** Every `def` in the 12 hand-written files is reachable — via an in-repo Ruby caller, a C++ reflection keeper, or the Ruby object protocol. (The `proxyIdentityCompare`/`proxyIdentityAndFacetCompare` helpers are called by their `…Equal` siblings, which the tests use; the enum `_enumerators` accessors back `from_int`/`each`; `getSliceDir` has 6 callers.)

---

## Category B — Public API / standard-interface, unused in-repo (review-only)

The C++ extension exposes the usual public Ice API, but the Ruby test suite is small and exercises only part of it. These methods have **zero** in-repo callers yet are legitimate public API (cannot be removed). Listed compactly; representative permalinks given.

| Symbol | Registration | Note |
|---|---|---|
| `Communicator#getLogger` | [`Communicator.cpp:542`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Communicator.cpp#L542) | Public; not used in any test. |
| `Connection#abort` / `#throwException` / `#toString` | [`Connection.cpp:321`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Connection.cpp#L321), [`:333`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Connection.cpp#L333), [`:334`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Connection.cpp#L334) | `throwException` is unused across Python too; `toString` is redundant with Ruby `to_s`. |
| `Endpoint#toString` | [`Endpoint.cpp:250`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Endpoint.cpp#L250) | Redundant with `to_s`. |
| `Logger#cloneWithPrefix`, `#print`, `#trace`, `#warning`, `#error`, `#getPrefix` | [`Logger.cpp:138`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Logger.cpp#L138) | Default-logger methods; no test drives the logger directly. |
| `Ice.getProcessLogger`, `Ice.stringVersion`, `Ice.intVersion` | [`Logger.cpp:143`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Logger.cpp#L143), [`Util.cpp:89`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/src/IceRuby/Util.cpp#L89) | Module functions; not called in-repo. |
| `Properties#getPropertyAsList`, proxy `#ice_getCachedConnection` | `Properties.cpp` / `Proxy.cpp` tables | Public API; not exercised. |

These are review-only and individually low-value. The cross-language-consistent ones are `throwException` and `toString` (both also unused-in-repo in Python).

---

## Category C — Conditional / obsolete-platform code

**None.** IceRuby contains only **active** conditional compilation, all guarding current Ruby C-API differences:

- `#ifdef RUBY_BLOCK_CALL_FUNC_TAKES_BLOCKARG` and `#ifdef RB_BLOCK_CALL_FUNC_STRICT` — handle block-call function-pointer signature differences across supported Ruby versions.

The only platform branch in the hand-written Ruby is `RUBY_PLATFORM =~ /linux/i` in `getSliceDir` ([`SliceUtil.rb:30`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/ruby/ruby/Ice/SliceUtil.rb#L30)) — an active default-install-path lookup for a supported platform, not dead. No AIX/Solaris/old-compiler residue.

---

## Confidence & limitations

- **High confidence** on Category A. `isHash` was verified to have exactly one definition + one declaration and no other occurrence (call or address-of) in the IceRuby tree.
- **The pure-Ruby layer is provably clean** — at ~700 lines across 12 files, every `def` was individually cross-referenced; all are live.
- **Known limitation** (same as Python): internal C++ *member* functions used only within their defining `.cpp` were checked via the `nm` cross-check and the Ruby-facing table sweep, not an exhaustive per-class source audit. All inspected `nm` candidates proved to be intra-TU-used false positives.
- All searches used git-ignore-blind `command grep`; low-count call sites were opened and read to rule out same-name collisions, and the `.cpp`-vs-`.h` distinction (which initially mis-cleared the header-inline `callProtected`) was corrected by searching both.

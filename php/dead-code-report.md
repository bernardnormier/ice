# Dead Code Report — Ice for PHP (`php/`)

Audit of dead and potentially-dead code in the Ice for PHP language mapping. Like Python and Ruby, PHP is a **two-layer** mapping: a Zend C++ extension (`php/src/`, ~12K lines) that bridges to the Ice C++ runtime, and a hand-written pure-PHP layer (`php/lib/Ice/`, ~370 lines) that decorates it. Both layers were audited with the methodology established in the Python/Ruby reports.

All permalinks are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487).

---

## Summary

| Category | Count | Removable? |
|---|---|---|
| **A. Truly dead** (no caller anywhere) | **2** | Yes |
| **B. Public-API, unused in-repo** | ~24 | Review-only (it's API) |
| **C. Conditional / obsolete-platform code** | 0 | — |

Both removable items are in the C++ extension: one stale function prototype and one unused RAII helper method. The pure-PHP layer has **no removable dead code** — its only flagged symbol is a public API function. Everything else is legitimate public Ice API that the PHP test suite doesn't exercise.

> **PHP is a client-only mapping**, like Ruby. IcePHP exposes no `ObjectAdapter`, servants, or server-side dispatch — only proxies, communicators, connections, properties, and value unmarshaling. The cross-language "unused `ObjectAdapter.findByProxy`" finding therefore **does not apply** to PHP.

---

## Scope & method

Same technique as the Python/Ruby audits; only the bridge specifics differ (Zend/PHP C-API).

**C++ extension (`php/src/`).** Built with `-Wall -Wextra -Wconversion …` and **0 compiler warnings** ⇒ unused file-local (`static`) functions are provably nil. Two surfaces the compiler cannot prove were checked at the source level:

1. **Exported `IcePHP::` helper functions** — all 23 free functions (from the object symbol table) were source-counted across `.cpp`+`.h`; **all 23 are used**.
2. **Every function/method declared in an IcePHP header** (free *or* member) — a second pass counted source occurrences of each header-declared `name(`, flagging any with only a definition+declaration and no call site. This is the pass that found both Category A items (the free-function pass alone misses them: one has no definition, the other is a member function). Each candidate was opened and verified.

A symbol-table (`nm`) pass over the 21 objects was used only as an overload cross-check; as in Python/Ruby it is not authoritative (intra-TU calls invisible to `nm`; `-O2` inlining erases cross-TU references).

**PHP-facing surface.** All 95 `ZEND_ME` methods + 12 `ZEND_FE` functions were cross-referenced against the PHP caller corpus → Category B.

**Pure-PHP layer (`php/lib/Ice/`).** Static reachability of every `function`/`class` (~30 symbols across 9 files) against the full corpus — small enough to verify by hand.

### Caller corpus

- Hand-written `lib/Ice.php` + `lib/Ice/*.php` + the `Glacier2`/`IceBox`/`IceGrid`/`IceStorm` loader stubs.
- **Generated** Slice→PHP code: generated `lib/Ice/*.php` (e.g. `Locator.php`, `BuiltinSequences.php`) and the generated package dirs.
- Tests under `php/test/`.
- The IcePHP extension itself (calls back into PHP by name — see keepers).

> **Tooling caveat:** the generated corpus is **git-ignored** (confirmed via `git check-ignore`). All searches used git-ignore-blind `command grep`. (A BSD-`sed` `\s` quirk initially corrupted the method-name list with leading spaces and zeroed every match — caught and corrected; the corrected sweep finds e.g. `ice_oneway` in tests as expected.)

### Live-by-reflection keepers (considered, excluded)

- **Local exceptions** — `IcePHP::convertException` ([`Util.cpp:417`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Util.cpp#L417)) builds the matching PHP exception class by name via `nameToClass` ([`Util.cpp:177`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Util.cpp#L177) → `zend_lookup_class`), falling back to `\Ice\LocalException`. The whole `LocalExceptions.php` hierarchy is therefore live.
- **Classes looked up via `nameToClass`** — `\Ice\Identity`, `\Ice\EncodingVersion`, `\Ice\InitializationData`, `\Ice\SlicedData`, `\Ice\SliceInfo`, `\Ice\LocalException`.
- **Generated-code-facing free functions** — `IcePHP_defineClass`, `IcePHP_defineEnum`, `IcePHP_defineException`, `IcePHP_defineStruct`, `IcePHP_defineSequence`, `IcePHP_defineDictionary`, `IcePHP_defineProxy`, `IcePHP_defineOperation`, `IcePHP_declareClass`, `IcePHP_declareProxy`, `IcePHP_stringify*` — all called from generated Slice→PHP code.
- **Lifecycle/constructor calls** — `invokeMethod` ([`Util.cpp:628`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Util.cpp#L628)) invokes `__construct` (value/exception construction); value `ice_id` is consumed during marshaling. `DefaultSliceLoader::newClassInstance`/`newExceptionInstance` are virtual overrides called by the Ice runtime.

---

## Category A — Truly dead (removable)

Both are in the C++ extension and were found by the header-declaration scan.

| Symbol | Location | Evidence |
|---|---|---|
| `IcePHP::createWrapper(zend_class_entry*, size_t)` | [`Util.h:17`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Util.h#L17) | **Stale prototype.** Sole occurrence in the entire repository (php/ *and* cpp/) — declared, never defined, never called. A vestige of removed functionality. |
| `IcePHP::AutoDestroy::release()` | [`Util.h:128`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Util.h#L128) | RAII helper method (releases ownership of the wrapped `zval`). Has a body but **zero callers** — no `.release()`/`->release()` anywhere. The enclosing `AutoDestroy` class is used for scope-guarding, but only its constructor/destructor; the ownership-release escape hatch is never needed. |

### Pure PHP

**None.** Every `function`/`class`/method in the 9 hand-written files is reachable — via an in-repo caller, a C++ reflection keeper, or the PHP object protocol. (`ObjectPrxHelper::createProxy`/`checkedCast`/`uncheckedCast` have 15–25 callers; `proxyIdentityCompare` is reached through the test-exercised `proxyIdentityAndFacetEqual`; the enum classes are constant-only.)

---

## Category B — Public API / standard-interface, unused in-repo (review-only)

The extension exposes the usual public Ice API; the PHP test suite exercises only part of it. These have **zero** in-repo callers yet are legitimate public API. Representative permalinks:

| Symbol | Registration | Note |
|---|---|---|
| `Connection::abort` / `disableInactivityCheck` / `throwException` | [`Connection.cpp:317`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Connection.cpp#L317), [`:321`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Connection.cpp#L321), [`:336`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Connection.cpp#L336) | `disableInactivityCheck` + `throwException` are unused across Python too. |
| `Communicator::getLogger`, `getDefaultLocator`/`Router`, `setDefaultLocator`/`Router`, `getImplicitContext` | [`Communicator.cpp:1222`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Communicator.cpp#L1222) | Public; not used in any test. |
| `Logger::cloneWithPrefix`, `print`, `trace`, `warning`, `error` | [`Logger.cpp:235`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/src/Logger.cpp#L235) | No test drives the logger directly. |
| `Properties::getCommandLineOptions`, `getPropertiesForPrefix`, `getPropertyAsList` | `Properties.cpp` table | Public; not exercised. |
| proxy `ice_endpoints`, `ice_getCachedConnection`; `Connection::close`, `clone` | tables | Public; not exercised. |
| **pure PHP:** `Ice\proxyIdentityEqual($lhs, $rhs)` | [`Proxy.php:59`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/php/lib/Ice/Proxy.php#L59) | Public namespace function; 0 callers (its sibling `proxyIdentityAndFacetEqual` *is* used by tests). Keep for API symmetry. |

`__toString` is registered but invoked implicitly by PHP string conversion — treated as live.

---

## Category C — Conditional / obsolete-platform code

**None.** IcePHP contains only **active** conditional compilation, all guarding the Zend/PHP C-API or build configuration:

- `#ifdef HT_ALLOW_COW_VIOLATION` — Zend hash copy-on-write API.
- `#if defined(__clang__) …` — compiler-warning guards.
- `#ifdef NDEBUG` / `#ifdef DEBUG` — debug/release.

No AIX/Solaris/old-compiler residue. The hand-written PHP contains no `PHP_OS`/platform branching.

---

## Confidence & limitations

- **High confidence** on Category A. `createWrapper` has a single occurrence in the whole tree; `AutoDestroy::release()` has a body and zero call sites (verified with `.release()`/`->release()` searches).
- **The pure-PHP layer is provably clean** — ~30 symbols across 9 files, each cross-referenced; the only unused one (`proxyIdentityEqual`) is public API.
- **Known limitation:** the header-declaration scan flags functions with no call site; a dead member function whose *name* collides with a used same-named method on another class could be masked (count > 2). This residual collision risk is the same one noted in the Python/Ruby reports; the high-value findings are captured.
- All searches used git-ignore-blind `command grep`; low-count call sites were opened and read, and the BSD-`sed` `\s` name-extraction bug was caught and corrected before drawing conclusions.

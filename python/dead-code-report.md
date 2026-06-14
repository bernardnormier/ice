# Dead Code Report — Ice for Python (`python/`)

Audit of dead and potentially-dead code in the Ice for Python language mapping. Like Swift, Python is a **two-layer** mapping: a C++ extension (`python/modules/IcePy/`, ~18.3K lines) that bridges to the Ice C++ runtime, and a hand-written pure-Python layer (`python/python/Ice/`, 39 files, ~5.7K lines) that wraps it. Both layers were audited.

All permalinks are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487).

---

## Summary

| Category | Count | Removable? |
|---|---|---|
| **A. Truly dead** (no caller anywhere) | **4** | Yes |
| **B. Public-API / standard-interface, unused in-repo** | 11 | Review-only (it's API) |
| **C. Conditional / obsolete-platform code** | 0 | — |

The Python mapping is clean. The four removable items are three orphaned C++ helper functions exported from internal IcePy headers, plus one redundant internal Python accessor. Everything else flagged is legitimate public Ice API that the repo's own code and tests simply don't exercise.

---

## Scope & method

### Two layers, two techniques

**C++ extension (`python/modules/IcePy/`).** Built clean with `-Wall -Wextra -Wconversion …` and **0 warnings**. Because `-Wall` includes `-Wunused-function`, **unused file-local (`static`) functions are provably nil** — the same compiler-as-prover argument used for the C# analyzers. That leaves two surfaces the compiler can *not* prove:

1. **Exported (`extern`) helper functions** declared in IcePy headers — the compiler assumes another translation unit might call them, so it never warns. Found by enumerating every IcePy free/`extern "C"` function (71 of them) and counting call sites in the source. → Category A.
2. **Python-facing methods** registered in `PyMethodDef`/`PyGetSetDef` tables (210 names) — live only if some Python code calls them. Cross-referenced against the Python caller corpus. → Category B.

A symbol-table (`nm`) cross-TU pass was run as a cross-check (it is the one tool immune to source-grep *overload masking* — e.g. it correctly distinguished the live `callMethod(obj, "name", …)` overload from its sibling). But `nm` proved **too imprecise to be authoritative here**: IcePy is organized as "one header declares a class, one `.cpp` implements *and* uses it," so intra-TU calls (invisible to `nm`) dominate, and at `-O2` inlining erases the cross-TU `U` references entirely (a debug `OPTIMIZE=no` rebuild confirmed this). Source-level reachability is the authoritative tool; `nm` only seeded candidates.

**Pure-Python layer (`python/python/Ice/`).** No compiler dead-code detection exists for Python (as with Java/Swift), so this is static reachability: every class, function, and method defined in the 39 git-tracked files was cross-referenced against the full caller corpus. Four parallel agents swept thematic slices; all findings were re-verified by hand.

### Caller corpus (what counts as a caller)

- The 39 hand-written `Ice/` modules (subjects *and* callers of each other).
- **Generated** Slice→Python code: ~258 generated `Ice/*.py` files plus the generated `Glacier2`, `IceBox`, `IceGrid`, `IceMX`, `IceStorm` packages (produced by `slice2py` at build time).
- Tests + generated test code under `python/test/` (803 generated `.py` files).
- The `slice2py` launcher `python/dist/lib/slice2py.py`.
- The IcePy C++ extension itself (it calls *back* into Python by name — see keepers).

> **Tooling caveat (the trap from the Java audit):** the entire generated corpus (~258 + 803 files) is **git-ignored**. `ripgrep`/`git grep` skip git-ignored files by default and would have reported almost the whole runtime as "dead." All searches used `command grep -rn … --include='*.py'` (BSD grep ignores `.gitignore`); each was sanity-checked to confirm generated files were visible.

### Live-by-reflection keepers (considered, excluded)

The C++ extension drives much of the Python layer by *name*, so many symbols with zero static Python callers are nonetheless live. These were mapped and excluded:

- **Local exceptions** — `IcePy::convertException` ([`Util.cpp:568`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/Util.cpp#L568)) catches every C++ Ice exception and constructs the matching Python class by converting its type-ID `::Ice::Foo` → `Ice.Foo` and instantiating it with specific args. **Every class in `LocalExceptions.py` / `LocalException.py` is therefore live**, as are their constructor parameters and fields.
- **Types instantiated via `lookupType`** — `Ice.Object`, `Ice.Value`, `Ice.Current`, `Ice.Identity`, `Ice.Logger`, `Ice.OperationMode`, `Ice.ServantLocator`, `Ice.UserException`, `Ice.RouterPrx`, `Ice.LocatorPrx`, `Ice.ObjectAdapter`, `Ice.Future`, `Ice.CompressBatch`, `Ice.EncodingVersion`, `Ice.SlicedData`, `Ice.SliceInfo`, `Ice._ArrayUtil.createArray/createNumPyArray`.
- **Methods invoked from C++ by name** — `ice_preMarshal`, `ice_postUnmarshal`, `ice_id`, `locate`, `finished`, `deactivate`, `cloneWithPrefix`, `error`, `warning`, `trace`, `print`, `getPrefix`, `set_result`, `set_exception`, `set_sent`, `wrapFuture`; servant dispatch resolves operations as `_op_<name>` (`Operation.cpp`); `Blobject.ice_invoke` and `Dispatch.dispatch` are called from C++.
- **Attributes read from C++** — `_impl`, `argv`, `value`.

---

## Category A — Truly dead (removable)

### C++ extension — exported functions declared in internal IcePy headers, never called

The direct analog of the C++ runtime report's "exported-in-internal-header but unused" finding. Each is declared in an IcePy header and defined in the corresponding `.cpp`, with **no call site, address-of, or table reference anywhere** in the IcePy sources. They escaped `-Wunused-function` only because they are `extern` (header-declared), not `static`.

| Function | Definition | Declaration |
|---|---|---|
| `IcePy::getConnectionInfo(PyObject*)` → `Ice::ConnectionInfoPtr` | [`ConnectionInfo.cpp:411`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/ConnectionInfo.cpp#L411) | [`ConnectionInfo.h:16`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/ConnectionInfo.h#L16) |
| `IcePy::getEndpointInfo(PyObject*)` → `Ice::EndpointInfoPtr` | [`EndpointInfo.cpp:409`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/EndpointInfo.cpp#L409) | [`EndpointInfo.h:16`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/EndpointInfo.h#L16) |
| `IcePy::getProxyCommunicator(PyObject*)` → `Ice::CommunicatorPtr` | [`Proxy.cpp:1551`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/Proxy.cpp#L1551) | [`Proxy.h:36`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/Proxy.h#L36) |

These look like utility accessors (extract the underlying C++ `ConnectionInfo` / `EndpointInfo` / `Communicator` from a Python wrapper object). Their "create" counterparts (`createConnectionInfo`, etc.) are heavily used; the reverse getters are not.

### Pure Python — redundant internal accessor

| Symbol | Location | Evidence |
|---|---|---|
| `Communicator._getImpl(self)` | [`Communicator.py:130`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/Communicator.py#L130) | Body is `return self._impl`. Zero callers in any `.py` or `.cpp`. Every other site reaches the impl directly as `self._impl`. Unlike the sibling `_getWrapper`/`_setWrapper` (used; called from C++) and unlike `Logger._print` (documented compat alias), it has no caller and no retention note. |

---

## Category B — Public API / standard-interface, unused in-repo (review-only)

These are genuine public Ice API or deliberate compatibility shims. They have no in-repo caller, but removing them would change the public surface, so they are review-only — listed for completeness and cross-language consistency.

### Pure-Python public methods with no in-repo caller

| Symbol | Location | Note |
|---|---|---|
| `ObjectAdapter.findByProxy` | [`ObjectAdapter.py:414`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/ObjectAdapter.py#L414) | **Unused across all five mappings** (C++, C#, Java, Swift, Python) — strongest cross-language signal. |
| `ObjectAdapter.findFacet` | [`ObjectAdapter.py:374`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/ObjectAdapter.py#L374) | Wrapper delegates to `self._impl.findFacet`; nothing calls the wrapper. |
| `ObjectAdapter.findAllFacets` | [`ObjectAdapter.py:398`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/ObjectAdapter.py#L398) | As above. |
| `ObjectAdapter.findServantLocator` | [`ObjectAdapter.py:475`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/ObjectAdapter.py#L475) | As above. (`find`, `findDefaultServant`, `findAdminFacet` *are* exercised by tests.) |
| `Future.running()` | [`Future.py:74`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/Future.py#L74) | Mirrors `concurrent.futures.Future.running()`; sibling predicates `cancelled()`/`done()` are used. Keep for stdlib parity. |
| `Logger._print()` | [`Logger.py:29`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/python/Ice/Logger.py#L29) | Self-documented as "an alias for `print`, provided for backwards compatibility." Intentional shim — keep. |

### C++ Python-facing methods/attributes exposed but not exercised in-repo

These IcePy types (`Connection`, `NativePropertiesAdmin`, the `*Info` classes) are re-exported directly as `Ice.*` with no pure-Python wrapper, so they are called by user code, not repo code. All are documented public API.

| Symbol | Registration | Note |
|---|---|---|
| `Connection.disableInactivityCheck` | [`Connection.cpp:756`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/Connection.cpp#L756) | Documented public `Connection` method; no in-repo call. |
| `Connection.throwException` | [`Connection.cpp:791`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/Connection.cpp#L791) | As above. |
| `NativePropertiesAdmin.removeUpdateCallback` | [`PropertiesAdmin.cpp:140`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/PropertiesAdmin.cpp#L140) | Paired with `addUpdateCallback` (which a test uses); the remover is not exercised. |
| Info data attributes: `UDPConnectionInfo.mcastAddress`/`mcastPort`, `OpaqueEndpointInfo.rawBytes`, `WSEndpointInfo.resource` | e.g. [`ConnectionInfo.cpp:227`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/python/modules/IcePy/ConnectionInfo.cpp#L227) | Introspection fields mirroring the C++ info structs; no in-repo reader. Part of the info API surface. |
| `Connection.toString` / `Endpoint.toString` | `Connection.cpp` / `Endpoint.cpp` tables | Redundant with Python `str()`; no `.toString()` call in-repo. |

---

## Category C — Conditional / obsolete-platform code

**None.** Unlike the C++ runtime (which carries AIX/Solaris/old-VC++ `#ifdef` blocks), the IcePy extension contains only **active** conditional compilation:

- `#if defined(__clang__) && defined(__has_warning)` and `#if defined(__GNUC__) && (__GNUC__ >= 8)` — current compiler-warning guards.
- `#ifdef WORDS_BIGENDIAN` — endianness (still relevant).
- `#if PY_VERSION_HEX …` / `#if defined(__GNUC__)` (Init.cpp visibility) — current Python/toolchain guards.

The hand-written Python layer contains no `sys.platform` / `os.name` branching at all.

---

## Cross-language signal

`findByProxy` (on `ObjectAdapter`) is now confirmed **unused in the repository across every language mapping** — C++, C#, Java, Swift, and Python (both the Python wrapper *and* the underlying IcePy C++ method). Each report independently flagged it. It is public API and cannot be removed unilaterally, but the consistency makes it the prime candidate for a coordinated cross-language deprecation.

The other recurring runtime-wide "unused public API" members (`getCommandLineOptions`, the `getProperty*List` family, etc.) are all present here too but **are** exercised by the Python test suite, so they are not flagged for Python.

---

## Confidence & limitations

- **High confidence** on Category A. The three C++ functions were verified to have exactly one definition + one header declaration and no other occurrence in the IcePy tree; `_getImpl` has zero occurrences beyond its definition across all `.py` and `.cpp`.
- **The pure-Python layer is essentially clean.** Across all 39 files, the only non-public, no-caller, no-keeper symbol is `Communicator._getImpl`. The local-exception hierarchy, value/servant lifecycle methods, futures, loggers, and enums are all reachable via the C++ reflection keepers documented above.
- **Known limitation:** internal C++ *member* functions (class methods used only within their defining `.cpp`) were checked via the `nm` cross-check and the Python-facing table sweep, but not via an exhaustive per-class source audit. The `nm` candidates that were inspected all proved to be intra-TU-used false positives, consistent with a well-maintained extension; a dead private member function declared in a header remains theoretically possible but none surfaced.
- All findings were produced with `command grep` (git-ignore-blind) and low-count call sites were opened and read to rule out same-name collisions across classes/overloads.

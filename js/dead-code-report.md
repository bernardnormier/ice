# Dead Code Report — Ice for JavaScript / TypeScript (`js/`)

Audit of dead and potentially-dead code in the Ice for JavaScript mapping. Unlike Python/Ruby/PHP, JavaScript is **not** a C++-extension mapping — it is a *pure* ESM JavaScript reimplementation of the Ice runtime (`js/packages/ice/src/`, 109 modules, ~18.5K lines), with hand-written TypeScript declarations (`.d.ts`) as the public API contract. There is no C++ layer, no `nm`, and no reflection-from-native keepers; the analysis is static reachability over the ESM module graph.

All permalinks are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487).

---

## Summary

| Category | Count | Removable? |
|---|---|---|
| **A. Truly dead** (no caller anywhere) | **13** | Yes |
| **B. Public-API, unused in-repo** | 12 | Review-only (it's API) |
| **C. Conditional / obsolete-platform code** | 0 | — |

This is the richest mapping for findings — not because the JS code is lower quality, but because TypeScript's **separate `.d.ts` public contract makes the public/internal boundary crisp**, surfacing internal class methods that are defined but never called. All dead code is method-level; the module and export graph is clean.

---

## Scope & method

### The boundary: `.d.ts` = public, no-`.d.ts` = internal

`Ice/index.js` imports every module (`import * as Ice_X from "./X.js"`) and spreads all of them into a single public `Ice` object (`export const Ice = { ...Ice_X, ... }`), so at runtime *everything* is reachable as `Ice.Foo`. The real public-API contract is therefore the hand-written **`.d.ts`** set: of 109 modules, **42 have a sibling `.d.ts` (public)** and **67 do not (internal plumbing)**. A symbol in a `.d.ts` is public API; a symbol in an internal module (no `.d.ts`) that nothing references is truly dead.

### Three corpora (ESM static reachability)

1. **Hand-written runtime** — `js/packages/ice/src` (all `.js`).
2. **Generated Slice→JS code** — produced with the prebuilt `cpp/bin/slice2js` into `/tmp/jsgen` (31 files). This is a critical caller: generated marshaling code imports runtime helpers (`Object`, `ObjectPrx`, `Operation`, `StreamHelpers`, `TypeRegistry`, `UserException`, `Value`, `DefaultSliceLoader`, `defineStruct`, `defineClass`, `HashUtil`…). Omitting it would falsely flag those as dead.
3. **Tests** — `js/packages/test` (71 `.ts` + 16 `.js`); where public API is exercised.

The module **import graph is clean**: every hand-written module is imported somewhere, except the three browser variants below (resolved at bundle time, not via static import). No whole-module dead code, and no unused named *exports* — every exported class/function is `new`'d, `extends`'d, imported, or used via the namespace. **The dead code is entirely at the method level.**

### Keepers (considered, excluded)

- **Generated-code callers** (`/tmp/jsgen`) — e.g. `Struct.defineStruct`, `DefaultSliceLoader.defineClass`, and `HashUtil` are dominated by generated callers.
- **Prototype extensions** — methods injected as `SomeClass.prototype.m = function…` in `*Extensions.js` files are real instance methods; live if called as `obj.m()`.
- **Polymorphic dispatch** — the transceiver/endpoint/request-handler interfaces (`read`/`write`/`initialize`/`close`/`type`/`sendAsyncRequest`/`getConnection`…) are invoked polymorphically by `ConnectionI`/the protocol engine; Value/Object/stream marshaling lifecycle methods (`_iceWrite`/`_iceRead`/`ice_id`) are called by generated code.
- **Browser/Node split** — `FileLogger.browser.js`, `TcpTransceiver.browser.js`, `TimerUtil.browser.js` are selected by the package.json `"browser"` field for browser builds. Live platform variants, not dead.

### Anti-false-positive note

JavaScript is dynamic, so method reachability requires care. The same **name-collision masking** that hid `defaultEncoding` in the C++ audit recurs here: a dead method's name collides with a heavily-used same-named member, so a blunt `.method` grep shows it "used." Three of the dead methods below (`ProtocolInstance.defaultEncoding/traceLevel/traceCategory`) were only found by disambiguating the receiver type. Searches used git-ignore-blind `command grep` (in-tree generated `.js` is git-ignored); low-count sites were opened and verified.

---

## Category A — Truly dead (internal, removable)

### `ProtocolInstance` — three dead getters (no `.d.ts`; `ProtocolInstance` is internal)

| Symbol | Location | Evidence |
|---|---|---|
| `defaultEncoding()` | [`ProtocolInstance.js:51`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ProtocolInstance.js#L51) | **Same dead method found in C#, Java, and Python.** Every `.defaultEncoding` reference in the corpus is `defaultsAndOverrides().defaultEncoding` (the `DefaultsAndOverrides` field); the `ProtocolInstance` method is never called. |
| `traceLevel()` | [`ProtocolInstance.js:15`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ProtocolInstance.js#L15) | Sole occurrence is its definition; no reader on a ProtocolInstance. |
| `traceCategory()` | [`ProtocolInstance.js:19`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ProtocolInstance.js#L19) | Only non-definition hit is an unrelated `this.traceCategory` *field* in generated `RemoteLogger.js` (collision). No reader on a ProtocolInstance. |

### `OpaqueEndpointI` — three dead endpoint operations (`OpaqueEndpoint.js`, internal)

The opaque endpoint is a pass-through placeholder that is never connected to or accepted on; these interface methods are never invoked.

| Symbol | Location |
|---|---|
| `rawBytes()` | [`OpaqueEndpoint.js:97`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/OpaqueEndpoint.js#L97) |
| `transceiver(endpoint)` | [`OpaqueEndpoint.js:108`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/OpaqueEndpoint.js#L108) |
| `acceptor(endpoint)` | [`OpaqueEndpoint.js:120`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/OpaqueEndpoint.js#L120) |

### Other internal dead methods

| Symbol | Location | Evidence |
|---|---|---|
| `LocatorInfo.hashCode()` | [`LocatorInfo.js:41`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/LocatorInfo.js#L41) | `LocatorInfo` is only ever a Map *value* (keyed by `LocatorPrx`), never a hash key, so its `hashCode` is never called. (Sibling `equals()` *is* live.) |
| `RouterInfo.hashCode()` | [`RouterInfo.js:38`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/RouterInfo.js#L38) | Same: only a Map value, never a key. |
| `Instance.prototype.setLogger` | [`InstanceExtensions.js:167`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/InstanceExtensions.js#L167) | Only occurrence is its definition; no `.setLogger(` call. |
| `ObjectPrx.prototype.ice_instanceof` (+ static `ObjectPrx._instanceof`) | [`ObjectPrxExtensions.js:337`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectPrxExtensions.js#L337) / [`:468`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectPrxExtensions.js#L468) | `ice_instanceof` has zero call sites (only a comment in `Object.js`); the static `_instanceof` is reachable only via `ice_instanceof` + its own recursion. Remove the pair together. |
| `ObjectAdapter.getServantManager` | [`ObjectAdapter.js:320`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectAdapter.js#L320) | Zero `.getServantManager(` calls; not in `ObjectAdapter.d.ts`. |
| `OutputStream.rewriteBool` | [`OutputStream.js:753`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/OutputStream.js#L753) | Single occurrence (its definition); not in `OutputStream.d.ts`, not in generated code. (Sibling `rewriteInt`/`rewriteByte` are live.) |
| Dead interval branch in `Timer.cancel()` | [`Timer.js:45`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Timer.js#L45) | The `if (token.isInterval)` branch calls `Timer.clearInterval`, which is **never defined** (only `clearTimeout`/`setTimeout`/`setImmediate` exist). `schedule()` hard-codes `isInterval: false` and nothing creates an interval token, so the branch is unreachable (and would throw if reached). |

---

## Category B — Public API / standard-interface, unused in-repo (review-only)

Declared in a `.d.ts` (public API) but with no in-repo caller. Removing them changes the public surface, so review-only.

| Symbol | Location | Note |
|---|---|---|
| `ObjectAdapter.findByProxy` | [`ObjectAdapter.js:256`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectAdapter.js#L256) | **Unused across C#, Java, Python, Swift, and JS** — the strongest cross-language deprecation candidate. |
| `ObjectAdapter.findAllFacets` | [`ObjectAdapter.js:250`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectAdapter.js#L250) | Also unused-in-repo in Python. |
| `ObjectAdapter.createDirectProxy` | [`ObjectAdapter.js:300`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/ObjectAdapter.js#L300) | Public; no in-repo caller. |
| `Communicator.getLogger` | [`Communicator.js:135`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Communicator.js#L135) | Unused-in-repo in Ruby/PHP too. |
| `Properties.getCommandLineOptions`, `getPropertyAsList` | [`Properties.js:180`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Properties.js#L180), [`:93`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Properties.js#L93) | Recurring across mappings. |
| `InputStream.readEncapsulation` / `skipSlice` | [`InputStream.js:1100`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/InputStream.js#L1100), [`:1155`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/InputStream.js#L1155) | Public stream API; the internal decoders have their own `skipSlice`. |
| `OutputStream.writeEncapsulation` | [`OutputStream.js:643`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/OutputStream.js#L643) | Public stream API; no in-repo caller. |
| `Logger.print`, `Logger.cloneWithPrefix` | [`Logger.js:24`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Logger.js#L24), [`:62`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Logger.js#L62) | Logger methods unused-in-repo across mappings. |
| `WSConnectionInfo.maxBufferedAmount` | [`Connection.js:75`](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/js/packages/ice/src/Ice/Connection.js#L75) | Public getter; no reader. |

---

## Category C — Conditional / obsolete-platform code

**None.** The only conditional code is the **browser/Node split** (`*.browser.js` variants chosen by the package.json `"browser"` field) — both variants are live for their respective targets. No obsolete-platform residue.

---

## Confidence & limitations

- **High confidence** on Category A — each was verified with full-corpus `command grep` (runtime + generated + tests), receiver-type disambiguation for collision-prone names, and inspection of every call site. The module/export graph is independently clean.
- **The `ProtocolInstance.defaultEncoding` match** is notable: it is dead in C#, Java, Python, **and** JavaScript, all via the same `DefaultsAndOverrides.defaultEncoding` collision masking. `ObjectAdapter.findByProxy` is now confirmed unused-in-repo across **five** mappings (C#, Java, Python, Swift, JS).
- **Known limitation (collision masking):** a dead method whose name collides with a heavily-used same-named member on another class can be masked from a blunt `.method` sweep. This was mitigated by receiver-type disambiguation for the high-interest modules (notably `ProtocolInstance`, which surfaced two extra dead getters that way), but a residual risk remains for the ~600 methods not individually receiver-checked — the same limitation noted in the other reports.
- The audit covered the `Ice` module (the runtime). The thin `Glacier2`/`IceBox`/`IceGrid`/`IceStorm`/`IceMX` packages are almost entirely generated and were treated as caller corpus, not subjects.

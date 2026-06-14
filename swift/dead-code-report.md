# Ice Swift Dead Code Report

**Date:** 2026-06-13
**Scope:** `swift/src` — the Swift `Ice` module (`src/Ice`, 53 hand-written `.swift`), the four umbrella modules (Glacier2/IceGrid/IceStorm/IceBox), and the Objective-C++ bridge (`src/IceImpl`, 16 `.mm` + 19 headers).
**Toolchain:** Swift + Objective-C++ (Clang), xcodebuild (no SwiftPM `Package.swift`).
**Permalink base:** all source links are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487/swift) so line numbers stay stable even as `main` moves.

---

## How this was produced

The Swift mapping has **two layers** — Swift, and the Objective-C++ bridge that wraps the C++ Ice runtime — so both were audited.

1. **Generated the Slice→Swift code.** The bulk of the Glacier2/IceGrid/IceStorm/IceBox modules and much of `Ice` is generated at build time and not committed. It is the primary in-module caller of the hand-written Swift API, so it was generated with `slice2swift` (into a scratch dir) and included in every usage search — otherwise hand-written API used only by generated code would look dead.
2. **Repo-wide static cross-referencing** of every symbol, across `swift/src` (all modules), `swift/test`, and the generated code. As with Java, **Swift's compiler does not warn on unused methods/types**, so static reachability is the primary method (no `-Wunused`-style corroboration).
3. **Objective-C → Swift name-translation handling** for the bridge: an ObjC selector maps to a different Swift name (`- (T)getFoo:(X)x bar:(Y)y` → `getFoo(x, bar: y)`), often overridden by `NS_SWIFT_NAME(...)`. Bridge methods were checked under both their selector base name (ObjC callers) and their Swift-visible name (Swift callers).
4. **Independent per-symbol verification** of every high-confidence finding.

### Access control is the key to classification

- **`private`/`fileprivate`** unused within its file → **dead** (high confidence).
- **`internal`** (Swift's default — no access keyword) unused anywhere → **dead** (high confidence): only same-module code (`src/Ice` + its generated code) can see it, and both were searched. External apps cannot reach it.
- **`public`/`open`**: live if used by another in-repo module (the Glacier2/IceGrid/… modules `import Ice`) or by tests; if **no** in-repo caller → **public-API review only** — external Swift apps may depend on it, so removal is a breaking change, not dead-code cleanup. (In Swift the generated code is in the *same* module, so — unlike Java/C# — `public` is genuinely about external/cross-module use, not generated-code access.)

### Excluded from the audit (not dead, but not reported)

- **Reflection-instantiated types** — the leaf local-exception classes (`ConnectionIdleException`, `DNSException`, `SecurityException`, …) are created at runtime via `NSClassFromString(...) as? LocalException.Type` in `LocalExceptionFactory`, keyed off the C++ exception type id. Live through dynamic dispatch despite no static reference.
- **Protocol-requirement implementations, `override`s, `@objc` members, conformance members** (`==`, `hash(into:)`, `description`, `encode(to:)`, …) — reachable indirectly.
- **C++-dispatched ObjC callbacks** — `ICEDispatchAdapter`, `ICEAdminFacetFactory`, the `ICEConnectionInfoFactory`/`ICEEndpointInfoFactory`/`ICELocalExceptionFactory` methods, and logger/observer callbacks are invoked from the C++ runtime, so they have no direct Swift caller but are live.
- **Generated code and tests count as callers.**

---

## Summary

| Category | Count | Risk to remove |
|---|---|---|
| A. Truly dead — `internal` Swift methods + dead ObjC bridge symbols | 8 | Low — internal/unexposed, no caller |
| B. Vestigial public helper (self-contained, unused) | 1 (a 2-type pair) | Low–medium — `public`, but clearly leftover |
| C. Public API with no in-repo caller (breaking to remove) | ~25 | High — external Swift apps may use these |
| D. Conditional / platform dead code | 0 | n/a — no dead `#if` branches |

**Headline:** The Swift mapping is clean. Because generated code shares the module, the truly-internal surface is small and almost entirely live — only two `internal` `InputStream` helpers are dead. The bridge yields six removable symbols (an orphaned proxy initializer, four unused proxy-identity comparison methods, and one C++ template helper). The larger **C** bucket is boilerplate public API (enum streaming methods, `ObjectAdapter` find-helpers) that mirrors the other language mappings — unused in-repo but external-facing.

---

## Category A — Truly dead code (removable)

### Swift `Ice` module — `internal` methods, zero callers anywhere

| Symbol | Location | Why dead |
|---|---|---|
| `InputStream.startOver()` | [InputStream.swift:268](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/InputStream.swift#L268) | `internal` "reset for retry" helper; only its definition exists — no caller in src, generated, test, or the bridge. Not an override/protocol requirement. |
| `InputStream.skipEncapsulation()` | [InputStream.swift:162](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/InputStream.swift#L162) | `internal`; distinct from the live `public skipEmptyEncapsulation()` / `readEncapsulation()`. No caller. |

### Objective-C++ bridge (`IceImpl`) — declared + implemented, never called from Swift or ObjC

| Symbol | Location | Why dead |
|---|---|---|
| `ICEObjectPrx initWithObjectPrx:` | [ObjectPrx.mm:13](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/ObjectPrx.mm#L13) (decl [ObjectPrx.h:17](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/include/ObjectPrx.h#L17)) | orphaned initializer; the constructor actually used everywhere is `initWithCppObjectPrx:`. Swift wraps handles via `ObjectPrxI(handle:communicator:)`, never this (`init(objectPrx:)`) form. |
| `ICEObjectPrx proxyIdentityLess:` | [ObjectPrx.mm:622](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/ObjectPrx.mm#L622) (decl [ObjectPrx.h:100](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/include/ObjectPrx.h#L100)) | no Swift/ObjC/test caller; Swift proxy comparison bridges to `isEqual:` only. |
| `ICEObjectPrx proxyIdentityEqual:` | [ObjectPrx.mm:627](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/ObjectPrx.mm#L627) (decl [ObjectPrx.h:101](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/include/ObjectPrx.h#L101)) | as above |
| `ICEObjectPrx proxyIdentityAndFacetLess:` | [ObjectPrx.mm:632](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/ObjectPrx.mm#L632) (decl [ObjectPrx.h:103](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/include/ObjectPrx.h#L103)) | as above |
| `ICEObjectPrx proxyIdentityAndFacetEqual:` | [ObjectPrx.mm:637](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/ObjectPrx.mm#L637) (decl [ObjectPrx.h:104](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/include/ObjectPrx.h#L104)) | as above |
| `toNSData<T>(...)` (C++ template helper) | [Convert.h:78](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/IceImpl/Convert.h#L78) | file-scope C++ helper; its siblings (`toNSArray`/`toNSDictionary`/`fromNSArray`/…) are all instantiated, but `toNSData` is never used. |

> The four `proxyIdentity*` methods are a self-contained group — the Swift mapping simply never exposes proxy-identity-only comparison through the bridge. Remove all four together with their header declarations.

---

## Category B — Vestigial public helper (review, but a strong deletion candidate)

- **`DictEntry<K,V>`** ([InputStream.swift:1674](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/InputStream.swift#L1674)) and **`DictEntryArray<K,V>`** ([InputStream.swift:1684](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/InputStream.swift#L1684)) — a `public` dictionary-marshaling helper pair that is used **only by each other**: `DictEntryArray` holds `[DictEntry]`, and nothing references `DictEntryArray`. A self-contained, unreachable leftover. Technically `public` (so external code *could* reference it), but it is almost certainly vestigial — recommend removing the pair after a quick confirmation that no published example relies on it.

---

## Category C — Public API with no in-repo caller (REVIEW ONLY — breaking to remove)

`public` symbols with no in-repo caller. External Swift apps may depend on them; listed for awareness, not as dead-code cleanup. Grouped by kind.

**Enum streaming boilerplate** — `InputStream.read()`/`read(tag:)` and `OutputStream.write(_:)`/`write(tag:value:)` plus the no-arg `init()`, generated-style (un)marshaling for enums that are used in the proxy/connection API but **never marshaled as Slice types in-repo**:
- `CompressBatch` — [Connection.swift:16,26,38,51,60](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/Connection.swift#L16)
- `EndpointSelectionType` — [EndpointSelectionType.swift:13,23,35,48,57](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/EndpointSelectionType.swift#L13)
- `ToStringMode` — [ToStringMode.swift:23,33,45,58,67](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/ToStringMode.swift#L23)

These three are the strongest *non-internal* deletion candidates — they are pure boilerplate for types that are never streamed — but they are `public`, so confirm against the published API before removing.

**`ObjectAdapter` find-helpers** — `public` protocol methods with no in-repo caller (their `ObjectAdapterI` implementations satisfy the protocol, so the impls are not dead):
- `findFacet(id:facet:)` [ObjectAdapter.swift:190](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/ObjectAdapter.swift#L190), `findAllFacets(_:)` [:196](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/ObjectAdapter.swift#L196), `findByProxy(_:)` [:204](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/ObjectAdapter.swift#L204), `findServantLocator(_:)` [:226](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/ObjectAdapter.swift#L226). `findByProxy` is unused in-repo in **C++, C#, and Java too** — a consistent cross-mapping signal.

**Other public API, no in-repo caller:**
- `Communicator.initializePlugins()` — [CommunicatorI.swift:295](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/CommunicatorI.swift#L295) — documented plug-in API for apps.
- `InputStream.read(tag:type: ObjectPrx.Protocol)` — [InputStream.swift:809](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/InputStream.swift#L809) — symmetric optional-proxy marshaling; the non-tagged form is live, this tagged base overload isn't.
- `intVersion` / `stringVersion` — [Initialize.swift:174](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/Initialize.swift#L174) / [:178](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/Initialize.swift#L178) — published version constants.

**Intentionally retained (do not remove):**
- `Disp` typealias — [Dispatcher.swift:13](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/Dispatcher.swift#L13) — `@available(*, deprecated, renamed: "Dispatcher")` compat alias.
- `UnknownException.reason` — [LocalExceptions.swift:178](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/swift/src/Ice/LocalExceptions.swift#L178) — `@available(*, deprecated, renamed: "message")` shim.

---

## Category D — Conditional / platform dead code

**Empty.** Swift `src` has **no `#if` conditional compilation** at all. The ObjC++ bridge has only standard `#ifdef __cplusplus` guards (separating the ObjC and ObjC++ sections of headers) and two legitimate live platform branches (`TARGET_OS_OSX` / `TARGET_OS_IPHONE` for macOS-vs-iOS). No dead/legacy branches.

---

## Suggested next steps

1. **Low-risk cleanup (Category A, 8 symbols):** delete the two `internal` `InputStream` helpers and the six bridge symbols (the orphaned `initWithObjectPrx:`, the four `proxyIdentity*` methods + their header decls, and the `toNSData` template). All have zero callers and no external visibility (the bridge symbols are internal to the mapping; `ICEObjectPrx` is not a published API).
2. **Quick confirm, then remove (Category B):** the `DictEntry`/`DictEntryArray` pair.
3. **API decisions (Category C):** the enum-streaming boilerplate is the most compelling removal but is `public`; the `ObjectAdapter` find-helpers and `findByProxy` could be coordinated with the C++/C#/Java mappings (all show `findByProxy` unused). Keep the two deprecated shims.

---

## Methodology notes & confidence

- **Two layers, one mapping.** Dead code can hide in either the Swift API or the ObjC++ bridge; both were searched, and the bridge required ObjC→Swift name translation (and `NS_SWIFT_NAME` awareness) to avoid false positives.
- **No compiler dead-code detection.** Swift does not warn on unused methods/types, so — as with the Java audit — this rests entirely on static reachability (reference counts + protocol/override/`@objc`/reflection checks), not an automated whole-program tool. The reflection-instantiated exception classes and the C++-dispatched ObjC callbacks are the main false-positive shapes, and were verified live and excluded.
- **Generated code is a caller.** Findings were checked against `slice2swift`-generated code; a symbol used only there is live.
- Confidence is per-symbol and evidence-based. Treat the list as a high-quality candidate set to confirm in review, not an auto-delete list. Findings reflect commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487/swift) (2026-06-13).

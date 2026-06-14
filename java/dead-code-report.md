# Ice Java Dead Code Report

**Date:** 2026-06-13
**Scope:** `java/src` (the Ice for Java runtime, the IceBox/IceGrid/IceStorm/Glacier2/discovery/IceBT components, and the IceGridGUI admin application)
**Toolchain:** Java 17 (`targetJavaRelease = 17`), Gradle 9.2.1
**Permalink base:** all source links below are pinned to commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487/java) so line numbers stay stable even as `main` moves.

---

## How this was produced

The same approach used for the C++ and C# audits, adapted to Java:

1. **Full build** (`./gradlew compileJava`, BUILD SUCCESSFUL) — primarily to **generate the Slice code**. Unlike C++ (`-Wunused`) and C# (`AnalysisMode=All` + `TreatWarningsAsErrors`), **Java's compiler does not warn about unused methods/classes**, and the project's OpenRewrite lints are style-only. So there is **no compiler corroboration** for dead code here — static cross-referencing is the sole method, which is why every finding below was checked by reference-counting and re-verified by hand.
2. **Repo-wide static cross-referencing** of every symbol defined in hand-written `java/src`, with usage counted across the **entire** Java tree — `java/src`, `java/test`, **and the 1,161 generated Slice files** under `*/build/generated/source/slice/`.
3. **Independent spot-verification** of every high-value finding (call-site vs. definition counts, overload binding, `@Override`/interface membership, reflective binding).

### Two Java-specific gotchas that shaped the method

- **`.gitignore`-aware search hides generated code.** In this environment `grep`/`rg` are wrapped to honor `.gitignore`, and `build/` is git-ignored — so a naive search **silently skips the 1,161 generated Slice files**, which are the primary callers of the runtime's marshaling/dispatch helpers. Searching that way produces false positives (e.g. `OutputStream.readEnum`, `Current.checkNonIdempotent` look "dead" but are called thousands of times by generated code). Every result here was re-verified with **`command grep` / `rg --no-ignore`** covering `build/generated`. **Anyone re-running this audit must search generated code explicitly.**
- **No separate "internal" package.** The C++ `IceInternal` namespace did **not** become a Java package — it was merged into `com.zeroc.Ice`, so public API and logically-internal plumbing share one package, and the plumbing is `public` purely so other packages can reach it (exactly the over-exposure you described). The internal surface was identified via the **`@hidden` Javadoc tag** (used on ~46 internal classes, e.g. `Instance`, `ProtocolInstance`, `Holder`) plus judgment about obvious plumbing (`ConnectionI`, `ThreadPool`, SSL transceivers/engines). A `public`-but-internal symbol with no caller anywhere is dead.

### Cross-language corroboration

Several findings are the **same dead methods** flagged in the C++ and C# audits — the same logically-dead code was mechanically ported to all three bindings. This independently corroborates them:

| Symbol (Java) | Also dead in |
|---|---|
| `ProtocolInstance.defaultEncoding()` | C# |
| `Instance.setThreadHooks(...)` (C#: `setThreadHook`) | C# |
| `OutputBase.setIndent(...)` (C++: `OutputBase::setIndent`) | C++ |

### Category D — Conditional / platform dead code

**Empty by construction.** Java has no preprocessor; there are no `#if`-style dead branches and a single toolchain target (Java 17). The 18 files that read `os.name`/`os.arch` are live runtime platform detection, not dead branches. (Same clean result as C#.)

### Excluded from the audit (not dead, but not in scope)

- **Generated Slice code** (`*/build/generated/`) — excluded as a source of candidate symbols, **searched** for usages.
- **Reflection / Slice dispatch / plugin & service entry points** — servant `_iceD_*` dispatch, marshaling helpers, instrumentation resolvers bound via `getMethod(...)`, plugins/services loaded by class-name string from config (verified against the Python test harness, e.g. `IceDiscovery.PluginFactory` is loaded by name in `scripts/Util.py`), and the IceGridGUI metrics fields instantiated reflectively. Verified and **not** reported.
- **Polymorphic / framework members** — interface impls, `@Override`/abstract overrides, Swing/AWT callbacks (`actionPerformed`, `getTreeCellRendererComponent`, `TableModel`/`TreeNode` methods…), `equals`/`hashCode`/`toString`/`compareTo`, Java-serialization callbacks (`readObject`/`writeObject`).
- **Test usage counts as live.**

---

## Summary

| Category | Count | Risk to remove |
|---|---|---|
| A. Truly dead — logically-internal (`@hidden`/plumbing) symbols | 30 | Low — not real public API, no caller on any path |
| B. Intentional "dead" — documented / deliberately retained | 2 groups | Keep — annotated `@SuppressWarnings`, or compile-only stubs |
| C. Public API with no in-repo caller (breaking to remove) | 13 | High — external SDK consumers may use these |
| D. Conditional / platform dead code | 0 | n/a — no preprocessor |

**Headline:** The Java codebase is clean. Despite the large public-but-internal surface (and the absence of any compiler dead-code check), only ~30 genuinely-dead internal members turned up — unused accessors, setters, convenience constructors, a couple of transitively-dead SSL delegation pairs, and one dead nested class in the GUI. All are low-risk deletions; many would also be safe to demote from `public` to package-private.

---

## Category A — Truly dead code (logically-internal)

Each symbol below has **zero** references anywhere in `java/src`, `java/test`, or generated code, is not an `@Override`/interface member, and is not reflectively bound.

### `com.zeroc.Ice` (core runtime)

| Symbol | Location | Why dead |
|---|---|---|
| `ConnectionI.isFinished()` | [ConnectionI.java:171](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/ConnectionI.java#L171) | `final` internal plumbing; not declared in `Connection`/`CancellationHandler`/`EventHandler`, never called |
| `Holder(T)` constructor | [Holder.java:11](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Holder.java#L11) | `@hidden`; only the no-arg `new Holder<>()` is ever used, even by generated code |
| `Instance.setThreadHooks(Runnable, Runnable)` | [Instance.java:479](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Instance.java#L479) | `@hidden`; thread hooks are set via `InitializationData` fields directly — **also dead in C#** |
| `ProtocolInstance.defaultEncoding()` | [ProtocolInstance.java:70](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/ProtocolInstance.java#L70) | `@hidden`; only the definition exists (subclasses `SSL.Instance`/`IceBT.Instance` don't call it) — **also dead in C#** |
| `OutputBase()` constructor | [OutputBase.java:12](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L12) | only the `PrintWriter` ctor is used |
| `OutputBase(String)` constructor | [OutputBase.java:30](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L30) | never instantiated |
| `OutputBase.open(String)` | [OutputBase.java:49](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L49) | transitively dead (sole caller is the dead `OutputBase(String)`) |
| `OutputBase.setIndent(int)` | [OutputBase.java:41](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L41) | no caller — **OutputBase cluster mirrors the C++ `OutputBase` dead members** |
| `OutputBase.dec()` | [OutputBase.java:74](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L74) | `inc()` is used, `dec()` is not |
| `OutputBase.zeroIndent()` | [OutputBase.java:84](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L84) | no caller |
| `OutputBase.sp()` | [OutputBase.java:124](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L124) | no caller |
| `OutputBase.valid()` | [OutputBase.java:130](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputBase.java#L130) | no caller |

> `OutputBase` is an internal utility (a port of the C++ `Output` helper) with **eight** dead members — the single biggest cluster. It's a strong candidate to trim down to just the methods actually used (`inc`, `nl`, `print`, `useCurrentPosAsIndent`/`restoreIndent`, the `PrintWriter` ctor).

### `com.zeroc.Ice.SSL`

| Symbol | Location | Why dead |
|---|---|---|
| `Instance.createSSLEngine(boolean, String, int)` | [SSL/Instance.java:31](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/SSL/Instance.java#L31) | dead wrapper; `AcceptorI`/`ConnectorI` call `engine().createSSLEngine(...)` directly, bypassing it |
| `Instance.securityTraceCategory()` | [SSL/Instance.java:27](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/SSL/Instance.java#L27) | no caller (sibling accessors are used) |
| `Instance.trustManagerFailure(boolean, CertificateException)` | [SSL/Instance.java:43](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/SSL/Instance.java#L43) | no caller |
| `SSLEngine.securityTraceCategory()` | [SSL/SSLEngine.java:289](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/SSL/SSLEngine.java#L289) | transitively dead — only caller is the dead `Instance.securityTraceCategory()` (the `_securityTraceCategory` field is used directly) |
| `SSLEngine.trustManagerFailure(boolean, CertificateException)` | [SSL/SSLEngine.java:371](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/SSL/SSLEngine.java#L371) | transitively dead — only caller is the dead `Instance.trustManagerFailure(...)` |

> The two SSL `Instance`→`SSLEngine` delegation pairs (`trustManagerFailure`, `securityTraceCategory`) are dead at **both** ends — remove all four together.

### `com.zeroc.IceMX`

| Symbol | Location | Why dead |
|---|---|---|
| `MetricsHelper.AttributeResolver.add(String, java.lang.reflect.Field)` | [MetricsHelper.java:92](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/IceMX/MetricsHelper.java#L92) | unused overload; the resolver's only users call the `(String, Method)`, `(String, Method, Field)`, `(String, Method, Method)` overloads |

### `com.zeroc.IceGridGUI` (admin application — all symbols are app-internal)

| Symbol | Location | Why dead |
|---|---|---|
| `Application.PlainServer.setServerDescriptor(ServerDescriptor)` | [Application/PlainServer.java:391](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/Application/PlainServer.java#L391) | non-override setter, no caller; `rebuild(...)` assigns `_descriptor` directly |
| `LiveDeployment.MetricsViewEditor.ColumnInfo` (nested class) | [LiveDeployment/MetricsViewEditor.java:966](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/MetricsViewEditor.java#L966) | whole class never instantiated; its ctor signature doesn't match the reflective metrics-field ctor |
| `Utils.iconToImage(...)` | [Utils.java:53](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/Utils.java#L53) | no caller |
| `Utils.propertySetToMap(...)` | [Utils.java:471](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/Utils.java#L471) | no caller (distinct from the live plural `propertySetsToMap`) |
| `Utils.Resolver.getParameters()` | [Utils.java:378](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/Utils.java#L378) | `Resolver` implements no interface; no caller |
| `SimpleInternalFrame.setFrameIcon(Icon)` | [SimpleInternalFrame.java:155](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/SimpleInternalFrame.java#L155) | `extends JPanel` (not `JInternalFrame`) → not an override; no caller |
| `SimpleInternalFrame.getFrameIcon()` | [SimpleInternalFrame.java:146](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/SimpleInternalFrame.java#L146) | *medium* — transitively dead (only caller is the dead `setFrameIcon`) |
| `SimpleInternalFrame.getTitleLabel()` | [SimpleInternalFrame.java:166](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/SimpleInternalFrame.java#L166) | not an override; no caller |
| `SessionKeeper.ConnectionInfo.setUUID(String)` | [SessionKeeper.java:623](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/SessionKeeper.java#L623) | `_uuid` is set in the ctor / via `UUID.randomUUID()`; this setter is never called (getter is used) |
| `LiveDeployment.Node.getServers()` | [LiveDeployment/Node.java:826](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/Node.java#L826) | unique token in the tree; not an override; no caller |
| `LiveDeployment.MetricsViewEditor.MetricsField.getPropertyPrefix()` | [LiveDeployment/MetricsViewEditor.java:981](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/MetricsViewEditor.java#L981) | *medium* — interface method + its sole `@Override` impl (`AbstractField`, line 1066) are never invoked; remove the pair together |

### `com.zeroc.IceBT`

| Symbol | Location | Why dead |
|---|---|---|
| `Instance.destroy()` | [icebt/Instance.java:26](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.icebt/src/main/java/com/zeroc/IceBT/Instance.java#L26) | package-private; never invoked (`PluginI.destroy()` is empty — the `Instance` objects are never torn down). Ported from C++ `Instance::destroy()` |

---

## Category B — Intentional "dead" (keep; do not remove)

- **WebSocket reserved-opcode constants** `WSTransceiver.OP_RES_0x3` … `OP_RES_0xF` — [WSTransceiver.java:1239-1270](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/WSTransceiver.java#L1239-L1270). Ten `private static final int` constants, each annotated `@SuppressWarnings("unused")` — the maintainers deliberately keep them as protocol documentation. Not accidental dead code.
- **Android Bluetooth compile-only stubs** under [icebt/.../android/bluetooth/](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.icebt/src/main/java/android/bluetooth/) — four uncalled stub methods exist to mirror the real Android API surface and are `exclude("android/**")`-d from the JAR (`build.gradle:26`). Intentional; leave as-is.

---

## Category C — Public API with no in-repo caller (REVIEW ONLY — breaking to remove)

`public` members of **genuine public-API** classes (not `@hidden`, not plumbing). "No in-repo caller" does not mean dead — external SDK consumers may use them. Review-only.

| Symbol | Location | Note |
|---|---|---|
| `Util.typeIdToClass(String)` | [Util.java:363](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Util.java#L363) | `public static` utility, fully documented, not `@hidden`; no in-tree use |
| `Properties.getCommandLineOptions()` | [Properties.java:391](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Properties.java#L391) | published `Properties` API; counterpart to `parseCommandLineOptions` — **also flagged in C#** |
| `ObjectAdapter.findByProxy(ObjectPrx)` | [ObjectAdapter.java:633](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/ObjectAdapter.java#L633) | only `@see` Javadoc links reference it — **also flagged in C#** |
| `PluginManager.getPlugins()` | [PluginManager.java:24](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/PluginManager.java#L24) | public interface method, no in-tree caller — **also flagged in C#** |
| `NativePropertiesAdmin.removeUpdateCallback(Consumer)` | [NativePropertiesAdmin.java:156](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/NativePropertiesAdmin.java#L156) | symmetric remove half; `addUpdateCallback` is used — **also flagged in C#** |
| `OutputStream.rewriteBool(boolean, int)` | [OutputStream.java:615](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/OutputStream.java#L615) | public streaming API; `rewriteByte`/`rewriteInt` are used, this sibling isn't |
| `CompressBatch.value()` | [CompressBatch.java:21](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/CompressBatch.java#L21) | public enum convenience method, unused in-tree |
| `CompressBatch.valueOf(int)` | [CompressBatch.java:31](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/CompressBatch.java#L31) | as above (custom int overload) |
| `Instrumentation.ConnectionState.value()` | [Instrumentation/ConnectionState.java:29](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Instrumentation/ConnectionState.java#L29) | public observer-API enum boilerplate; constants are used, `value()` isn't |
| `Instrumentation.ConnectionState.valueOf(int)` | [Instrumentation/ConnectionState.java:39](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Instrumentation/ConnectionState.java#L39) | as above |
| `Instrumentation.ThreadState.value()` | [Instrumentation/ThreadState.java:27](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Instrumentation/ThreadState.java#L27) | as above |
| `Instrumentation.ThreadState.valueOf(int)` | [Instrumentation/ThreadState.java:37](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Instrumentation/ThreadState.java#L37) | as above |
| `LoggerPlugin` class + constructor | [LoggerPlugin.java:17](https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/LoggerPlugin.java#L17) | a documented public `Plugin` apps instantiate to install a custom logger — expected to be unused in-tree; **keep** |

---

## Suggested next steps

1. **Low-risk cleanup (Category A, 30 symbols):** delete the unused internal accessors/setters/ctors. Highest-value single target is the **`OutputBase` cluster** (8 dead members) and the **two SSL delegation pairs** (4 members, remove both ends together). The two *medium* GUI items (`SimpleInternalFrame.getFrameIcon`, `MetricsViewEditor.MetricsField.getPropertyPrefix`) are interface/transitive pairs — remove the declaration and impl together.
2. **Tighten visibility, not just delete:** many surviving `com.zeroc.Ice` plumbing members are `public` only for cross-package access. Where a `@hidden` member is used solely within `com.zeroc.Ice`, demoting `public` → package-private would let future disuse surface in review (Java still won't warn, but it shrinks the over-exposed surface you flagged).
3. **API decisions (Category C, 13 symbols):** deprecate-vs-keep calls. Note the strong overlap with the C# review-only list (`getCommandLineOptions`, `findByProxy`, `getPlugins`, `removeUpdateCallback`) — these are unused in both bindings and worth a coordinated decision. `LoggerPlugin` is intentional public API; keep.
4. **Leave Category B alone** — the WS opcode constants and Android stubs are deliberate.

---

### Caveats / known coverage gaps

- Findings reflect the source tree at commit [`2f1d39c`](https://github.com/zeroc-ice/ice/tree/2f1d39c427a5a6dcbee73208d9536e760e322487/java) (2026-06-13).
- **Java provides no compiler-level dead-code detection** (no analyzer/lint for unused methods, unlike the C# and C++ builds), so this audit rests entirely on static reachability. Confidence is per-symbol and evidence-based (reference counts incl. generated code + override/reflection checks). Treat it as a high-quality candidate set to confirm in review, not an auto-delete list.
- The one structural risk — generated Slice code being hidden by `.gitignore`-aware search — was handled by re-verifying every finding with generation-aware `grep`. Anyone reproducing this must do the same.
- The audit targets hand-written `java/src`. The IceGridGUI Swing application has heavy framework/reflection wiring; its callbacks and reflectively-instantiated metrics fields were verified live and excluded.

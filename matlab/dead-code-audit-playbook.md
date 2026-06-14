# MATLAB Dead-Code Audit — Handoff Playbook

**Read this first.** This is the start-here document for auditing dead code in the Ice for MATLAB mapping (`matlab/`). The other eight mappings (C++, C#, Java, Swift, Python, Ruby, PHP, JavaScript) are already done — their reports are committed alongside this file as `*/dead-code-report.md`. MATLAB was deferred to Windows because **the MEX extension can't be built on macOS** (no MATLAB there), and this audit method leans on building the extension.

**Deliverable:** `matlab/dead-code-report.md`, in the same format as the other eight (Category A/B/C, GitHub permalinks, methodology + confidence sections).

**Pin all permalinks to commit `2f1d39c427a5a6dcbee73208d9536e760e322487`** (format: `https://github.com/zeroc-ice/ice/blob/2f1d39c427a5a6dcbee73208d9536e760e322487/<path>#L<line>`). If you re-pin to a newer commit, re-pin every link.

---

## What MATLAB is (already scouted on macOS)

Same two-layer C++-extension shape as Python/Ruby/PHP — **read `python/dead-code-report.md`, `ruby/dead-code-report.md`, and `php/dead-code-report.md` first; they are the closest analogs.**

- **C++ MEX extension** — `matlab/src/` (Communicator, Connection, Endpoint, Future, ImplicitContext, Init, Logger, ObjectPrx, Properties, Util + `Util.h`, `Future.h`, `ice.h`). ~?K lines.
- **Pure-MATLAB layer** — `matlab/lib/`, with a **package-based public/internal split that makes classification trivial**:
  - `+Ice/` — **84** `.m` files = PUBLIC API (one `classdef` per file).
  - `+IceInternal/` — **23** `.m` files = INTERNAL (Buffer, EncapsDecoder/Encoder*, DefaultSliceLoader, …).
  - Also `+Ice/+SSL/` and a generated `toolbox/`.
  This is the MATLAB analog of C#'s `Ice.Internal` namespace or JS's "no `.d.ts`" split — **`+IceInternal` membership ⇒ internal; unused internal symbol ⇒ truly dead. `+Ice` ⇒ public API; unused ⇒ Category B (review-only).**
- **Client-only**, like Ruby/PHP: there is **no `ObjectAdapter`** (`matlab/src/ObjectAdapter.cpp` and `matlab/lib/+Ice/ObjectAdapter.m` do not exist). So the cross-language `findByProxy` finding **does not apply** — the whole server-side surface is simply absent, not dead.
- **`cpp/bin/slice2matlab` is already built** — you can generate the caller corpus the same way the other mappings did (it was prebuilt at the pinned commit; rebuild C++ if needed).

### The C++↔MATLAB boundary

- **MATLAB → C++:** a single MEX gateway (`mexFunction` in `matlab/src/Init.cpp`) — confirm how it dispatches (likely a string opcode / method id from the `.m` classes via `calllib`/a generated dispatch). The `.m` classes hold a C++ handle and call into the extension.
- **C++ → MATLAB (reflection keepers):** the extension constructs MATLAB objects and calls MATLAB functions by **name** via `mexCallMATLAB(..., "Name")`. Already spotted in `matlab/src/Util.cpp`: `mexCallMATLAB(..., "Ice.Identity")`, `"string"`, `"cellstr"`, `"int32"`. **Grep `matlab/src/*.cpp` for every `mexCallMATLAB(... "X")` and every constructed class name — those `+Ice` classes/functions are LIVE even with no `.m` caller** (this is the `convertException`/`nameToClass`/`rb_path2class`/`lookupType` analog).
- **Local-exception construction:** find the `convertException` analog in `matlab/src/Util.cpp` (it maps a C++ `Ice::LocalException` → the matching `Ice.*` MATLAB class by name). That keeps the **entire `+Ice` local-exception hierarchy LIVE** regardless of `.m` callers — do not flag any of them dead. (Same finding in every other mapping.)

---

## The method (distilled from the 8 completed audits)

### C++ layer — two passes

1. **Clean-compile proof.** Build the extension and confirm **0 compiler warnings** with `-Wall -Wextra` (Windows: check the MSBuild output; a benign linker note about MATLAB lib versions is fine, like the Ruby `libruby` note). `-Wall` includes `-Wunused-function`, so **unused file-local `static` functions are provably nil** — you don't have to hunt them. This is the same proof used for the C# analyzers and every extension mapping.
2. **Header-declaration scan (the high-value pass).** The compiler can NOT prove an *exported* (`extern`, header-declared) function unused. Enumerate every function declared in `matlab/src/*.h`, then count call sites across `matlab/src/*.cpp` **and `*.h`** (header-inline callers matter — this is what caught Ruby's `callProtected` false-clear). A function with only a definition + declaration and **no call site or address-of** is dead. *This pass found the only C++ dead code in Ruby (`isHash`) and PHP (`createWrapper`, `AutoDestroy::release`) — run it.*
   - `nm` cross-TU analysis is **only an overload-collision cross-check, not authoritative** here: one-`.cpp`-per-class means intra-TU calls are invisible to `nm`, and `-O2` inlining erases cross-TU `U` refs. If you use it, build `OPTIMIZE=no` and treat candidates as *suspects to source-verify*, not findings.
3. **MEX-facing surface.** Enumerate the methods/functions the extension exposes to MATLAB (the dispatch table / gateway opcodes) and cross-reference against the `.m` caller corpus. Unused ones that are **public** Ice API ⇒ Category B; internal ⇒ Category A (but the internal ones are usually generated-code-facing keepers — verify).

### Pure-MATLAB layer — static reachability (no build needed; could be pre-done)

For every `classdef`, method, and function in `matlab/lib/+Ice/**` and `matlab/lib/+IceInternal/**`:
- A symbol referenced nowhere in the corpus, classified by package: `+IceInternal` ⇒ **Category A**; `+Ice` ⇒ **Category B**.
- MATLAB method-call syntax: `obj.method(...)`, `obj.method` (property/getter), `Ice.Foo(...)` (constructor/package function), `import`. Watch `classdef` inheritance (`< Base`) and dynamic dispatch.
- **Keepers:** anything constructed/called from C++ via `mexCallMATLAB` (mapped above); all `+Ice` local exceptions (C++-constructed); value/stream lifecycle methods invoked by the marshaling engine (`ice_preMarshal`/`ice_postUnmarshal`/`iceWrite`/`iceRead`/`ice_id` analogs — check `+IceInternal/EncapsEncoder*`/`EncapsDecoder*`); the `SliceLoader`/`DefaultSliceLoader` path; enum helpers.

### Caller corpus (use `command grep`, never bare `rg`/`git grep`)

1. Hand-written `.m`: `matlab/lib/+Ice/**` + `matlab/lib/+IceInternal/**` (subjects *and* callers).
2. **Generated** Slice→MATLAB code: produced by `cpp/bin/slice2matlab` from `slice/**.ice` (and the test `.ice`). **This is git-ignored** — generate it to a temp dir and include it, or your reachability will over-report. Confirm the gitignore with `git check-ignore`.
3. Tests: `matlab/test/**`.

> **The git-ignore trap is real and recurring:** the generated corpus is git-ignored in every mapping, and `ripgrep`/`git grep` skip git-ignored files by default → you'd flag the whole runtime as dead. Use `command grep -rn ... --include='*.m'` / `--include='*.cpp'`.

---

## Cross-mapping suspects — check these specifically in MATLAB

These recurred across mappings; verify the MATLAB equivalents (disambiguate by receiver — see collision masking below):

- **`ProtocolInstance.defaultEncoding`** — DEAD in C#, Java, Python, **and** JS, every time **masked** by the heavily-used `DefaultsAndOverrides.defaultEncoding`. If MATLAB has a `ProtocolInstance` (likely `+IceInternal/ProtocolInstance.m`), check whether its `defaultEncoding` is ever read on a *ProtocolInstance* receiver (vs the DefaultsAndOverrides field). Strong candidate.
- **Logger methods** `cloneWithPrefix` / `print` / `trace` / `warning` / `error` — public-API-unused-in-repo in most mappings.
- **`Connection.throwException`** and **`toString`** (vs MATLAB's `char`/`disp`) — unused-in-repo in Python/Ruby/PHP.
- **`Communicator.getLogger`**, **`Properties.getCommandLineOptions` / `getPropertyAsList`** — recurring Category B.
- `findByProxy` — N/A (MATLAB is client-only, no ObjectAdapter).

---

## Gotchas / lessons (paid for across 8 audits)

- **Collision masking** (the big one): a dead method whose name matches a heavily-used same-named member shows as "used" in a blunt `.name` grep. This is how `defaultEncoding` was missed in the very first C++ pass. **For any candidate, disambiguate by receiver type / class** before declaring it live OR dead. It cuts both ways (false-live *and*, via generic names, false-dead).
- **zsh does not word-split unquoted variables.** `for n in $NAMES` runs once with the whole string; `grep $DIRS` passes one bogus path. Use `${=VAR}`, arrays, or literal args. (Cost a redo in Python and JS.)
- **BSD `sed`/`grep` don't grok `\s`.** `sed 's/.*,\s*//'` leaves a leading space → every later pattern fails to match. Use `[[:space:]]`. (Cost a redo in PHP.) On Windows you'll likely use Git-Bash/PowerShell — re-verify your tool's regex flavor with a known-positive sanity check (e.g. confirm a method you *know* is used returns nonzero) before trusting any zero.
- **Search `.cpp` AND `.h`.** Header-inline template/wrapper calls are real callers (Ruby `callProtected`).
- **`nm` is a cross-check, not an oracle** — inlining and intra-TU calls defeat it; source-level is authoritative.
- **Report honestly:** the residual collision-masking risk for not-individually-verified methods is a real limitation — state it in the report's Confidence section, as the other eight do.

---

## Step-by-step on Windows

1. `git checkout <this branch>`; confirm the 8 `*/dead-code-report.md` files and this playbook are present.
2. Build Ice for C++ (prereq), then build Ice for MATLAB (`matlab/BUILDING.md`; MSBuild on Windows). **Capture the build log; confirm 0 compiler warnings.**
3. Generate the caller corpus: run `cpp/bin/slice2matlab` (or `.exe`) over `slice/**/*.ice` and the MATLAB test `.ice` into a temp dir; confirm generated `.m` are git-ignored.
4. Map the boundary: grep `matlab/src/*.cpp` for `mexCallMATLAB(... "X")` and the `convertException` analog → the C++→MATLAB keeper set.
5. C++ layer: clean-compile proof + header-declaration scan (+ optional `nm OPTIMIZE=no` cross-check) + MEX-facing surface sweep.
6. Pure-MATLAB layer: reachability over `+Ice` (→ B) and `+IceInternal` (→ A), honoring keepers. *(This step needs no build — it can be done first, even on macOS, if you want to front-load it.)*
7. Write `matlab/dead-code-report.md` (Category A/B/C, permalinks at the pinned SHA, methodology + confidence). Mirror the structure of `php/dead-code-report.md` (closest analog: client-only C++-extension mapping).

## Expectation

Based on the other client-only extension mappings (Ruby: 1 dead; PHP: 2 dead), expect **a small Category A** (likely 0–3 dead C++ helpers + a handful of unused `+IceInternal` methods, e.g. a `ProtocolInstance.defaultEncoding` if present), a **Category B** list of public `+Ice` API the MATLAB test suite doesn't exercise, and **Category C empty** (only active MATLAB/compiler guards). MATLAB is client-only, so no server-side surface to consider.

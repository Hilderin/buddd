# ADR-004: Demo System Architecture — Per-Demo Files and Extensible Dispatch

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

SPEC-006 introduced a Command pattern with `TestCommand` for running an automated render verification. Experience revealed several limitations that SPEC-007 addressed:

1. **TestCommand conflated two concerns**: running a render verification and running a named demo. Each future visual demo (texture, animation, etc.) would have required its own top-level subcommand, cluttering the command namespace.
2. **No extensibility convention**: Adding a new visual demonstration required modifying the command dispatch table and adding a new command class. There was no convention for per-demo files — the demo logic lived inside `TestCommand::run()`.
3. **Demo helpers lived at the module root**: `demo_helpers.h` and `demo_helpers.cpp` sat in `src/cmd/` alongside `main.cpp` rather than being co-located with the code that uses them.
4. **No namespace scoping for demo code**: All command code lived in `buddd::cmd` with no sub-namespace, making it harder to distinguish command infrastructure from demo implementations.

The project also had architectural constraints that influenced the design:
- **CONST-001**: No SDL3, OpenGL, or GLM headers outside `src/engine/`. Demo code must go through engine abstractions.
- **No CLI framework**: The project uses C++26 standard library only for argument parsing.
- **No dynamic discovery**: Plugin systems, reflection, or runtime demo registration were ruled out as over-engineering for the foreseeable scope of demos (< 20).

## Decision

We adopt the following architecture for the demo system:

### 1. A dedicated `src/cmd/demo/` directory

Every visual demo lives in its own `.h`/`.cpp` pair under `src/cmd/demo/`. This creates a clear boundary between command infrastructure (`src/cmd/commands/`) and demo implementations.

### 2. Per-demo free functions in `buddd::cmd::demo` namespace

Each demo exposes a single free function with a consistent signature:

```cpp
namespace buddd::cmd::demo {
auto run_<name>_demo(buddd::engine::Platform& platform,
                     buddd::engine::RenderDevice& device,
                     int argc, const char* const* argv) -> int;
}
```

Demo functions receive `argc - 2, argv + 2` from `DemoCommand`, so `argv[0]` is the demo name and `argv[1..]` are extra arguments. This consistent signature enables future demos to parse per-demo arguments without changing the dispatch contract.

### 3. If/else-if dispatch in `DemoCommand::run()`

`DemoCommand` maintains an explicit if/else-if chain mapping demo name strings to function calls:

```cpp
if (demo_name == "triangle") {
    return buddd::cmd::demo::run_triangle_demo(...);
}
```

No registry, no map data structure, no virtual dispatch. The chain is linear and fully visible in `demo_command.cpp`. Adding a new demo means adding one `else if` branch.

### 4. Demo helpers co-located with demos

The shared `setup_triangle()` helper (used by multiple demos) is moved from `src/cmd/` into `src/cmd/demo/` and its namespace updated from `buddd::cmd` to `buddd::cmd::demo`. This ensures that all demo-related code — including shared utilities — lives under the same directory and namespace.

### 5. CMake glob auto-discovers demo files

The `src/cmd/CMakeLists.txt` glob includes `demo/*.cpp`, so new demo files are automatically picked up by the build system without modifying CMakeLists.txt.

## Alternatives considered

### Each demo as its own top-level subcommand (e.g., `buddd triangle`, `buddd texture`)

- **Pros**: Direct invocation; no dispatch step; subcommand is self-documenting.
- **Cons**: Every new demo adds a top-level command, cluttering the `buddd` command namespace. The dispatch table in `main.cpp` grows linearly with demos. Creates an artificial distinction between "run" (interactive mode) and individual demos, when demos are conceptually variations of the same activity (running a visual demonstration).
- **Verdict**: Rejected. The command namespace should be reserved for distinct *modes* of operation (run, demo, version, help), not individual demonstrations.

### Dynamic demo discovery (plugin system, runtime directory scan, or reflection)

- **Pros**: Adding a demo requires no code changes to dispatch logic; plugin loading enables third-party demos; self-registering demos are elegant.
- **Cons**: Requires plugin loading infrastructure, reflection, or filesystem scanning at runtime — all significant engineering investments. Plugin APIs couple the engine to an ABI. Runtime directory scanning introduces filesystem I/O at startup. None of this complexity is justified for the foreseeable number of demos (< 20, all authored by the project team).
- **Verdict**: Rejected. Over-engineering for current and near-future needs. Explicitly listed as a non-goal in SPEC-007.

### Demo registry pattern (static map or array of `{name, function}` pairs)

- **Pros**: Centralised registration; dispatch is a single `map.find()` call; easy to add/remove demos without touching the if/else chain.
- **Cons**: Requires maintaining a separate data structure. The if/else chain in `DemoCommand` has the same maintenance cost (one line per demo) and is more transparent — the full list of demos is immediately visible when reading `demo_command.cpp`. A registry would add indirection with no benefit for the current scale.
- **Verdict**: Rejected for now. The if/else chain is simpler and more transparent. A registry can be introduced later if the demo count grows significantly.

### Keeping everything in `TestCommand` (status quo from SPEC-006)

- **Pros**: No refactoring cost; no new files or directories; works for the single triangle demo.
- **Cons**: Does not scale to multiple demos. Each new demo would require either a new command class (cluttering `src/cmd/commands/`) or an increasingly large `TestCommand::run()` method with nested conditionals. The naming ("test") conflates CI verification with visual demonstration.
- **Verdict**: Rejected. The status quo was the motivation for SPEC-007.

### Putting demo code inside `src/engine/`

- **Pros**: Direct access to all engine internals; no abstraction boundary concerns.
- **Cons**: Blatantly violates CONST-001 and the separation between engine and application code. Demos are application-level compositions of engine primitives, not engine internals.
- **Verdict**: Rejected. Would violate the constitution.

## Consequences

### Positive

- **Clear extensibility contract**: Adding a new demo requires only (a) creating `.h`/`.cpp` in `src/cmd/demo/`, (b) adding one `else if` branch in `DemoCommand::run()`. No CMake changes, no new command classes, no namespace pollution.
- **Module boundary enforced**: Command infrastructure (`src/cmd/commands/`) and demo implementations (`src/cmd/demo/`) are clearly separated, preventing cross-contamination.
- **Namespace scoping**: `buddd::cmd::demo` clearly denotes demo code, distinct from command classes in `buddd::cmd`.
- **Architecture boundary preserved**: All demo code goes through engine abstractions (`Platform`, `Window`, `RenderDevice`). No SDL3/OpenGL/GLM headers in `src/cmd/` — verified by CONST-001 compliance checks.
- **Self-discovering build**: The CMake glob picks up new `demo/*.cpp` files automatically.
- **Consistent function signature**: All demos share the same `(Platform&, RenderDevice&, int, const char* const*)` signature, making dispatch uniform and enabling optional per-demo argument parsing.
- **No dynamic allocation or dispatch overhead**: The if/else chain is resolved at compile time with no virtual calls, map lookups, or heap allocation.

### Negative

- **Manual dispatch maintenance**: Each new demo requires a manual `else if` addition in `DemoCommand::run()`. This could be error-prone if multiple developers add demos concurrently (merge conflicts on the if/else chain).
- **No runtime discovery**: Demos cannot be added at runtime (no plugin system). This is acceptable for the project's scope but limits third-party extensibility.
- **Demo function signature fixed**: All demos must accept `Platform&` and `RenderDevice&` even if they don't use rendering. Future demos that don't render (e.g., audio-only demos) would need workarounds or a signature evolution.
- **Duplication of boilerplate**: Each demo file pair must include engine headers, set up namespace aliases, and implement its own render loop. This is intentional (each demo owns its loop) but means some boilerplate is repeated across demo files.

### Preservation of precedent

- This ADR is consistent with ADR-003's precedent that the render loop is an application-level concern owned by command code, not the engine.
- The architecture boundary (CONST-001) is preserved without new exceptions.
- ADR-001's `Result<T>` pattern continues to apply to all fallible engine APIs; demo code is application code and may use any error-handling approach.

## References

- SPEC-007 (`.specs/sprint-2026-05/cli-command-evolution/spec.md`): Spec-level documentation of the demo system CLI interface.
- IMPL-007 (`.specs/sprint-2026-05/cli-command-evolution/implementation-contract.md`): Contract-level implementation details including pseudo-code.
- CONST-001 (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): Architecture boundary preserved by this design.
- ADR-001 (`docs/adr/ADR-001-result-error-pattern.md`): Error handling convention (unchanged by this ADR).
- ADR-003 (`docs/adr/ADR-003-render-pipeline-architecture.md`): Precedent that render loops are owned by application code.
- SPEC-006 (`.specs/sprint-2026-05/cli-command-system/spec.md`): The previous CLI command system that this design builds upon.

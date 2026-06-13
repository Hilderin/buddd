# Implementation Contract Review — IMPL-036 (Settings System MVP1 Skeleton)

## Fresh review (13-Jun-2026)

**Outcome**: accepted (fix verified)

This is a fresh review after the contract was substantially reworked. The contract correctly implements the TypeRegistry integration, removes `migrate_ini()`, merges `layout_ini_path()` to a single `const std::string&` method, moves `os_user_config_dir()` to `src/engine/util/`, and adds `editor_data_root()`/`editor_user_data_root()` utilities. All 32 ACs are mapped to test cases. The pimpl pattern for yaml-cpp (ADR-016) is correctly applied.

The **compile-error-level issue** identified in the first review (`SerializationContext` forward-declared but stored by value) has been fixed: both `settings_store.h` and `settings_manager.h` now `#include "scene/component_registry/serialization_context.h"` directly. No remaining blocking issues found on re-review.

## Blocking issues

Items that must be resolved before the contract can be accepted.

- [x] **`SerializationContext` stored by value after forward declaration** — FIXED: both `settings_store.h` and `settings_manager.h` now `#include "scene/component_registry/serialization_context.h"` instead of forward-declaring. Verified in contract at lines 198 and 370. Compiles correctly.

## Warnings

Non-blocking concerns for awareness:

- **Double-`setup()` behavior contradicts spec edge case** — The spec's edge case table (spec.md line 176–177) states: *"Settings stores are NOT reloaded on subsequent `setup()` calls (idempotent after first setup)."* However, the contract's `Editor::setup()` code (line 462–474) constructs a **new** `SettingsManager` on each call (via `settings_manager_ = std::make_unique<...>`), which reloads stores. The contract's own edge case (line 608) acknowledges this: *"Second call constructs a new SettingsManager (replacing old one). Stores are reloaded."* This inherited inconsistency should be resolved — either the spec's edge case is wrong (and should be updated to match the contract's behavior, which is more pragmatic), or the contract should align with the spec's text. Not blocking for this contract cycle.

- **`Connection` defined as top-level class vs spec's nested class** — The spec (spec.md line 255) defines `Connection` as a nested class of `SettingsStore`, accessed as `SettingsStore::Connection`. The contract (line 205) defines `Connection` as a separate class at namespace scope (`buddd::engine::Connection`). While this doesn't affect functionality (callers use `auto` from `observe()`), it's a deviation from the spec's API surface. The spec should ideally be updated to match, or the contract should nest the class. Minor — not blocking.

- **`PathsMatch` test helper referenced but not defined** — The contract's AC-014 test (line 552) references a `PathsMatch(matcher)` helper. This is not an existing Catch2 matcher in the project. The implementer will need to define it or use plain `REQUIRE(path.string() == expected)` instead. Minor documentation imprecision — not blocking.

## Required changes (all resolved)

1. **[RESOLVED] Fix `SerializationContext` forward-declaration in `settings_store.h` and `settings_manager.h`**: Both headers now include `"scene/component_registry/serialization_context.h"` directly. Verified.

## Suggested improvements

Optional ideas (not required):

- Consider updating the spec to match the contract's double-`setup()` behavior (reload on second call) rather than the current spec text (no reload). The contract's behavior is safer — a second `setup()` should reinitialize, not silently return.
- Consider updating the spec to define `Connection` as a top-level class (matching the contract) if this is the preferred design direction.
- Remove the `PathsMatch` reference from AC-014 test description, or document it as a helper the implementer should create.

## Re-review (13-Jun-2026)

**Outcome**: accepted

Verification of the SerializationContext include fix requested in Cycle 2:

- `settings_store.h` (line 198): `#include "scene/component_registry/serialization_context.h"` — ✅ present
- `settings_manager.h` (line 370): `#include "scene/component_registry/serialization_context.h"` — ✅ present
- No forward-declaration of `struct SerializationContext;` remains in either header — ✅ confirmed

**Final contract assessment**: The contract is complete, testable, spec-faithful, and ADR-compliant. All 32 ACs are mapped to test cases. The pimpl pattern for yaml-cpp (ADR-016) is correctly applied with template method definitions and explicit instantiations confined to `.cpp` files. No platform/graphics headers leak (ADR-019). ImGui integration follows ADR-026. Error handling uses existing `Error` categories (ADR-001). No non-goal violations detected. No forbidden files are modified.

Three non-blocking warnings remain (double-setup inconsistency, Connection nesting, PathsMatch helper) — none block acceptance of this contract.

## Resolution history

This section tracks review cycles across the contract's lifetime. Each cycle appends a new entry.

### Cycle 1 (previous review — pimpl fix verification)
- [x] **Observer exception safety** — Resolved. Copy-before-call + try-catch wrapping added.
- [x] **`YAML::Node root_` in public header violates ADR-016** — Resolved. Forward declaration + `std::unique_ptr<YAML::Node>` pimpl pattern applied.

### Cycle 2 — Fresh review after rework
- [x] **`SerializationContext` stored by value after forward declaration** — NEW. Blocking. Fixed in subsequent re-review cycle.

### Cycle 3 — Re-review after SerializationContext include fix (13-Jun-2026)
- [x] **`SerializationContext` include fix verified** — `settings_store.h` and `settings_manager.h` now include `serialization_context.h` directly. No forward declaration remains. Contract accepted.

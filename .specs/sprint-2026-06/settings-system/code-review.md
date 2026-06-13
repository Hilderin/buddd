# Implementation Contract Review — Settings System (MVP1 Skeleton)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- No blocking issues found.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **`double` explicit instantiation omitted** — The contract explicitly lists `double` in the explicit template instantiations (lines 350, 358) and states that TypeRegistry pre-registers `double`. In reality, TypeRegistry does NOT register `double` (confirmed in `register_builtin_types()` at `register_all_components.cpp`), and the implementer correctly omitted `double` from the explicit instantiations in `settings_store.cpp`. The behavior is correct — unregistered types return defaults — but this is a deviation from the contract's letter. Tests confirm this is intentional.

- **`find_node` returns `YAML::Node` by value (not `YAML::Node*`)** — The contract specifies `YAML::Node*` return types for `find_node`. The implementation returns `YAML::Node` by value, which is idiomatic for yaml-cpp (`YAML::Node` is a thin handle, not a heavyweight object). The pointer approach would risk dangling pointers. This is correct behavior but a deviation from the contract's explicit API.

- **`set_impl` does not use `ensure_node_path`** — The contract specifies `set()` should use `ensure_node_path(key)` to get the target node, then compare and assign. The implementation instead does inline node navigation using `YAML::Node::reset()`. This was necessary because `operator=` on `YAML::Node` calls `AssignNode` → `set_ref`, which corrupts the parent tree. The implementer discovered and fixed this yaml-cpp bug. The deviation is justified and the approach is correct.

- **AC-015 test is weak** — The contract requires a platform-guarded test (`#ifdef _WIN32` / `__APPLE__` / `__linux__`) verifying OS-appropriate path resolution. The test at `settings_store_tests.cpp:607` runs on all platforms without guards and only checks that OS config dir path ends with `"editor.yaml"`. It does not validate platform-specific behavior (XDG on Linux, Application Support on macOS, APPDATA on Windows). The `os_user_config_dir()` function IS correctly implemented, but the test coverage for AC-015 is weaker than the contract specified.

- **`Connection` as top-level class vs spec's nested `SettingsStore::Connection`** — The spec defines `Connection` as a nested class of `SettingsStore`. The implementation defines it as a top-level class in `buddd::engine` namespace. Since callers use `auto` from `observe()`, this has no practical impact, but it deviates from the spec's API surface. Already documented as a known deviation by the contract-critic.

## Required changes

Concrete, actionable changes requested:

- None.

## Suggested improvements

Optional ideas (not required):

- **Strengthen AC-015 test**: Add `#ifdef _WIN32` / `__APPLE__` / `__linux__` guards with platform-specific expected path checks (e.g., `REQUIRE(path.string().find(".config/buddd/editor.yaml") != std::string::npos)` on Linux, `REQUIRE(path.string().find("Application Support/buddd/editor.yaml") != std::string::npos)` on macOS, `REQUIRE(path.string().find("AppData/Roaming/buddd/editor.yaml") != std::string::npos)` on Windows). This would properly validate platform-specific path resolution.

- **Document yaml-cpp `reset()` vs `operator=` bug**: The subtle yaml-cpp bug where `operator=` on a `YAML::Node` handle calls `AssignNode` → `set_ref` and corrupts the parent tree, while `reset()` only replaces the handle's internal pointers, is critical knowledge. Consider adding a code comment or wiki page documenting this.

## AC-by-AC verification

| AC | Status | Test location | Notes |
|----|--------|---------------|-------|
| AC-001 | ✅ | `settings_store_tests.cpp:105` | Construct + load non-existent file → defaults |
| AC-002 | ✅ | `settings_store_tests.cpp:124` | Load valid YAML, get returns correct values |
| AC-003 | ✅ | `settings_store_tests.cpp:149` | Load from directory → `IoFailed` |
| AC-004 | ✅ | `settings_store_tests.cpp:163` | Load non-existent file succeeds |
| AC-005 | ✅ | `settings_store_tests.cpp:178` | Load malformed YAML → `InvalidFormat` |
| AC-006 | ✅ | `settings_store_tests.cpp:195` | Set keys, save, read back via YAML |
| AC-007 | ✅ | `settings_store_tests.cpp:224` | Save to non-writable path → `IoFailed` |
| AC-008 | ✅ | `settings_store_tests.cpp:245` | Save without changes is no-op |
| AC-009 | ✅ | `settings_store_tests.cpp:273` | Dirty state transitions (construct→load→set→save) |
| AC-010 | ✅ | `settings_store_tests.cpp:297` | Set/get all types via TypeRegistry |
| AC-011 | ✅ | `settings_store_tests.cpp:333` | Existing key returns value, missing returns default |
| AC-012 | ✅ | `settings_store_tests.cpp:356` | Observer fires on observed key, not on unrelated |
| AC-013 | ✅ | `settings_store_tests.cpp:387` | Two observers for same key both fire |
| AC-014 | ✅ | `settings_store_tests.cpp:410` | SettingsManager owns three stores |
| AC-015 | ⚠️ | `settings_store_tests.cpp:607` | Weak test — no platform guards, no specific path assertion |
| AC-016 | ✅ | `settings_store_tests.cpp:625` | Project path = `<cwd>/buddd.project.yaml` |
| AC-017 | ✅ | `settings_store_tests.cpp:645` | User project path via `editor_user_data_root` |
| AC-018 | ✅ | `settings_store_tests.cpp:663` | `load_all()` creates `.buddd/user/` |
| AC-019 | ✅ | `settings_store_tests.cpp:682` | `save_all()` writes dirty stores |
| AC-020 | ✅ | `settings_integration_tests.cpp:31` | Editor setup → .buddd/user/ exists + layout path set |
| AC-021 | ✅ | `settings_integration_tests.cpp:71` | Editor shutdown saves settings to disk |
| AC-022 | ✅ | Code review | No platform/graphics headers in settings headers |
| AC-023 | ✅ | `settings_store_tests.cpp:426` | Set same value does not mark dirty |
| AC-024 | ✅ | `settings_store_tests.cpp:451` | Nested dot-path keys round-trip correctly |
| AC-025 | ✅ | `settings_store_tests.cpp:487` | Connection destruction unregisters observer |
| AC-026 | ✅ | `settings_store_tests.cpp:89` | `editor_data_root()` returns `<root>/.buddd/` |
| AC-027 | ✅ | `settings_store_tests.cpp:95` | `editor_user_data_root()` returns `<root>/.buddd/user/` |
| AC-028 | ✅ | `settings_store_tests.cpp:515` | `get<bool>()` reads YAML boolean |
| AC-029 | ✅ | `settings_store_tests.cpp:533` | `get<int32_t>()` reads YAML int |
| AC-030 | ✅ | `settings_store_tests.cpp:551` | `get<float>()` reads YAML float |
| AC-031 | ✅ | `settings_store_tests.cpp:569` | `get<std::string>()` reads YAML string |
| AC-032 | ✅ | `settings_store_tests.cpp:587` | Unregistered type returns default + logs warning |

## Edge case coverage

| Edge case | Status | Location |
|-----------|--------|----------|
| Empty file (zero bytes) | ✅ | `settings_store_tests.cpp:725` |
| File with only comments | ✅ | `settings_store_tests.cpp:744` |
| Observer throws exception | ✅ | `settings_store_tests.cpp:761` |
| Connection move semantics | ✅ | `settings_store_tests.cpp:785` |
| Multiple consecutive `set()` on same key | ✅ | `settings_store_tests.cpp:810` |
| Dot-path with empty segments `"foo..bar"` | ✅ | `settings_store_tests.cpp:835` |
| `layout_ini_path()` correctness | ✅ | `settings_store_tests.cpp:853` |
| `double` unregistered type | ✅ | `settings_store_tests.cpp:866` |
| `shutdown()` without `setup()` | ✅ | `settings_integration_tests.cpp:119` (display) |
| Double `setup()` | ✅ | `settings_integration_tests.cpp:130` (display) |

## Architecture compliance

| ADR | Check | Status |
|-----|-------|--------|
| ADR-016 (yaml-cpp PRIVATE) | No yaml-cpp includes in .h files under `src/engine/settings/` or `src/engine/util/` | ✅ |
| ADR-019 (no platform/graphics outside engine) | `src/engine/settings/` and `src/engine/util/` headers include only stdlib headers | ✅ |
| ADR-001 (Result<T>) | All fallible operations return `Result<T>` | ✅ |
| ADR-011 ([[nodiscard]]) | All `Result<T>`-returning functions are `[[nodiscard]]` | ✅ |
| ADR-026 (ImGui lifecycle) | Only touch point is `IniFilename` via `layout_ini_path()` | ✅ |

## Files verification

| File | Status | Notes |
|------|--------|-------|
| `src/engine/util/os_config_dir.h` | ✅ Created | Correct API, stdlib only |
| `src/engine/util/os_config_dir.cpp` | ✅ Created | Platform-specific `getenv` |
| `src/engine/util/editor_data_root.h` | ✅ Created | Correct API |
| `src/engine/util/editor_data_root.cpp` | ✅ Created | Simple delegation |
| `src/engine/settings/settings_store.h` | ✅ Created | Pimpl pattern, no yaml-cpp in header |
| `src/engine/settings/settings_store.cpp` | ✅ Created | yaml-cpp + TypeRegistry confined to .cpp |
| `src/engine/settings/settings_manager.h` | ✅ Created | Persistent `ini_path_` for lifetime safety |
| `src/engine/settings/settings_manager.cpp` | ✅ Created | Path resolution per spec |
| `src/editor/editor.h` | ✅ Modified | Added include, member, accessor |
| `src/editor/editor.cpp` | ✅ Modified | Old INI block removed, setup/shutdown integrated |
| `tests/engine/settings_store_tests.cpp` | ✅ Created | 879 lines, 25 test cases |
| `tests/editor/settings_integration_tests.cpp` | ✅ Created | Guarded with `BUDDD_HAS_DISPLAY` |
| Forbidden files (`error.h`, CMakeLists.txt, TypeRegistry headers) | ✅ Unchanged | None modified |

## Build warnings

Build produces zero warnings in our code (`src/` and `tests/`). ✅

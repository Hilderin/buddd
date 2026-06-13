# Governance Review — Settings System (SPEC-036 / IMPL-036)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **Connection nesting: spec defines `SettingsStore::Connection` (nested), contract/code implement `buddd::engine::Connection` (top-level)** — The spec (spec.md line 255) defines `Connection` as a nested class inside `SettingsStore`. The implementation contract (contract lines 204–216) and the actual code define it as a top-level class at namespace scope. This is documented by the contract-critic and code-review as a non-blocking deviation. Callers use `auto` from `observe()` so there is no practical impact, but the spec should be updated to match the contract/code. Wiki correctly documents it as a top-level class.

- [ ] **`find_node` return type: contract specifies `YAML::Node*`, code returns `YAML::Node` by value** — The contract (lines 243–245) specifies `auto find_node(const std::string& key) -> YAML::Node*`. The implementation returns `YAML::Node` by value, which is idiomatic for yaml-cpp (a thin handle type). This is a documented, justified deviation with no functional impact.

- [ ] **`set_impl` vs `ensure_node_path`: contract specifies `ensure_node_path(key)`, code uses inline `Node::reset()` navigation** — The implementer discovered a yaml-cpp bug where `Node::operator=` calls `AssignNode` → `set_ref` which corrupts the parent tree. The fix uses `Node::reset()` instead. This deviation is justified, documented in code-review, and critical to correctness.

- [ ] **`double` explicit instantiation: contract lists `double`, TypeRegistry does NOT register `double`, implementer correctly omitted it** — The contract (lines 349, 350, 355, 356) explicitly includes `double` in both `get` and `set` template instantiations. However, `register_builtin_types()` in `register_all_components.cpp` does NOT register `double` — only `float` is registered. The implementer correctly omitted `double` from the explicit instantiations. Tests confirm `get<double>()` returns default with warning (AC-032 covers this). Code-review and wiki both record this correctly.

- [ ] **Double-`setup()` behavior: spec edge case contradicts contract/code** — The spec's edge case table (spec.md line 176–177) states: *"Settings stores are NOT reloaded on subsequent `setup()` calls (idempotent after first setup)."* However, the contract's `Editor::setup()` code constructs a **new** `SettingsManager` on each call, which reloads stores. The contract-critic flagged this as an inherited inconsistency. The contract's behavior is safer (re-initialize on second call) and should be the spec's final word.

- [ ] **AC-015 test weaker than contract specifies** — The contract (line 549) requires a platform-guarded test (`#ifdef _WIN32` / `__APPLE__` / `__linux__`) that verifies OS-appropriate path resolution. The test at `settings_store_tests.cpp:607` runs on all platforms without guards and only checks that the path ends with `"editor.yaml"`. It does not validate platform-specific behavior (XDG on Linux, Application Support on macOS, APPDATA on Windows). The `os_user_config_dir()` function IS correctly implemented, but the test coverage is weaker than specified.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result<T>)** — All fallible operations (`load()`, `save()`) return `Result<void>`. Errors use existing `Error::Category` values (`IoFailed`, `InvalidFormat`). yaml-cpp exceptions are caught and converted to `Result` errors. ✅ Compliant.

- [x] **ADR-016 (yaml-cpp PRIVATE dependency)** — yaml-cpp is fully confined to `.cpp` files via pimpl pattern (`std::unique_ptr<YAML::Node>` in `settings_store.h`, no yaml-cpp includes in headers). `TypeRegistry.h` (which includes yaml-cpp) is only included in `settings_store.cpp`. ✅ Compliant.

- [x] **ADR-019 (architecture boundaries)** — `src/engine/settings/` and `src/engine/util/` headers include only `std::filesystem`, `<cstdlib>`, `<functional>`, `<memory>`, `<string>`, and standard library headers. No SDL3, OpenGL, GLM, or `render/` headers. ✅ Compliant. Verified by code-review.

- [x] **ADR-026 (ImGui lifecycle)** — The only ImGui touch point is `SettingsManager::layout_ini_path()` providing the `.c_str()` persistent pointer for `ImGui::GetIO().IniFilename`. SettingsManager does NOT init/shutdown ImGui. ✅ Compliant.

- [x] **ADR-011 ([[nodiscard]])** — All `Result<T>`-returning functions are `[[nodiscard]]`. Accessors returning references are also `[[nodiscard]]` per the contract. ✅ Compliant.

- [x] **ADR-007 (Release dependency build)** — yaml-cpp is built in Release mode via `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` (inherited, no new configuration needed). ✅ Compliant.

- [x] **ADR-028 (TypeRegistry / SerializationContext)** — `SettingsStore` takes `SerializationContext` and delegates all type conversions to `TypeRegistry::yaml_encode<T>()` / `yaml_decode<T>()`. This follows the existing pattern established by ADR-028 component serialization. ✅ Compliant.

- [x] **No new ADR required** — The implementation is fully constrained by existing ADRs. No new architectural decisions were introduced. ✅ Verified.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **Wiki page `docs/wiki/editor/settings-system.md`** — Correctly documents the three tiers, YAML storage format, file locations, TypeRegistry integration, observer pattern, editor lifecycle, and key classes. Correctly notes `Connection` as a top-level class (matching contract/code). Correctly lists explicit template instantiations as `bool`, `int32_t`, `float`, `std::string` (minus `double` which is not registered). ✅ Matches implementation.

- [x] **Wiki page `docs/wiki/architecture/module-map.md`** — Correctly documents `util/` and `settings/` submodules with accurate file-by-file role tables. References SPEC-036 correctly. ✅ Matches implementation.

- [x] **Wiki page `docs/wiki/architecture/overview.md`** — Correctly shows `util/` and `settings/` in the directory layout and engine internal structure trees. ✅ Matches implementation.

- [x] **Wiki page `docs/wiki/decisions/adr-index.md`** — Added SPEC-036 entry documenting the settings system design. ✅ Accurate.

- [x] **Wiki does NOT become law** — All wiki pages reference SPEC-036 and the implementation contract as authoritative sources. Wiki follows the established pattern of operational documentation that defers to specs/ADRs for authority. ✅

## Warnings

Non-blocking concerns for awareness:

- **Spec edge case contradicts contract for double-`setup()` behavior** — Spec says stores are not reloaded on second `setup()`, contract constructs new `SettingsManager` (reloads). The spec edge case should be updated to match the contract's behavior, which is safer (re-initialize on second call). Already documented by contract-critic.
  
- **`Connection` nesting deviation from spec** — Spec defines `Connection` as nested `SettingsStore::Connection`, contract/code implement it as top-level `buddd::engine::Connection`. Wiki correctly documents the top-level version. The spec should be updated for consistency.

- **AC-015 test is weaker than contract specification** — Contract requires platform-guarded tests with specific OS path checks. The actual test runs without platform guards and only checks that the path ends with `"editor.yaml"`. The implementation of `os_user_config_dir()` is correct, but the test does not validate platform-specific behavior as originally specified.

- **`find_node` and `set_impl` minor contract deviations** — Contract specified `YAML::Node*` return types and `ensure_node_path()`, but the implementation uses `YAML::Node` by value (idiomatic for yaml-cpp) and inline `Node::reset()` navigation (fixing an actual yaml-cpp parent-tree corruption bug). Both deviations are justified and documented.

- **`double` instantiation listed in contract but not registered** — Contract includes `double` in explicit template instantiations, but TypeRegistry does not register `double`. The implementer correctly omitted `double` — the behavior (unregistered type returns default) is correct and tested via AC-032. The contract letter is slightly inaccurate but the outcome is correct.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- **Spec update (optional, recommended)**: Update spec.md to define `Connection` as a top-level class (matching contract/code) and update the double-`setup()` edge case to match the contract's reload behavior.
- **Contract update (optional, recommended)**: Remove `double` from explicit instantiation list in contract, or add a note explaining that `double` is not registered in TypeRegistry and will behave as an unregistered type.
- **AC-015 test strengthening (optional, recommended)**: Add `#ifdef`-guarded platform-specific path checks to the AC-015 test so it validates OS-appropriate config directory resolution, matching the contract's specification.

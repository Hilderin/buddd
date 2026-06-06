# Implementation Contract Review — IMPL-022 Logging System Adoption

## Re-review (Loop #2) — 2026-06-06

All 8 blocking issues from the previous review have been resolved in the updated contract. No new issues found. The contract is now acceptable.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **`asset_manager.cpp` and `asset_manager.tpp` both declare `BUDDD_LOG_TAG("Asset")` → redefinition error.** RESOLVED: Contract now explicitly says NOT to add the tag/include to `asset_manager.cpp` — only in `asset_manager.tpp`.

- [x] **`make_checkerboard_texture` (phong_app.cpp) `std::exit()` removal causes UB.** RESOLVED: Contract now says KEEP `std::exit()` — function returns non-`Result` type, no valid non-terminating return path.

- [x] **`make_solid_texture` (phong_app.cpp) `std::exit()` removal causes UB.** RESOLVED: Same fix as above — KEEP `std::exit()`.

- [x] **`phong_material.cpp` `Impl::create_material` `std::exit()` removal causes UB.** RESOLVED: Contract now says KEEP `std::exit()` for all 3 FATAL blocks (dereferences `*vs`/`*fs`/`*mat` on failed Results).

- [x] **`pbr_material.cpp` `PbrMaterial::Impl::create_material` `std::exit()` removal causes UB.** RESOLVED: Same fix — KEEP `std::exit()` for all 3 FATAL blocks.

- [x] **`render_device_opengl.cpp` `fallback_material()` `std::terminate()` removal causes UB.** RESOLVED: Contract now says KEEP `std::terminate()` — truly unrecoverable per AC-016 exception.

- [x] **`render_device_headless.cpp` `fallback_material()` `std::terminate()` removal causes UB.** RESOLVED: Same fix — KEEP `std::terminate()`.

- [x] **Missing test adaptation for "buddd with no arguments defaults to run command".** RESOLVED: Section C.6 now explicitly adds this test adaptation — change assertion to check stderr for `"[INFO] [App] Window opened: 1024x768"`.

## Warnings

Non-blocking concerns for awareness:

- **"Warning: unexpected arguments" format string ambiguity** (main.cpp lines 152–155). The contract says to collapse three `fprintf` calls (prefix + loop arguments + trailing `\n`) into a single `BUDDD_LOG_WARN` with `"Warning: unexpected arguments after '{}': {} ..."`. The `...` placeholder is vague — the contract should clarify that the implementation must build a single string from the loop arguments (e.g., `std::format` or string concatenation) before passing to the macro. Implementers will need to figure this out.

- **Minor formatting change for Unknown command/scene errors**. The original `fprintf(stderr, "Unknown command: '%s'\n\n", ...)` had a double newline `\n\n` which produced a blank line between the error and the usage text. After migration, the trailing newlines are removed (per spec) and ConsoleSink adds a single `\n`, eliminating the blank line separator. This is consistent with the spec's trailing `\n` removal rule, but is a minor visual difference. Test assertions using `find()` will still pass because they check substring presence, not formatting.

## Required changes

Concrete, actionable changes requested:

1. Fix the `asset_manager.tpp` / `asset_manager.cpp` `BUDDD_LOG_TAG` duplication — declare only in the `.tpp`.
2. Document that `make_checkerboard_texture` and `make_solid_texture` (phong_app.cpp) keep `std::exit()` (like `create_phong_cube`), or specify alternative control flow (`return nullptr`).
3. Document that `phong_material.cpp` `Impl::create_material` keeps `std::exit()` (truly unrecoverable error in library utility code).
4. Document that `pbr_material.cpp` `PbrMaterial::Impl::create_material` keeps `std::exit()` (same reason).
5. Document that `render_device_opengl.cpp` `fallback_material()` keeps `std::terminate()` (truly unrecoverable — AC-016 clause).
6. Document that `render_device_headless.cpp` `fallback_material()` keeps `std::terminate()` (same reason).
7. Add test adaptation for "buddd with no arguments defaults to run command" — update to check stderr for "Window opened" message.

## Suggested improvements

Optional ideas (not required):

- Clarify the "Warning: unexpected arguments" migration by specifying the argument concatenation strategy (e.g., build a single string with `std::format` or `fmt::join`-like approach).
- After fixing the above blocking issues, re-verify that the `#include <iostream>` removal in `asset_manager.cpp` is still valid. The file uses `<iostream>` only for `std::cerr`, so removing it is correct after all `std::cerr` calls are replaced with `BUDDD_LOG_*` macros.

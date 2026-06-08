# Spec Review — Window Resize for All Apps (Re-review)

*Previous review: rejected. Updated spec expected to address all issues.*

## Blocking issues

- [x] DoR criterion: Existing documentation to update not listed — spec now includes a "Documentation updates" section listing `module-map.md`, `data-flow.md`, and a general wiki audit note.

## Warnings (all previously raised, now resolved)

- [x] on_resize signature ambiguous (pure virtual `= 0` vs default body) — AC-001 and Key Entities now show `= 0` explicitly.
- [x] No explicit Dependencies section — dependencies remain adequately identified across Actors, Assumptions, and Key Entities. Acceptable as-is.
- [x] EC-04: no guarantee for negative on_resize() in headless — now includes an implementation note about bounds-checking responsibility.
- [x] Key Entities entry for PlatformSDL3 omits MAXIMIZED/RESTORED event handling — now includes `SDL_EVENT_WINDOW_MAXIMIZED` and `SDL_EVENT_WINDOW_RESTORED`.
- [x] windowID map has no un-registration on Window destruction — now includes un-registration via `PlatformSDL3::unregister_window()`.

## Required changes

All previously requested changes have been implemented. No new required changes.

## Suggested improvements

None.

---

## Re-review outcome

All Definition of Ready criteria are now satisfied:

| Criterion | Status |
|---|---|
| Clarity & Completeness | ✅ Scope (Goals + Non-goals + Out of scope), dependencies (Actors + Assumptions), 8 edge cases, 3 error cases |
| Verification | ✅ 11 ACs with specific verifications, 3 E2E verification methods, 5 success criteria |
| Documentation | ✅ Interface changes documented in Key Entities + AC-001; existing docs to update listed in "Documentation updates" section |
| Technical | ✅ Technical constraints (SDL3 APIs, ImGui, OpenGL), risks (assumptions section), performance noted (O(1)) |

**All issues resolved — spec is ready for the next workflow step.**

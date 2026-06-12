# Spec Review — Editor UX Design (North-Star)

## Blocking issues

Items that must be resolved before the spec can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

- [x] **Project panel placement is undefined** — The spec describes the Project Panel as "a persistent utility panel visible in all tab types" (lines 135-136) and the View menu lists it among toggleable panels (line 192), but the Scene Tab Layout diagram (lines 417-443) and panel structure description (lines 445-451) omit it entirely. Only 5 panels are shown (Hierarchy, Viewport, Inspector, Console, Assets). The spec never states where the Project panel lives within the dockspace, whether it is tabbed with another panel (e.g., Assets), or whether it appears as a separate floating panel. This is a **contradiction** that must be resolved before any implementation can proceed — the layout is defined as "fixed, pre-defined" and "users cannot add or remove panels from a tab type's layout" (line 98-100), yet a panel described as always present has no assigned position.
- [x] **AC-003: Child ordering on entity creation is unspecified** — When a new entity is created as a child of a selected entity, the spec says it becomes "its child" but does not specify where in the child list it is inserted (first child, last child, or relative to existing children). The entity hierarchy is ordered, so insertion position is an observable behavior that must be defined for testability.
- [x] **No list of documentation to update** — The Definition of Ready requires that "Existing documentation that must be updated is listed." The spec does not identify which wiki pages, ADRs, or other specs need revision when this feature is implemented. At minimum, the architecture overview wiki page and ADR-027 (Editor Architecture) should be noted for future updates when panels, tabs, and play mode are added.
- [x] **World clone prerequisite is unverified and unimplemented** — Assumption A-03 states "The engine's `World` class supports deep cloning… If not already implemented, this must be added as a prerequisite." A search of the engine source confirms that no `World::clone()` or deep-copy mechanism exists. Play mode (Stories 5, 9 and AC-010 through AC-014) depends entirely on this capability. The spec must either: (a) verify that this prerequisite exists, (b) define it as a tracked dependency with a separate feature spec, or (c) explicitly scope Play mode as dependent on an external prerequisite that must be satisfied first.
- [x] **AC-010: Visual Play mode indicator is underspecified** — The "visual indicator" for Play mode is described with illustrative language ("Colored border or tint… e.g., orange/red border," line 356) rather than a definitive specification. The exact visual treatment (color, thickness, animation if any, which UI elements are affected) must be defined to be testable. A tester cannot verify "a visual Play mode indicator" without knowing exactly what it looks like.

### Resolution verification (re-review, 2026-06-11)

All 5 previous blocking issues have been resolved in the updated spec:

1. **Project panel placement** — Now explicitly defined as a tab in the bottom panel area alongside Console and Assets (line 136). The Scene Tab Layout diagram (line 439) shows `[Project] [Console] [Assets]` as bottom tabs, and the panel structure description (lines 450-454) describes the three-tab bottom bar consistently. All tab types that include the bottom area (Scene, Prefab) reference this triad.

2. **AC-003 child ordering** — AC-003 (line 778) now states "appended as the **last child** of the selected entity." The Create Empty operation description (line 230) says "appended at the end of the child list." Story 2 (line 602) also specifies "last child… appended at the end of the root list." Consistent and definitive.

3. **Documentation update list** — A dedicated "Documentation to update" section (lines 953-964) lists four documents: `docs/wiki/architecture/overview.md`, `docs/wiki/engineering/setup.md`, `docs/adr/ADR-027-editor-architecture.md`, and a new `docs/wiki/editor/editor-panels.md`. The section correctly notes these updates are tracked for the implementation phase.

4. **World clone prerequisite** — A-03 (line 941) now explicitly states: "**⚠️ PREREQUISITE NOT YET IMPLEMENTED:** A search of the engine source confirms that no `World::clone()` or deep-copy mechanism exists. This is a required engine-level prerequisite that must be implemented before Play mode can function. A separate feature spec must be created for `World::clone()` before any Play mode implementation begins." The Key entities section (line 563) also flags this exception.

5. **AC-010 Play mode indicator** — AC-010 (line 785) now provides a definitive specification: viewport border `#FF3300` 3px, title bar `[Playing]` prefix, status bar "🔴 PLAY MODE" on dark-red background, and Inspector read-only with gray background and lock icons. The Play Mode section (lines 355-360) reinforces the same definitive details.

**No new blocking issues found.** The spec satisfies all Definition of Ready criteria.

## Warnings

Non-blocking concerns for awareness:

- **Detached tabs risk (from previous review)** — Detached tabs (Story 9) require OS-level windows with their own GL contexts (A-11). ADR-026 explicitly notes "No multi-viewport support in this scope." While A-11 uses SDL windows rather than ImGui ViewportsEnable, the technical complexity of managing multiple GL contexts, input routing, and cross-window state synchronization is significant for MVP1. Consider deferring to post-MVP1 if effort exceeds initial estimates.
- **Undo/Redo menu labeling** — The Edit menu shows "Undo" and "Redo" (line 191) with a note explaining the single-level entity-deletion-only scope. The generic Ctrl+Z/Ctrl+Shift+Z shortcuts may still users who expect standard multi-level undo. Consider renaming to "Undo Delete" or adding a tooltip.
- **Performance budgets remain aspirational** — SC-003 and SC-004 are now annotated as aspirational (lines 835-836), but lack concrete resource budgets or a defined test environment. This is acceptable for a north-star UX spec but will need refinement during implementation.
- **AC-024 panel size persistence scope** — "New sizes are remembered for the session" is stated, but it remains ambiguous whether this means: (a) shared across all tabs of the same type, (b) per-tab-instance, or (c) per-tab-type with session-only lifetime. The spec text (line 94-95) clarifies "per tab type" and "session only," which is sufficient for a north-star spec.
- **Story 2 minor ambiguity** — Story 2 (line 602) says "if one is selected" which could be read as "if any entity is selected" vs "if exactly one entity is selected." The spec text (line 230) and AC-003 (line 778) clarify this as "if a single entity is selected / multiple entities → root level." Not a blocker, but Story 2 could be tightened for consistency.
- **Permissions scope on file system access** — Line 878 states "The editor accesses the file system only within the project directory," but File > Open Scene and Save Scene As use OS-native file dialogs (unrestricted), allowing user navigation outside the project. This is a minor imprecision — the intent is clearly that the editor operates within the project, and the OS dialog is a standard UX pattern.

## Required changes

None. All previous blocking issues have been resolved. No new required changes identified.

## Suggested improvements

Optional ideas (not required):

- **Consider an explicit "Prerequisites" section** — The spec currently scatters dependencies across assumptions (A-01 to A-13). A consolidated "Prerequisites" section listing engine capabilities the editor depends on and their implementation status (World::clone, ComponentRegistry introspection, ImGuizmo, OS file dialogs) would serve as a single dependency manifest for implementers.
- **Strengthen SC-002** — "Restores the editor World to its exact pre-Play state 100% of the time" could be strengthened by defining what "exact pre-Play state" means: entity IDs, transform values, component property values, hierarchy structure, etc.
- **Consider a "Tab lifecycle" state diagram** — A diagram showing the full lifecycle of each tab type (Scene: always-present; Prefab: open → dirty → save → close; Game: open on Play → close on Stop) would benefit implementers.
- **Component type name in YAML prefab format** — The Prefab section (line 387) assumes prefabs are YAML files with `type: Prefab`. Cross-referencing the existing SceneLoader/SceneSaver YAML format would confirm whether this format is already supported.

# Spec Review — auto-rename-on-create

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Missing documentation impact listing (DoR: Documentation)** — The spec does not list existing documentation that must be updated. The wiki at `docs/wiki/editor/editor-panels.md` currently says the new entity is "Not auto-selected" for Create Empty (line 335) and documents entity operations behavior that this spec overrides. The spec should either add a "Documentation impact" section (like F-04 does at line 468) or explicitly list files to update. The coordination.md workflow accounts for a wiki-agent, but the spec itself fails this DoR criterion.
> **Resolved**: Documentation impact section added at lines 250-257, listing `docs/wiki/editor/editor-panels.md` with specific update reasons.

## Warnings

Non-blocking concerns for awareness:

- **Dependencies are scattered, not enumerated in one place** — Dependencies on F-04 entity-operations, F-03 entity-selection, `CreateEntityCommand`, `RenameEntityCommand`, `CommandStack`, `EditorSelection`, and ImGui are identified throughout the spec text but never listed in a dedicated Dependencies section. Consider adding one for clarity.
- **Informal language in edge case description** — Edge case "Create entity, type name, then click empty area" (line 218) contains an internal thought note: "(leaves new entity selected... actually empty-area click clears selection)." This should be cleaned up to state the final expected behavior definitively.
- **Auto-rename mode distinction mechanism not explicitly described** — A-02 says `confirm_rename()` must "check whether it is operating in auto-rename mode (post-creation) vs. regular rename (F2-initiated)" but the spec doesn't describe what flag or state variable enables this distinction (e.g., a `bool auto_rename_mode_` flag or a `CreateEntityCommand* pending_cmd_` pointer). Consider adding a state description to the "Auto-rename state" section or in Assumptions.

## Required changes

Concrete, actionable changes requested:

- ~~Add a **Documentation impact** section listing `docs/wiki/editor/editor-panels.md` as needing updates (specifically the "Entity Operations" table and the "Create Empty" description in the F-04 additions paragraph).~~
  ✅ Resolved — Documentation impact section added at lines 250-257.

## Suggested improvements

Optional ideas (not required):

- Consider adding an explicit **Dependencies** table early in the spec for clarity (F-04, F-03, ImGui, `CreateEntityCommand`, `RenameEntityCommand`, `CommandStack`, `EditorSelection`).
- Consider adding a sentence about the new state variable in ScenePanel that tracks auto-rename mode (e.g., `CreateEntityCommand* pending_auto_rename_cmd_` or similar) for implementer clarity.
- The "Escape discards" edge case (line 216) could be slightly clearer: "If the user typed a name first and then presses Escape, the entity is discarded regardless of buffer contents." This is correct but could be stated more directly.

# Spec Review — Color Type

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **DoR: Existing documentation update list missing** — FIXED: spec now includes a "Document updates" section (lines 292-301) listing `docs/wiki/architecture/module-map.md` (TypeRegistry section), `docs/wiki/editor/editor-panels.md` (Inspector Property Editors table), `docs/adr/ADR-002-glm-wrapper-math.md` (no changes needed), and `docs/adr/ADR-028-component-type-registry.md` (no changes needed), each with the specific update action and rationale.

## Warnings

Non-blocking concerns for awareness:

- [x] **Naming inconsistency (`PropertyFlag::HideAlpha` vs `bool hide_alpha`)** — FIXED: spec now uses the string-based tag system consistently throughout (`std::vector<std::string> tags_` + `.tag("rgb")` fluent setter, `has_tag("rgb")` checker). No enum-style or bool references remain.

- [x] **HSV epsilon unspecified** — FIXED: HSV roundtrip epsilon is now explicitly defined as `1e-5f` in the spec (line 107) and recorded in coordination.md decision #10.

- **AC-011 manual verification dependency** — AC-011's editor registration part is unit-testable; the ColorEdit3 vs ColorEdit4 rendering decision based on `has_tag("rgb")` relies on ImGui rendering which is inherently manual. This is acceptable for UI features but cannot be fully verified in CI without a human step or screenshot-based test.

- **PropertyFlags compatibility risk** — Adding a `std::vector<std::string> tags_` member to `PropertyFlags` is safe for aggregate initialization `PropertyFlags{}` and fluent `PropertyFlags{}.min(...)` patterns. However, any existing code using explicit positional brace initialization matching the old field count could break structurally. This is a known and accepted risk (documented in assumptions).

## Required changes

All previously requested changes have been addressed:

1. [x] **Documentation update section** — Added as "Document updates" (lines 292-301) covering `module-map.md`, `editor-panels.md`, ADR-002, and ADR-028.
2. [x] **Naming consistency** — Resolved by adopting the string-based tag system throughout.

## Suggested improvements

Optional ideas (not required):

- The glossary (`docs/wiki/domain/glossary.md`) still says "Eight built-in types" in its TypeRegistry entry. The spec's "Document updates" section could include this file for completeness, though the wiki-agent will likely handle it.
- Consider cross-referencing the `.specs/sprint-2026-06/inspector-transform/spec.md` for the `InspectorTypeEditor` pattern to clarify how the Color editor registration works.

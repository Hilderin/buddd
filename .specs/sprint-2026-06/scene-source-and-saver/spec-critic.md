# Spec Review — Scene Source Tracking and Saver

**Re-review (2026-06-11): Both previously blocking issues are confirmed fixed. No new blocking issues found. The spec is approved.**

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Contradiction: Component serialization failure behavior** — Previously the Error cases said "entire save fails" while Observability said "skip and warn". Now both agree: error is logged at ERROR level and propagated; entire save fails. (Resolved in spec v2.)

- [x] **Edge case table indicates unresolved decision already resolved by Q01** — The edge case row now reads "`components:` key is omitted entirely (resolved in Q01)." Decision is no longer left open. (Resolved in spec v2.)

## Warnings

Non-blocking concerns for awareness:

- ✅ **Pending-destroy entities** — Now addressed in the edge case table (line 291): skipped from output. Caller should flush before saving if undesired. (Resolved in spec v2.)

- ⚠️ **Mapping `Component*` to `ComponentInfoBase*` is assumed but not detailed** — R-05 in the risk table now documents this gap and states "the implementation must add a mechanism." This is sufficient for the spec; the implementation contract will need to address this concretely.

- ⚠️ **No priority mapping on acceptance criteria** — User stories 1–4 are P1, stories 5–6 are P2, but the 19 acceptance criteria carry no priority labels. This makes it unclear which ACs are minimum-viable vs. stretch. Consider adding a Priority column to the AC table.

## Required changes

All previously requested changes have been addressed:

1. **Resolve the serialization-failure contradiction** — ✅ DONE: Both Error cases (line 300) and Observability (line 316) now agree that component serialization failure logs ERROR and propagates to fail the entire save.
2. **Update the edge case table to match Q01** — ✅ DONE: Edge case row now reads "`components:` key is omitted entirely (resolved in Q01)."
3. **Add priority labels to acceptance criteria** — ⚠️ Still a suggestion (non-blocking). ACs remain unlabeled with P1/P2 priority.

## Suggested improvements

Optional ideas (not required):

- Consider adding an edge case for pending-destroy entities being excluded from save output.
- Consider specifying whether the `SceneSaver` should call `flush_destroyed()` internally or whether the caller is responsible.
- The "Component serialization failure" error case could be split into two distinct scenarios: (a) unregistered component type → fail the save, and (b) property serialization error within a registered component → skip that component with a warning. This would reconcile the current contradiction.

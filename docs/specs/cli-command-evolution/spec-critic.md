# SPEC-007 — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### B-01 — Window title case inconsistency (Story 1 / AC-007 vs. User-visible behavior)

The User-visible behavior section (line 114) states:

> Window title: `"Buddd Engine — Demo: <name>"` where `<name>` is the demo name as typed (preserving case).

When the user types `buddd demo triangle`, the demo name is `triangle` (lowercase). However, Story 1 (line 293) and AC-007 (line 385) show the title as `"Buddd Engine — Demo: Triangle"` with a capital **T**. These are contradictory — the acceptance criterion does not match the behavioural specification.

**Impact**: AC-007 cannot be reliably tested because the expected title is ambiguous (capitalised vs. case-preserved). An implementer who follows the case-preservation rule will produce `"Buddd Engine — Demo: triangle"` and fail the AC-007 check as written.

**Required resolution**: Either:
- (a) Fix Story 1 (line 293) and AC-007 (line 385) to use lowercase `triangle` in the title, matching the case-preservation rule; or
- (b) Amend the case-preservation rule (line 114) if the design intent is to capitalise demo names in the title.

---

### B-02 — CONST-002 violation: new tests marked "optional"

The spec's "Test implications" section (line 487) labels new tests as "optional but recommended":

> | Test | Condition | Description |
> |---|---|---|
> | `buddd demo` with no name | Always runs (no display needed) | Verify stderr contains demo usage; exit code is 1. |
> | `buddd demo unknownname` | Always runs (no display needed) | Verify stderr contains `"Unknown demo:"`; exit code is 1. |
> | `buddd test` is unknown command | Always runs (no display needed) | Verify stderr contains `"Unknown command: 'test'"`; exit code is 1. |
> | `buddd demo triangle` | Guarded by `BUDDD_HAS_DISPLAY` | Verify the demo window opens and completes (with timeout). |

However, **CONST-002** (Testing Policy) states:

> "All testable code added or modified in this project must have corresponding unit tests. Those tests must pass (i.e., the code must work)."
> **Exceptions**: None.

At least three of the four listed tests (all except the display-dependent `buddd demo triangle` test) are unconditionally testable without a display. Under CONST-002 they are **required**, not optional. The "optional" label directly contradicts a blocking constitution rule.

The spec does partially acknowledge CONST-002 on line 496, but the "optional" label on the table header (line 487) undermines that acknowledgment and creates internal contradiction within the spec itself.

**Required resolution**: Either:
- (a) Remove "optional" from the test table header and explicitly state that these tests are required by CONST-002; or
- (b) Provide a documented rationale explaining why these code paths are exempt from CONST-002 (noting that CONST-002 currently lists no exceptions).

---

### B-03 — Per-demo function `argv` contract contradiction

The per-demo function interface documentation (lines 234–236) states:

> ```cpp
> /// @param argc      Argument count (including the demo name as argv[0]).
> /// @param argv      Argument vector (argv[0] is the demo name).
> ```

However, the `DemoCommand::run()` dispatch (line 569) passes the **unmodified** `argc`/`argv` from the original command-line invocation:

> ```
> "triangle" → call run_triangle_demo(*platform, *device, argc, argv), return its result
> ```

In this call, `argv[0]` is the program name (e.g., `"buddd"`), `argv[1]` is the subcommand (`"demo"`), and `argv[2]` is the demo name. The doc comment's assertion that `argv[0]` is the demo name is **false** under this dispatch. These two statements are contradictory and cannot both be true.

**Impact**: An implementer who follows the doc comment literally would expect `argv[0]` to be the demo name, but the pseudocode never constructs such an argv. Either the dispatch must construct a sub-argv (`argv + 2`, `argc - 2`), or the doc comment is wrong and must be corrected to describe the real argv layout.

**Required resolution**: Reconcile the interface contract with the dispatch pseudocode. Choose one:
- **Option A**: Update the doc comment to reflect that the demo function receives the full `argc`/`argv` from the command line, and `argv[2]` is the demo name.
- **Option B**: Update the `DemoCommand` dispatch to pass `argc - 2, argv + 2` so that `argv[0]` becomes the demo name as documented, and adjust the extra-arguments warning to iterate from the new `argv[1]`.

---

## Warnings

Non-blocking concerns for awareness:

### W-01 — Framebuffer clear operation is undefined

The spec says `RunCommand` "clears the framebuffer to black" (line 166) and the implementation section (lines 578–581) replaces the `draw()` call with:

```cpp
(*device)->begin_frame();
// framebuffer clear (no draw calls)
(*device)->end_frame();
```

The current `RenderDevice` interface (`src/engine/render/render_device.h`) does **not** expose a `clear()` or `clear_color()` method — only `begin_frame()`, `end_frame()`, `draw()`, `draw_indexed()`, and resource factories. If `begin_frame()` does not implicitly clear the framebuffer, the implementation cannot satisfy the spec without either:

- Adding a clear method to the engine, which contradicts the non-goal of "No changes to the engine library" (line 64–65); or
- Drawing a full-screen quad with a black material, which contradicts "no draw calls" (line 166).

The spec should document what mechanism achieves the framebuffer clear (e.g., "`begin_frame()` clears the colour buffer to black as an implementation detail of the OpenGL backend"). If this is not the case, either the non-goal or the behavior requirement must change.

---

### W-02 — Extra-arguments warning printed before resource creation

The `DemoCommand` implementation pseudocode (steps 3–4, lines 565–568) prints the extra-arguments warning **before** creating the platform, window, and render device. If resource creation fails (e.g., no display available), the user sees:

```
Warning: unexpected arguments after 'demo triangle': extra1 extra2
Failed to create platform: ...
```

The warning about extra arguments is misleading when the command is about to fail for an unrelated reason. Consider moving the warning to after successful resource creation, or document this ordering as intentional (with a note about the UX trade-off).

---

### W-03 — `buddd run extra_arg` missing from edge case table

The edge case table (lines 413–433) explicitly lists extra-argument behavior for `buddd version extra_arg` and `buddd help extra_arg` but does not list `buddd run extra_arg`. The specification for `RunCommand` (line 169) says "Extra arguments: silently ignored" — so the behavior is defined, but it is not tested or enumerated alongside the sibling commands. Add it for symmetry and testability.

---

### W-04 — `RunCommand.h` class comment not updated

The spec documents `.cpp` changes for `RunCommand` (lines 574–581) but does not mention updating `run_command.h`. The current file (`src/cmd/commands/run_command.h`, line 8) says:

> "Opens an interactive window (1024×768, title "Buddd Engine") and renders a coloured triangle until the user closes the window."

After this spec, RunCommand draws nothing. The `.h` comment must be updated to match the new behavior. Add this to the Required implementation behavior section.

---

### W-05 — Demo usage message does not mention case sensitivity

When the user runs `buddd demo TRIANGLE` (uppercase), they get `"Unknown demo: 'TRIANGLE'"`. The usage message (line 136–141) lists available demos but does not indicate that demo names are case-sensitive. Users may reasonably try `buddd demo Triangle` or `buddd demo TRIANGLE` and receive no guidance. Adding `"Demo names are case-sensitive."` to the usage text would improve discoverability.

---

### W-06 — Story 1 diagnostic message uses lowercase while title example uses mixed case

Story 1 (line 293) shows the window title as `"Buddd Engine — Demo: Triangle"` but the demo completion message (line 127–128) and the demo-start message (line 127) use lowercase:

```
Demo started: triangle (120 frames)
Demo complete: triangle (120 frames rendered)
```

This is not a contradiction per se (the title and the messages are separate strings), but the asymmetry may appear inconsistent to users. Consider whether the diagnostic messages should also capitalise (e.g., `"Demo started: Triangle (120 frames)"`) or whether the title should use lowercase for consistency. This is a minor UX polish concern.

---

## Required changes

Concrete, actionable changes requested:

- [x] **B-01**: Reconcile the window title case between the User-visible behavior rule (case-preserving, line 114) and the examples in Story 1 (line 293) and AC-007 (line 385). Choose either all-lowercase or all-capitalised and apply consistently.
- [x] **B-02**: Either (a) remove "optional" from the "New tests" table header (line 487) and mark all unconditional tests as required, or (b) document an explicit rationale for a CONST-002 exception with user/approval sign-off.
- [x] **B-03**: Resolve the per-demo function argv contract contradiction. Either update the doc comment (lines 234–236) to reflect that `argv[2]` is the demo name, or change the DemoCommand dispatch to construct a sub-argv where `argv[0]` is the demo name.
- [x] **W-01**: Clarify how the framebuffer is cleared in `RunCommand`. Either document that `begin_frame()` performs the clear as an engine contract, or update the non-goals to allow a minimal engine API change.
- [x] **W-02**: Either move the extra-arguments warning in `DemoCommand` to after successful resource creation, or document the intentional ordering.
- [x] **W-03**: Add `buddd run extra_arg` to the edge case table.
- [x] **W-04**: Add run_command.h comment update to the Required implementation behavior section.
- [x] **W-05**: Add a case-sensitivity note to the demo usage text.
- [x] **W-06**: Align the Story 1 title and diagnostic message casing for consistency — all now use lowercase `triangle`.

---

## Suggested improvements

Optional ideas (not required):

1. **Spec-001 supersession chaining**: SPEC-006's review required adding a note that SPEC-006 supersedes SPEC-001 AC-005/AC-006. SPEC-007 inherits that relationship without mentioning SPEC-001. Adding a brief note in the Supersedes section ("SPEC-007 carries forward SPEC-006's supersession of SPEC-001 CLI behavior") would close the chain cleanly.

2. **`setup_triangle()` fatal `std::exit()` behavior**: The error cases table (line 445) documents that `setup_triangle()` calls `std::exit(EXIT_FAILURE)` on failure. This is existing behavior but is a poor library-function contract — it prevents callers from handling failures gracefully. Consider noting this as a known smell for a future refactor.

3. **Demo dispatch scalability**: SC-001 says a new demo requires adding an `else if` branch in `DemoCommand::run()`. For 3–5 demos this is fine, but for 10+ it will become unwieldy. Consider adding a compile-time registration mechanism (e.g., a constexpr map) as a future enhancement.

4. **RunCommand no-args path for demos**: The spec does not define what happens if `buddd demo` is run with `BUDDD_HAS_DISPLAY=OFF`. The platform creation fails at runtime — but there is no early check for display availability before resource-heavy operations. Not a spec issue, but could be a UX improvement.

5. **Triangulate `WEXITSTATUS` portability in tests**: The existing `version_test.cpp` uses `WEXITSTATUS(ret)` which is POSIX-specific. This is existing code and not a SPEC-007 issue, but the Test implications section could note that any new `[cli]` tests inherit this assumption.

---

## Re-review summary

| Check | Outcome |
|---|---|
| Internal consistency | **3 blocking issues**: title case contradiction (B-01), tests labelled optional vs CONST-002 (B-02), argv contract contradiction (B-03) |
| Properly supersedes SPEC-006 | Yes — Supersedes section is clear, thorough, and correctly lists what is and is not superseded |
| Acceptance criteria testability | All ACs are testable in principle, but B-01 makes AC-007 ambiguous |
| Edge/error case coverage | Mostly thorough. Missing `buddd run extra_arg` (W-03). Framebuffer clear undefined (W-01). |
| Constitution rules preserved | CONST-002 violated (B-02). CONST-001 preserved (AC-018). |
| Hidden assumptions/ambiguities | Framebuffer clear mechanism (W-01), argv contract (B-03), split render-loop ownership (W-05 observation in warnings) |
| Open questions resolution | All 5 resolved with clear rationales — no issues |
| Test implications | Clear and actionable, but CONST-002 compliance requires removing "optional" label |
| Verdict | **Rejected** — three blocking issues must be resolved before acceptance |

---

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st (this) | `Rejected` | 3 blocking issues: B-01 (title case), B-02 (CONST-002 / tests optional), B-03 (argv contract). 6 warnings. |

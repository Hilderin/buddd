# Spec Review — SPEC-006: CLI Command System

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- None identified. The spec is well-structured, thorough, and addresses a clear problem with a well-scoped solution.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

1. **SPEC-001 AC-005 / AC-006 contradiction (no-args and `--version` behavior)**: SPEC-001 (Project Setup) AC-005 specifies that `buddd --version` prints `"buddd 0.1.0"`, and AC-006 specifies that running `buddd` with no arguments prints the greeting message `"Buddd Engine v0.1.0"`. SPEC-006 drops the `--version` flag entirely (replacing it with `buddd version`) and changes no-args behavior to open an interactive window (defaulting to `run`). These are breaking changes from SPEC-001's accepted CLI contract. While the existing codebase already deviates from SPEC-001 AC-006 (it currently opens an interactive window rather than printing a greeting), and the evolution is intentional, the spec does not acknowledge SPEC-001 or state that it supersedes those acceptance criteria. A supersession note should be added.

2. **CONST-002 testing policy implication**: CONST-002 mandates that "all testable code added or modified in this project must have corresponding unit tests" with no exceptions. The spec explicitly defers unit tests for individual commands to "a future concern" (see Out of scope: "Unit tests for individual commands (each command is testable via shell invocation; dedicated unit tests are a future concern)." While the ACs provide integration-level testing (shell process verification via AC-006–AC-018), the extracted command classes expose public `run(int, const char* const*) -> int` methods that are inherently unit-testable without a display. The spec should either (a) add basic unit tests for the command dispatch logic and error handling (which need no GPU), or (b) acknowledge the CONST-002 deviation with a rationale, or (c) explicitly state that the integration-level ACs satisfy the testing requirement.

3. **Subjective success criterion SC-002**: SC-002 states: "The `main.cpp` dispatch logic is immediately understandable: the command table is visible in the first 30 lines of `main()`." The phrase "immediately understandable" is subjective and the 30-line metric is arbitrary. This criterion is not precisely verifiable. Consider rephrasing as a concrete structural requirement (e.g., "the command dispatch consists of a single if/else-if chain or a fixed-size array mapping strings to function pointers, contained within the first 30 lines of `main()`").

4. **Missing `src/cmd/CMakeLists.txt` build integration details**: AC-015 specifies that `src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE CONFIGURE_DEPENDS src/cmd/*.cpp)`. However, the spec does not document how this new file integrates with the existing root `CMakeLists.txt`. Currently the source file for `buddd` is compiled via some existing mechanism (the root CMake or some other file). The spec should clarify: (a) whether `src/cmd/CMakeLists.txt` is a new file or whether an existing one is being modified, (b) how the root `CMakeLists.txt` picks it up (presumably via `add_subdirectory(src/cmd)`), and (c) what happens to any existing `add_executable(buddd ...)` rules that referenced `main.cpp` directly. This is assumed but not documented.

## Required changes

Concrete, actionable changes requested:

- [x] **W-1 (SPEC-001 supersession)**: Add a note to the spec that it supersedes SPEC-001 AC-005 and AC-006 regarding CLI behavior. Specifically, the old `--version` flag behavior is replaced by `buddd version`, and running `buddd` with no arguments now defaults to the `run` command (interactive window) rather than printing a greeting message. Update the Goals or add a new "Supersedes" section.

- [x] **W-2 (CONST-002 / unit tests)**: Either add unit-test ACs for command dispatch (e.g., verify that `version` maps to VersionCommand, unknown command returns code 1), or add a rationale explaining why the integration-level ACs satisfy CONST-002, or explicitly flag the deviation as a known exception. The dispatch logic in `main.cpp` and the unknown-command error path are testable without a display or GPU and should have at minimum a test.

- [x] **W-3 (SC-002 precision)**: Reword SC-002 to be objectively verifiable. Suggested wording: "The command dispatch logic consists of a single if/else-if chain (or an equivalent lookup structure) mapping command name strings to function calls or command objects, and is contained within the first 30 lines of `main()`."

- [x] **W-4 (CMake integration)**: Add a brief note describing how `src/cmd/CMakeLists.txt` integrates with the root `CMakeLists.txt` — specifically whether a new `add_subdirectory(src/cmd)` is added and whether existing `add_executable` rules for `buddd` (if any exist outside `src/cmd/`) are removed.

## Suggested improvements

Optional ideas (not required):

1. **Extra-argument behavior inconsistency**: The spec defines three different behaviors for extra arguments across commands: silently ignored (`version`, `help`, `run`), warned-about-but-proceeded (`test`), and error (`unknown command`). While each is reasonable in isolation, having three distinct policies may confuse users. Consider whether `test` should also silently ignore extras (for consistency) or whether `run` should warn (for discoverability). This is a design preference, not a spec defect.

2. **`--help` flag partial UX concern**: Running `buddd --help` produces `"Unknown command: '--help'"` with the usage block. This is correct per the spec but may confuse users accustomed to `--help` in every CLI tool. The error message does helpfully include the usage block, so this is a minor ergonomic concern. Consider documenting this explicitly in the edge cases table.

3. **`setup_triangle` duplication risk**: The spec mentions (A-06) that a shared utility file (e.g., `demo_helpers.h/.cpp`) is preferred over duplicating the triangle setup code across `RunCommand` and `TestCommand`. Since the spec is silent on exactly which file this should be, the implementation may end up with duplication. A concrete file name recommendation would help consistency.

4. **No `help` in `--test`/`--version` old-flag error messages**: The "Old flags rejected" Story (Story 6) uses standard unknown command error output. This is correct behavior, but the error message doesn't tell the user *what the new syntax is* (e.g., "Use `buddd version` instead"). Adding a hint to these two specific error messages would improve UX. However, this would require special-casing `--test` and `--version`, which adds complexity. Acceptable to leave as-is.

5. **Help text maintenance burden**: Assumption A-09 notes that if the set of commands changes, `HelpCommand` must be updated manually. This is a maintenance risk. Consider adding a TODO or a compile-time assertion that the help text mentions all registered commands (e.g., via a macro or constexpr string concatenation), though this is acknowledged as out of scope for now.

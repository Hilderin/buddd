# Implementation Contract Review — Scene Graph (IMPL-008) — 6th Review

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

The previous blocking issue **B-IC-03** (incorrect compiler version claim for `std::optional<T&>`) has been resolved. The contract's "Compiler support note" now correctly states **GCC 16+** and **Clang 22+**, matching cppreference's C++26 compiler support table. ADR-005 has also been corrected to reflect GCC 16+, Clang 22+. ADR-001 was partially updated (line 17 corrected) but retains an internal inconsistency (line 97 still references the old baseline).

**No blocking issues remain.** The contract is ready for implementation with 6 non-blocking warnings.

---

### B-IC-03 (compiler support note): ✅ RESOLVED

The "Compiler support note" section (lines 967–974) now reads:

> - **GCC 16+** — full support for `std::optional<T&>`
> - **Clang 22+** — full support for `std::optional<T&>`

This matches cppreference's C++26 compiler support table (P2988R12 + P3836R2):

| Feature | Paper(s) | GCC libstdc++ | Clang libc++ |
|---|---|---|---|
| `std::optional<T&>` | P2988R12 + P3836R2 | **16** | **22** |

The fallback note is also preserved: "A fallback mechanism (e.g., `T*` with `nullptr` for absent) should be added if targeting compilers older than these versions in the future."

**Previosuly blocked: implementable now.**

### B-IC-04 (world_matrix algorithm): ✅ STILL CORRECT (unchanged from 4th/5th review)

The `Transform::world_matrix()` algorithm at lines 405–448 remains correct for both paths (verified by manual trace in 4th and 5th reviews):

- **Normal case (depth < MAX_DEPTH)**: Accumulates root-to-leaf via the stack array, second while loop skipped. Result: `T_root * ... * T_entity` — correct.
- **Overflow case (depth >= MAX_DEPTH)**: Break stores break-entity's local matrix in `result`, for-loop prepends MAX_DEPTH ancestors, second while-loop prepends remaining ancestors. Final result: `T_root * ... * T_entity` — correct.

### W-06 (ADR reference mismatch): ✅ RESOLVED

Line 1006 now reads "A new ADR (ADR-005) documents...", correctly referencing `docs/adr/005-optional-ref-component-api.md`. No ADR-003 references remain.

### W-07 (ADR-005 incorrect GCC claim): ✅ RESOLVED

ADR-005 line 29 now correctly states "GCC 16+ and Clang 22+ have full support for `std::optional<T&>`", matching the contract and cppreference.

---

## Blocking issues

No blocking issues. All items from previous reviews are resolved.

---

## Warnings

Non-blocking concerns for awareness:

- **W-01** (carried): **Unused `<span>` include** in `entity.h` (line 312) and `world.h` (line 547). Leftover from the old `children()`-returns-span API design. No `std::span` is used in the final API. Harmless but untidy.

- **W-02** (carried): **Redundant `EntityId::operator!=`** (line 204). C++20 synthesizes `operator!=` from `operator==`. Inconsistent with `Entity` which correctly omits explicit `operator!=`.

- **W-03** (carried): **`reparent()` move-then-erase pattern** (lines 776–777) is correct but subtle. A clarifying comment explaining the ownership transfer would aid readability.

- **W-04** (carried): **Missing `#include <utility>`** for `std::forward`. Used in `entity.h` (line 385) and `world.h` (line 628) template methods. While `<utility>` is usually dragged in transitively by `<memory>`, this is not guaranteed by the standard.

- **W-05** (carried): **`flush_destroyed()` declared `noexcept` but calls `free_slots_.push_back()`** (line 735) — a vector `push_back` may allocate (reallocation). The contract states "noexcept (does not allocate; only reclaims existing resources)" which is technically inaccurate. While the project treats OOM as unrecoverable, the noexcept declaration contradicts the stated rationale.

- **W-08 (new)**: **ADR-001 has internal inconsistency after incomplete baseline update** — ADR-001 line 17 correctly states "GCC 16+, Clang 22+" but line 97 still says "GCC 14+, Clang 19+" (`"All compilers in the support matrix must provide std::expected — currently GCC 14+, Clang 19+, MSVC 2025+"`). The contract's ADR impact section (line 1006) claims "ADR-001 has been updated to reflect the new baseline" which is only partially true. ADR-001 line 97 should be updated to "GCC 16+, Clang 22+" to eliminate the contradiction.
  - **Impact on Code Agent**: None directly — the contract's compiler note is correct (GCC 16+, Clang 22+), and the Code Agent follows the contract. However, if the Code Agent cross-references ADR-001, it will find conflicting version numbers. Recommend updating ADR-001 line 97 for consistency.

---

## Required changes

None. All required changes from previous reviews have been addressed. The contract is ready for implementation.

---

## Suggested improvements

Optional ideas (not required):

1. **Remove unused `<span>` include** from `entity.h` and `world.h` (addresses W-01).
2. **Remove redundant `operator!=`** from `EntityId` (addresses W-02).
3. **Add clarifying comment** to the `reparent()` move-then-erase ownership transfer (addresses W-03).
4. **Add `#include <utility>`** to `entity.h` and `world.h` for explicit `std::forward` inclusion (addresses W-04).
5. **Pre-reserve `free_slots_` capacity** or update the noexcept documentation for `flush_destroyed()` (addresses W-05).
6. **Fix ADR-001 line 97** to say "GCC 16+, Clang 22+" instead of "GCC 14+, Clang 19+" (addresses W-08).
7. **Add ADR-005 to the "Relevant ADRs" section** (lines 48–52) for completeness, since ADR-005 governs the `std::optional<T&>` design decision used throughout the contract.
8. **Markdown formatting fix**: Line 849 has a double backtick (`;`` `) that should be `;`. `.

---

## Resolved issues (from previous reviews)

| ID | Summary | Resolution |
|---|---|---|
| B-IC-01 | Shared `children_buffer_` violates AC-023 span validity guarantee | Removed shared buffer; replaced with per-node children + `child_count()`/`get_child()` methods |
| B-IC-02 | `get_children()` declared `noexcept` but allocates | Removed `get_children()`; new `get_child_count()`/`get_child()` are pure getters, trivially noexcept |
| B-IC-03 | Compiler support note claims GCC 14+ for `std::optional<T&>` (needs GCC 16+) | ✅ **FIXED.** Contract now states "GCC 16+" matching cppreference. ADR-005 also corrected. ADR-001 partially updated (line 17, though line 97 remains inconsistent — see W-08). |
| B-IC-04 | `Transform::world_matrix()` drops one transform at MAX_DEPTH break point | ✅ **Fixed in 4th review.** Algorithm now correctly accumulates `current.transform().local_matrix()` into `result` before break. |
| W-01 (1st rev) | Include order violation: local before standard headers | Fixed: standard headers now precede local headers in both `entity.h` and `world.h` |
| W-03 (1st rev) | RTTI requirement for `dynamic_cast` not documented | New "RTTI requirement" section added |
| W-04 (1st rev) | `children_buffer_` pre-reservation mechanism unspecified | Moot: shared buffer removed |
| W-06 (4th rev) | ADR reference mismatch: line 1006 said "ADR-003" | ✅ **Fixed.** Now correctly references ADR-005. |
| W-07 (5th rev) | ADR-005 itself contains incorrect GCC version claim (GCC 14+) | ✅ **Fixed.** ADR-005 now says "GCC 16+ and Clang 22+". |

---

## Verdict

**Accepted with warnings.**

The sole blocking issue from the previous review — **B-IC-03** (incorrect compiler version claim for `std::optional<T&>`) — has been resolved. The contract now correctly states **GCC 16+, Clang 22+** for `std::optional<T&>` support, matching cppreference's C++26 compiler support table and ADR-005.

All other previously resolved items (B-IC-01, B-IC-02, B-IC-04, W-01, W-03, W-04, W-06, W-07) remain verified correct.

The 6 remaining warnings (W-01 through W-05 carried, W-08 new) are non-blocking. None prevent implementation. The ADR-001 internal inconsistency (W-08) is noted for separate cleanup but does not affect the Code Agent — the contract's compiler baseline is self-consistent and correct.

**The Code Agent may proceed with implementation under this contract.**

---

## Re-review summary

| Check | Outcome |
|---|---|
| B-IC-03: Compiler support note GCC claim | ✅ **RESOLVED**: Now says GCC 16+ (correct) |
| B-IC-03: Compiler support note Clang claim | ✅ Clang 22+ (correct, unchanged from 5th review) |
| B-IC-03: ADR-005 alignment | ✅ ADR-005 now says GCC 16+, Clang 22+ (consistent) |
| B-IC-03: ADR-001 alignment | ⚠️ **Partial**: line 17 corrected, line 97 still has old values (W-08) |
| B-IC-04: world_matrix algorithm | ✅ **Correct**. Both normal and overflow paths verified |
| ADR reference (was W-06) | ✅ Correctly references ADR-005 |
| Consistency with accepted spec | ✅ All ACs satisfied |
| Include order | ✅ Correct |
| RTTI documentation | ✅ Documented |
| noexcept correctness | ⚠️ W-05: `flush_destroyed()` noexcept but vector push_back may allocate |
| Feasibility / technical correctness | ✅ Correct compiler versions now stated |
| Completeness (AC coverage) | Excellent — all 49 tests cover 32 ACs |
| Clarity of pseudo-code | Good |
| Project conventions | Respected |
| Edge cases | Thoroughly documented |
| ADR impact section accuracy | ✅ References ADR-005 correctly; states "ADR-001 has been updated" (partially true — line 17 done, line 97 not) |
| **Verdict** | **Accepted with warnings** — 0 blocking, 6 warnings |

---

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st | `Rejected` | 2 blocking issues: B-IC-01 (shared children_buffer_), B-IC-02 (noexcept + allocation contradiction). 6 warnings. |
| 2nd | `Accepted` | Both blocking issues resolved. 3 new minor warnings. |
| 3rd | `Rejected` | 2 new blocking issues: B-IC-03 (std::optional<T&> compiler support risk), B-IC-04 (world_matrix algorithm bug). 5 warnings. |
| 4th | `Rejected` | B-IC-04 resolved ✅, B-IC-03 reopened — compiler support note makes factually incorrect GCC/Clang version claims ❌. New warning W-06 (ADR reference mismatch). 6 warnings. |
| 5th | `Rejected` | W-06 (ADR reference) resolved ✅. B-IC-04 confirmed correct ✅. **B-IC-03 still unresolved** — compiler support note still claims "GCC 14+" when cppreference shows GCC 16+ required ❌. New warning W-07: ADR-005 itself contains the same incorrect GCC version claim. |
| **6th (this review)** | **`Accepted with warnings`** | **B-IC-03 resolved ✅** — contract now says GCC 16+, Clang 22+. ADR-005 also corrected. ADR-001 partially updated (line 17 done, line 97 still has old values → W-08). W-07 resolved. 6 warnings total (5 carried + 1 new: W-08). No blocking issues remain. |

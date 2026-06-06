# ADR-001: Project-wide `Result<T>` / `Error` Pattern

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The Buddd Engine needs a consistent, project-wide mechanism for error propagation. Before this ADR, there was no established convention — each subsystem could have used a different approach, leading to inconsistency and cognitive overhead for developers.

The decision was made concretely in SPEC-002 (Platform Abstraction Layer) which introduced `Result<T>` and `Error` as the error-return mechanism for the platform, window, and render device APIs. However, the impact extends far beyond SPEC-002: the spec explicitly states this is "the standard error-return pattern for all engine APIs going forward."

The following factors influenced the decision:

- **C++26 availability**: The project targets C++26 compilers (GCC 16+, Clang 22+) which provide `std::expected` natively. No external library is needed.
- **No-exceptions environments**: The project may target platforms or build configurations where C++ exceptions are disabled (`-fno-exceptions`). `std::expected` works in such environments; exceptions do not.
- **Explicit error paths**: The project constitution principles prefer "explicit contracts over implicit assumptions." `std::expected` makes error paths visible in function signatures rather than hiding them in implicit exception propagation.
- **Composability**: `std::expected` supports monadic operations (`and_then`, `or_else`, `transform`, `transform_error`) which enable concise error chaining as the codebase grows.

## Decision

We adopt the following as the project-wide error handling pattern:

1. **`Result<T>`** is defined as `template<typename T> using Result = std::expected<T, Error>`. All engine APIs that can fail return `Result<T>` rather than throwing exceptions or returning error codes.

2. **`Error`** is a struct with three fields:
   - `Category category` — an `enum class` with values scoped to the error domain (e.g., `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `Unsupported`, `Unknown`).
   - `int code` — a backend-specific numeric error code (defaults to `0` when not relevant).
   - `std::string message` — a human-readable description.

3. **`make_error()`** is a free helper function returning `std::unexpected<Error>` for concise error construction: `return make_error(Error::Category::InitFailed, "Something went wrong");`

4. **`to_string(const Error&)`** provides human-readable formatting: `"Category: message (code N)"`.

5. All public API functions that can fail return `Result<T>`. Exceptions are not used for control flow in engine code.

### Where this applies

- All factory functions (`Platform::create()`, `RenderDevice::create()`)
- All operations that can meaningfully fail (resource creation, initialization, I/O)
- All future engine subsystems (audio, input, asset loading, ECS, etc.)

### Where this does NOT apply

- Functions that cannot logically fail (pure getters, predicates, trivial computations) — these return plain values.
- Internal implementation details where an alternative is justified (e.g., a deeply recursive function that would be painful to propagate `Result` through every call).

## Alternatives considered

### C++ exceptions

- **Pros**: Automatic stack unwinding, no boilerplate in intermediate functions, familiar to most C++ developers.
- **Cons**: Not available with `-fno-exceptions`; invisible error paths; runtime overhead for unwind tables; inconsistent with the project's preference for explicit contracts.
- **Verdict**: Rejected. The project may target environments without exception support, and the constitution prefers explicit contracts.

### Traditional error codes (e.g., `int` return + out parameters)

- **Pros**: No language feature requirements, works in C and C++, minimal overhead.
- **Cons**: Error codes are easy to ignore; out parameters clutter signatures; no type safety; no automatic error propagation.
- **Verdict**: Rejected. Too error-prone and verbose.

### `std::optional<T>` + error sink

- **Pros**: Simple API; `std::nullopt` on failure.
- **Cons**: No error information propagated (caller knows *that* something failed but not *what*); requires a separate error-logging mechanism; loses error context.
- **Verdict**: Rejected. Error context (category, message, code) is critical for debugging and diagnostics.

### `std::variant<T, Error>` (custom expected-like type)

- **Pros**: No dependency on `std::expected`; full control over API.
- **Cons**: Reinventing the wheel; no monadic interface; non-standard; increased maintenance burden.
- **Verdict**: Rejected. `std::expected` is standard and provides the necessary API.

### `boost::outcome`

- **Pros**: Richer error model (outcomes, results, error codes); production-tested in large codebases.
- **Cons**: External dependency; not standard; adds build complexity; most features not needed for this project's scale.
- **Verdict**: Rejected. `std::expected` suffices and avoids a Boost dependency.

## Consequences

### Positive

- **Explicit contract**: Every fallible function advertises this in its return type.
- **Exception-free**: Works with `-fno-exceptions` and in constrained environments.
- **Composable**: `and_then`, `transform`, `or_else` enable ergonomic chaining as the codebase grows.
- **Standard**: Uses C++26 `std::expected` — no third-party dependency.
- **Uniform**: A single error type (`Error`) and pattern (`Result<T>`) across the entire engine.
- **Backend-specific errors**: The `int code` field captures platform-specific error codes (e.g., SDL error codes, OpenGL error codes) without leaking backend types.

### Negative

- **Verbose**: Every error-returning function requires `-> Result<T>` and every call site must handle or propagate the error.
- **Learning curve**: Developers new to `std::expected` must learn monadic patterns or write explicit `.has_value()` / `.error()` branches.
- **C++26 requirement**: All compilers in the support matrix must provide `std::expected` — currently GCC 16+, Clang 22+, MSVC 2025+.
- **Not zero-cost**: `std::expected` has a small size and copy overhead compared to raw error codes, though it is typically optimized well.

### Migration

- All new code MUST use `Result<T>` for fallible APIs.
- Existing APIs (if any) SHOULD be migrated to `Result<T>` when they are next modified.
- The `Error::Category` enum is extensible — new categories are added as new subsystems are introduced.

## References

- SPEC-002, lines 34–35, 90, 260: Definition and scope of `Result<T>` pattern.
- `src/engine/error.h`: Canonical implementation of `Error`, `Result<T>`, `make_error()`, `to_string()`.
- `src/engine/platform/platform.h`: Usage of `Result<std::unique_ptr<Platform>>` and `Result<std::unique_ptr<Window>>`.
- `src/engine/render/render_device.h`: Usage of `Result<std::unique_ptr<RenderDevice>>`.
- [C++26 `std::expected`](https://en.cppreference.com/w/cpp/utility/expected)

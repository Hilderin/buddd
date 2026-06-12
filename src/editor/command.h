#pragma once

#include <string_view>

namespace buddd::editor {

struct EditorContext;

/// Abstract base for all editor commands.
/// Lifecycle: construct -> execute() -> (undo() | execute())...
class Command {
public:
    virtual ~Command() = default;

    /// Execute (or re-execute) the command.
    virtual auto execute(EditorContext const& ctx) -> void = 0;

    /// Undo the command. Only called after execute().
    virtual auto undo(EditorContext const& ctx) -> void = 0;

    /// Human-readable name for menu display (e.g., "Quit").
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;
};

} // namespace buddd::editor

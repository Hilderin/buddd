#pragma once

#include "command.h"

#include <memory>
#include <string_view>
#include <vector>

namespace buddd::editor {

struct EditorContext;

/// Bounded undo/redo stack for editor commands.
/// Thread-compatible: not thread-safe — used from the main thread only.
class CommandStack {
public:
    /// @param max_history Maximum number of undoable commands to retain.
    ///                    Default 128. Must be clamped to minimum 1.
    explicit CommandStack(size_t max_history = 128);

    /// Execute a command and push it onto the undo stack.
    /// Clears the redo stack (any previously undone commands are discarded).
    auto execute(std::unique_ptr<Command> command, EditorContext const& ctx) -> void;

    /// Undo the most recent command. Returns false if undo stack is empty.
    [[nodiscard]] auto undo(EditorContext const& ctx) -> bool;

    /// Redo the most recently undone command. Returns false if redo stack is empty.
    [[nodiscard]] auto redo(EditorContext const& ctx) -> bool;

    /// Returns true if there is at least one command to undo.
    [[nodiscard]] auto can_undo() const -> bool;

    /// Returns true if there is at least one command to redo.
    [[nodiscard]] auto can_redo() const -> bool;

    /// Name of the command at the top of the undo stack (empty view if empty).
    [[nodiscard]] auto undo_name() const -> std::string_view;

    /// Name of the command at the top of the redo stack (empty view if empty).
    [[nodiscard]] auto redo_name() const -> std::string_view;

    /// Clear all stacks.
    auto clear() -> void;

private:
    std::vector<std::unique_ptr<Command>> undo_stack_;
    std::vector<std::unique_ptr<Command>> redo_stack_;
    size_t max_history_;
};

} // namespace buddd::editor

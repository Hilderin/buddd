#include "command_stack.h"

#include <algorithm>  // for std::min, std::max

namespace buddd::editor {

CommandStack::CommandStack(size_t max_history)
    : max_history_(std::max<size_t>(max_history, 1))  // clamp to minimum 1
{
}

auto CommandStack::execute(std::unique_ptr<Command> command) -> void {
    command->execute();
    undo_stack_.push_back(std::move(command));
    redo_stack_.clear();

    // Enforce max_history bound: if undo stack exceeds limit, drop oldest command
    if (undo_stack_.size() > max_history_) {
        undo_stack_.erase(undo_stack_.begin());
    }
}

auto CommandStack::undo() -> bool {
    if (undo_stack_.empty()) {
        return false;
    }
    auto command = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    command->undo();
    redo_stack_.push_back(std::move(command));
    return true;
}

auto CommandStack::redo() -> bool {
    if (redo_stack_.empty()) {
        return false;
    }
    auto command = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    command->execute();  // re-execute, not undo
    undo_stack_.push_back(std::move(command));
    return true;
}

auto CommandStack::can_undo() const -> bool {
    return !undo_stack_.empty();
}

auto CommandStack::can_redo() const -> bool {
    return !redo_stack_.empty();
}

auto CommandStack::undo_name() const -> std::string_view {
    if (undo_stack_.empty()) {
        return {};
    }
    return undo_stack_.back()->name();
}

auto CommandStack::redo_name() const -> std::string_view {
    if (redo_stack_.empty()) {
        return {};
    }
    return redo_stack_.back()->name();
}

auto CommandStack::clear() -> void {
    undo_stack_.clear();
    redo_stack_.clear();
}

} // namespace buddd::editor

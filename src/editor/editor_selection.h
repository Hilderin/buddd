#pragma once

#include "scene/entity_id.h"

#include <algorithm>       // std::find_if for callback removal
#include <cstddef>         // size_t
#include <cstdint>         // uint64_t
#include <functional>      // std::function, std::hash
#include <optional>
#include <span>
#include <unordered_set>
#include <utility>         // std::pair
#include <vector>

// ── std::hash specialization for EntityId ────────────────────────────
// EntityId has no hash in its own header; we provide one here so
// std::unordered_set<EntityId> compiles. Placed in editor_selection.h
// rather than modifying the engine file.
template<>
struct std::hash<buddd::engine::EntityId> {
    auto operator()(buddd::engine::EntityId const& id) const noexcept -> size_t {
        // Simple hash: XOR index and generation (shifted to avoid collisions)
        return static_cast<size_t>(id.index) ^ (static_cast<size_t>(id.generation) << 16);
    }
};

// ── SelectionModifier enum ───────────────────────────────────────────

namespace buddd::editor {

// EntityId is in buddd::engine; we bring it into scope for convenience.
using EntityId = buddd::engine::EntityId;

enum class SelectionModifier {
    Replace,  // Clear + select this entity (plain click). Sets anchor.
    Toggle,   // Add or remove this entity (Ctrl+click). Anchor unchanged.
};

// ── Selection value class ────────────────────────────────────────────

class Selection {
public:
    // -- Query --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // -- Iteration (expose the underlying set for range-for) --
    using const_iterator = std::unordered_set<EntityId>::const_iterator;
    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    // -- Local mutation (builds a copy — does NOT affect EditorSelection) --
    auto add(EntityId id) -> void;
    auto remove(EntityId id) -> void;
    auto clear() -> void;

    // -- Primary (last-selected) --
    [[nodiscard]] auto primary() const noexcept -> std::optional<EntityId> { return primary_; }
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId> { return anchor_; }
    void set_primary(EntityId id) { primary_ = id; }
    void set_anchor(EntityId id) { anchor_ = id; }
    void reset_primary() { primary_ = std::nullopt; }
    void reset_anchor() { anchor_ = std::nullopt; }

    // -- Comparison --
    auto operator==(const Selection&) const noexcept -> bool = default;

private:
    friend class EditorSelection;
    std::unordered_set<EntityId> selected_;
    std::optional<EntityId> primary_;
    std::optional<EntityId> anchor_;
};

// ── EditorSelection manager ──────────────────────────────────────────

class EditorSelection {
public:
    // -- Query (delegates to current_) --
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto first() const noexcept -> std::optional<EntityId>;

    // -- Mutation (all fire callbacks after modification) --
    void select(EntityId id, SelectionModifier modifier = SelectionModifier::Replace);
    void clear();
    void set_selection(std::span<const EntityId> ids);

    // -- Snapshot for Commands (F-04+) --
    [[nodiscard]] auto snapshot() const noexcept -> Selection;
    void restore(const Selection& saved);  // fires callbacks

    // -- Generation counter (avoids unnecessary snapshots) --
    [[nodiscard]] auto generation() const noexcept -> uint64_t { return generation_; }

    // -- Primary (last-selected) --
    [[nodiscard]] auto primary() const noexcept -> std::optional<EntityId>;

    // -- Shift+click anchor --
    [[nodiscard]] auto anchor() const noexcept -> std::optional<EntityId>;
    void set_anchor(EntityId id);

    // -- Callbacks --
    using ChangeCallback = std::function<void()>;
    auto on_change(ChangeCallback cb) -> size_t;   // returns token
    void remove_on_change(size_t token);

    // -- Selection object access (for iteration) --
    [[nodiscard]] auto current() const noexcept -> const Selection&;

private:
    void fire_callbacks();

    Selection current_;
    std::vector<std::pair<size_t, ChangeCallback>> callbacks_;
    size_t next_token_ = 0;
    uint64_t generation_ = 0;
};

// ── Selection inline implementations ─────────────────────────────────

inline auto Selection::contains(EntityId id) const noexcept -> bool {
    return selected_.contains(id);
}

inline auto Selection::size() const noexcept -> size_t {
    return selected_.size();
}

inline auto Selection::empty() const noexcept -> bool {
    return selected_.empty();
}

inline auto Selection::first() const noexcept -> std::optional<EntityId> {
    if (selected_.empty()) return std::nullopt;
    return *selected_.begin();
}

inline auto Selection::begin() const noexcept -> const_iterator {
    return selected_.begin();
}

inline auto Selection::end() const noexcept -> const_iterator {
    return selected_.end();
}

inline auto Selection::add(EntityId id) -> void {
    selected_.insert(id);
}

inline auto Selection::remove(EntityId id) -> void {
    selected_.erase(id);
}

inline auto Selection::clear() -> void {
    selected_.clear();
}

// ── EditorSelection inline implementations ───────────────────────────

inline auto EditorSelection::contains(EntityId id) const noexcept -> bool {
    return current_.contains(id);
}

inline auto EditorSelection::size() const noexcept -> size_t {
    return current_.size();
}

inline auto EditorSelection::empty() const noexcept -> bool {
    return current_.empty();
}

inline auto EditorSelection::first() const noexcept -> std::optional<EntityId> {
    return current_.first();
}

inline auto EditorSelection::snapshot() const noexcept -> Selection {
    return current_;  // copy — independent clone
}

inline auto EditorSelection::primary() const noexcept -> std::optional<EntityId> {
    return current_.primary();
}

inline auto EditorSelection::anchor() const noexcept -> std::optional<EntityId> {
    return current_.anchor();
}

inline auto EditorSelection::set_anchor(EntityId id) -> void {
    current_.set_anchor(id);
}

inline auto EditorSelection::current() const noexcept -> const Selection& {
    return current_;
}

inline void EditorSelection::select(EntityId id, SelectionModifier modifier) {
    // Defensive guard: silently ignore EntityId::none()
    if (id == EntityId::none()) return;

    ++generation_;
    if (modifier == SelectionModifier::Replace) {
        current_.selected_.clear();
        current_.selected_.insert(id);
        current_.set_primary(id);
        current_.set_anchor(id);
    } else {  // Toggle
        if (current_.selected_.contains(id)) {
            current_.selected_.erase(id);
        } else {
            current_.selected_.insert(id);
        }
        current_.set_primary(id);
        // anchor_ unchanged
    }
    fire_callbacks();
}

inline void EditorSelection::clear() {
    ++generation_;
    current_.selected_.clear();
    current_.reset_primary();
    current_.reset_anchor();
    fire_callbacks();
}

inline void EditorSelection::set_selection(std::span<const EntityId> ids) {
    ++generation_;
    current_.selected_.clear();
    for (auto id : ids) {
        if (id != EntityId::none()) {
            current_.selected_.insert(id);
        }
    }
    if (!ids.empty()) {
        current_.set_primary(ids[0]);
    } else {
        current_.reset_primary();
    }
    // anchor_ unchanged
    fire_callbacks();
}

inline void EditorSelection::restore(const Selection& saved) {
    ++generation_;
    current_ = saved;  // copy
    fire_callbacks();
}

inline auto EditorSelection::on_change(ChangeCallback cb) -> size_t {
    auto token = next_token_++;
    callbacks_.emplace_back(token, std::move(cb));
    return token;
}

inline void EditorSelection::remove_on_change(size_t token) {
    auto it = std::find_if(callbacks_.begin(), callbacks_.end(),
        [token](auto const& pair) { return pair.first == token; });
    if (it != callbacks_.end()) {
        callbacks_.erase(it);
    }
    // If token not found, no-op
}

inline void EditorSelection::fire_callbacks() {
    for (auto const& [token, cb] : callbacks_) {
        static_cast<void>(token);
        if (cb) cb();
    }
}

} // namespace buddd::editor

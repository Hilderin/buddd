#pragma once

#include "editor_context.h"

#include <algorithm>
#include <any>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace buddd::engine {
struct SerializationContext;
} // namespace buddd::engine

namespace buddd::editor {

// ── Forward declarations ──

/// Internal: read-only fallback display when no editor is registered (defined in .cpp).
/// Shows "(no editor for type <name>)" in disabled text.
auto draw_fallback_readonly(const std::string& label, std::type_index type,
                            const EditorContext& ctx) -> void;

/// Flags for editor behaviour (mirrors engine PropertyFlags conventions).
struct EditorFlags {
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step_value = 0.0f;
    std::vector<std::string> tags_;

    auto tag(std::string t) noexcept -> EditorFlags& {
        tags_.push_back(std::move(t));
        return *this;
    }
    auto has_tag(const std::string& t) const noexcept -> bool {
        return std::find(tags_.begin(), tags_.end(), t) != tags_.end();
    }
};

/// Abstract base for a single type editor widget.
class InspectorTypeEditor {
public:
    virtual ~InspectorTypeEditor() = default;
    [[nodiscard]] virtual auto draw(const std::string& label, void* value,
                                    const EditorFlags& flags,
                                    const EditorContext& ctx) -> bool = 0;

    /// Draw the editor for a type-erased value. The `type_index` identifies
    /// the C++ type of the value stored in `value` (a std::any).
    /// Default implementation returns false (untyped editors must override).
    /// @return true if the value was modified.
    [[nodiscard]] virtual auto draw_any(const std::string& label,
                                        std::any& value,
                                        std::type_index type_index,
                                        const EditorFlags& flags,
                                        const EditorContext& ctx) -> bool { return false; }
};

/// Typed convenience subclass. Implementors provide a DrawFn that renders the ImGui widget.
template<typename T>
class TypedInspectorEditor : public InspectorTypeEditor {
public:
    using DrawFn = std::function<bool(const std::string&, T&,
                                      const EditorFlags&,
                                      const EditorContext&)>;

    explicit TypedInspectorEditor(DrawFn fn) : draw_fn_(std::move(fn)) {}

    [[nodiscard]] auto draw(const std::string& label, void* value,
                            const EditorFlags& flags,
                            const EditorContext& ctx) -> bool override {
        return draw_fn_(label, *static_cast<T*>(value), flags, ctx);
    }

    [[nodiscard]] auto draw_any(const std::string& label,
                                std::any& value,
                                std::type_index type_index,
                                const EditorFlags& flags,
                                const EditorContext& ctx) -> bool override {
        (void)type_index;
        auto* typed = std::any_cast<T>(&value);
        if (!typed) return false;  // type mismatch
        return draw_fn_(label, *typed, flags, ctx);
    }

private:
    DrawFn draw_fn_;
};

/// Static registry mapping C++ types to InspectorTypeEditor instances.
class InspectorTypeEditorRegistry {
public:
    InspectorTypeEditorRegistry() = delete;

    template<typename T>
    static auto register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void;

    template<typename T>
    static auto register_editor(typename TypedInspectorEditor<T>::DrawFn draw_fn) -> void;

    template<typename T>
    [[nodiscard]] static auto draw(const std::string& label, T& value,
                                   const EditorFlags& flags,
                                   const EditorContext& ctx) -> bool;

    template<typename T>
    [[nodiscard]] static auto has_editor() -> bool;

    static auto get(std::type_index type) -> InspectorTypeEditor*;

    /// Draw the editor for a type-erased value, dispatching by type_index.
    /// If no editor is registered for the type, renders read-only fallback.
    /// @return true if the value was modified.
    [[nodiscard]] static auto draw_any(const std::string& label,
                                       std::any& value,
                                       std::type_index type_index,
                                       const EditorFlags& flags,
                                       const EditorContext& ctx) -> bool;

private:
    static auto map() -> std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>&;
};

// ── Template implementations ──

template<typename T>
inline auto InspectorTypeEditorRegistry::register_editor(std::unique_ptr<InspectorTypeEditor> editor) -> void {
    auto key = std::type_index(typeid(T));
    map()[key] = std::move(editor);
}

template<typename T>
inline auto InspectorTypeEditorRegistry::register_editor(typename TypedInspectorEditor<T>::DrawFn draw_fn) -> void {
    register_editor<T>(std::make_unique<TypedInspectorEditor<T>>(std::move(draw_fn)));
}

template<typename T>
inline auto InspectorTypeEditorRegistry::has_editor() -> bool {
    auto& m = map();
    return m.find(std::type_index(typeid(T))) != m.end();
}

template<typename T>
inline auto InspectorTypeEditorRegistry::draw(const std::string& label, T& value,
                                              const EditorFlags& flags,
                                              const EditorContext& ctx) -> bool {
    auto* editor = get(std::type_index(typeid(T)));
    if (editor) {
        return editor->draw(label, &value, flags, ctx);
    }
    // No editor registered for type T — fallback shows read-only text.
    draw_fallback_readonly(label, std::type_index(typeid(T)), ctx);
    return false;
}

inline auto InspectorTypeEditorRegistry::draw_any(
    const std::string& label,
    std::any& value,
    std::type_index type_index,
    const EditorFlags& flags,
    const EditorContext& ctx) -> bool
{
    auto* editor = get(type_index);
    if (editor) {
        return editor->draw_any(label, value, type_index, flags, ctx);
    }
    // No editor registered — fallback to read-only display
    draw_fallback_readonly(label, type_index, ctx);
    return false;
}

/// Called from Editor::setup() at startup.
auto register_builtin_inspector_editors() -> void;

} // namespace buddd::editor

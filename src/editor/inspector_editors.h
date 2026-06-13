#pragma once

#include "editor_context.h"

#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

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
};

/// Abstract base for a single type editor widget.
class InspectorTypeEditor {
public:
    virtual ~InspectorTypeEditor() = default;
    [[nodiscard]] virtual auto draw(const std::string& label, void* value,
                                    const EditorFlags& flags,
                                    const EditorContext& ctx) -> bool = 0;
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

/// Called from Editor::setup() at startup.
auto register_builtin_inspector_editors() -> void;

} // namespace buddd::editor

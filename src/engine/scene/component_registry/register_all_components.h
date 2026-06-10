#pragma once

namespace buddd::engine {

class ComponentRegistry;

/// Pre-register the eight built-in types in TypeRegistry.
/// Must be called once during engine startup, before register_all_components().
void register_builtin_types();

/// Register all engine component types. Called once during engine startup,
/// after register_builtin_types().
/// Calling twice is safe — duplicates produce a logged WARNING.
void register_all_components(ComponentRegistry& registry);

} // namespace buddd::engine

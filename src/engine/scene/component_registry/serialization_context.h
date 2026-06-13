#pragma once

#include <cstddef>

namespace buddd::engine {

class AssetManager;

struct SerializationContext {
    AssetManager& assets;
};

} // namespace buddd::engine

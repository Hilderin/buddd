#pragma once

#include <cstdint>
#include <string>

namespace buddd::engine {

enum class EntitySourceType : uint8_t {
    None,    // Created directly (no prefab/model) — default
    Prefab,  // Created from a prefab file via `prefab:`
    Model,   // Created from a model directive via `model:`
};

struct EntitySource {
    EntitySourceType type{EntitySourceType::None};
    std::string path;  // Path to the prefab or model file (empty for None type)
};

} // namespace buddd::engine

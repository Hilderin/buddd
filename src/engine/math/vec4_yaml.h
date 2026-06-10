#pragma once

#include "math/vec4.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Vec4> {
    static auto encode(const buddd::engine::math::Vec4& v) -> Node {
        Node node;
        node["x"] = v.x;
        node["y"] = v.y;
        node["z"] = v.z;
        node["w"] = v.w;
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Vec4& v) -> bool {
        if (!node.IsMap() || !node["x"] || !node["y"] || !node["z"] || !node["w"]) {
            return false;
        }
        try {
            v.x = node["x"].as<float>();
            v.y = node["y"].as<float>();
            v.z = node["z"].as<float>();
            v.w = node["w"].as<float>();
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML

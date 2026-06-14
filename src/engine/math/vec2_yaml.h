#pragma once

#include "math/vec2.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Vec2> {
    static auto encode(const buddd::engine::math::Vec2& v) -> Node {
        Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        node.push_back(v.x);
        node.push_back(v.y);
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Vec2& v) -> bool {
        try {
            // Sequence format: [x, y]
            if (node.IsSequence() && node.size() == 2) {
                v.x = node[0].as<float>();
                v.y = node[1].as<float>();
                return true;
            }
            // Legacy mapping format: {x: , y: }
            if (node.IsMap() && node["x"] && node["y"]) {
                v.x = node["x"].as<float>();
                v.y = node["y"].as<float>();
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML

#pragma once

#include "math/vec4.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Vec4> {
    static auto encode(const buddd::engine::math::Vec4& v) -> Node {
        Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Vec4& v) -> bool {
        try {
            // Sequence format: [x, y, z, w]
            if (node.IsSequence() && node.size() == 4) {
                v.x = node[0].as<float>();
                v.y = node[1].as<float>();
                v.z = node[2].as<float>();
                v.w = node[3].as<float>();
                return true;
            }
            // Legacy mapping format: {x: , y: , z: , w: }
            if (node.IsMap() && node["x"] && node["y"] && node["z"] && node["w"]) {
                v.x = node["x"].as<float>();
                v.y = node["y"].as<float>();
                v.z = node["z"].as<float>();
                v.w = node["w"].as<float>();
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML

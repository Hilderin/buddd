#pragma once

#include "math/quat.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Quat> {
    static auto encode(const buddd::engine::math::Quat& v) -> Node {
        Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        node.push_back(v.w);
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Quat& v) -> bool {
        try {
            // Sequence format: [w, x, y, z]
            if (node.IsSequence() && node.size() == 4) {
                v = buddd::engine::math::Quat{
                    node[0].as<float>(),  // w
                    node[1].as<float>(),  // x
                    node[2].as<float>(),  // y
                    node[3].as<float>()   // z
                };
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

#pragma once

#include "math/color.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Color> {
    static auto encode(const buddd::engine::math::Color& c) -> Node {
        Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        node.push_back(c.r);
        node.push_back(c.g);
        node.push_back(c.b);
        node.push_back(c.a);
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Color& c) -> bool {
        try {
            // Only sequence format is supported.
            if (!node.IsSequence()) return false;
            if (node.size() < 3 || node.size() > 4) return false;

            c.r = node[0].as<float>();
            c.g = node[1].as<float>();
            c.b = node[2].as<float>();
            c.a = (node.size() == 4) ? node[3].as<float>() : 1.0f;
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML

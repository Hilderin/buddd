#pragma once

namespace buddd::engine {

class Component {
public:
    virtual ~Component() = default;

    Component(const Component&) = delete;
    auto operator=(const Component&) -> Component& = delete;
    Component(Component&&) = delete;
    auto operator=(Component&&) -> Component& = delete;

protected:
    Component() = default;
};

} // namespace buddd::engine

#pragma once

namespace buddd::engine {

class Asset {
public:
    virtual ~Asset() = default;

    Asset(const Asset&) = delete;
    auto operator=(const Asset&) -> Asset& = delete;
    Asset(Asset&&) = delete;
    auto operator=(Asset&&) -> Asset& = delete;

protected:
    Asset() = default;
};

} // namespace buddd::engine

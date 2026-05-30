#pragma once

#include <cstdint>
#include <type_traits>

namespace buddd::engine {

struct EntityId {
    uint32_t index;
    uint32_t generation;

    static constexpr auto none() noexcept -> EntityId {
        return EntityId{UINT32_MAX, UINT32_MAX};
    }

    auto operator==(const EntityId&) const noexcept -> bool = default;
    auto operator!=(const EntityId&) const noexcept -> bool = default;
};

static_assert(std::is_trivially_copyable_v<EntityId>,
    "EntityId must be trivially copyable");
static_assert(sizeof(EntityId) == 8,
    "EntityId must be 8 bytes");

} // namespace buddd::engine

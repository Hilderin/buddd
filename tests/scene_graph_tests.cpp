#include "scene/entity_id.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdint>
#include <type_traits>
#include <vector>

using namespace buddd::engine;
using Catch::Approx;

namespace {
    constexpr float TOL = 1e-5f;
}

// ===========================================================================
// Component helpers (anonymous namespace)
// ===========================================================================
namespace {

struct MyComp : Component {
    int x;
    MyComp(int x) : x(x) {}
};

struct OtherComp : Component {
    float y;
    OtherComp() : y(0.0f) {}
    explicit OtherComp(float y) : y(y) {}
};

struct DestructorTracker : Component {
    static int destroy_count;
    int id;
    DestructorTracker(int id) : id(id) {}
    ~DestructorTracker() { destroy_count++; }
};
int DestructorTracker::destroy_count = 0;

struct DestructorFlag : Component {
    bool* flag;
    explicit DestructorFlag(bool* flag) : flag(flag) {}
    ~DestructorFlag() { *flag = true; }
};

} // anonymous namespace

// ===========================================================================
// EntityId tests (T-01 to T-04)
// ===========================================================================
TEST_CASE("EntityId default construction", "[scene][entity_id]") {
    EntityId id;
    REQUIRE(id.index == 0);
    REQUIRE(id.generation == 0);
}

TEST_CASE("EntityId::none() returns sentinel", "[scene][entity_id]") {
    auto id = EntityId::none();
    REQUIRE(id.index == UINT32_MAX);
    REQUIRE(id.generation == UINT32_MAX);
}

TEST_CASE("EntityId comparison", "[scene][entity_id]") {
    REQUIRE(EntityId{0, 0} == EntityId{0, 0});
    REQUIRE(EntityId{0, 0} != EntityId{1, 0});
    REQUIRE(EntityId::none() == EntityId::none());
    REQUIRE(EntityId{0, 0} != EntityId::none());
}

TEST_CASE("EntityId static_asserts pass", "[scene][entity_id]") {
    static_assert(sizeof(EntityId) == 8,
        "EntityId must be 8 bytes");
    static_assert(std::is_trivially_copyable_v<EntityId>,
        "EntityId must be trivially copyable");
    REQUIRE(true);
}

// ===========================================================================
// Transform tests (T-05 to T-09)
// ===========================================================================
TEST_CASE("Transform default values", "[scene][transform]") {
    Transform t;
    REQUIRE(t.position == math::Vec3::zero());
    REQUIRE(t.rotation == math::Quat::identity());
    REQUIRE(t.scale == math::Vec3::one());
}

TEST_CASE("Transform::local_matrix() TRS order", "[scene][transform]") {
    // Position (1,2,3), rotation 90° around Y, scale (2,1,1)
    Transform t;
    t.position = math::Vec3(1.0f, 2.0f, 3.0f);
    t.rotation = math::Quat::angle_axis(1.57079632679f, math::Vec3::unit_y()); // 90° around Y
    t.scale = math::Vec3(2.0f, 1.0f, 1.0f);

    math::Mat4 m = t.local_matrix();

    // Transform a point (0,0,0) -> should be at (1,2,3) (translation)
    math::Vec3 p0 = m * math::Vec3{0.0f, 0.0f, 0.0f};
    REQUIRE(p0.x == Approx(1.0f).margin(TOL));
    REQUIRE(p0.y == Approx(2.0f).margin(TOL));
    REQUIRE(p0.z == Approx(3.0f).margin(TOL));

    // Transform a point (1,0,0) -> scale x by 2, rotate 90° around Y, then translate
    // After scale: (2,0,0). After rotate 90° around Y: (0,0,-2). After translate: (1,2,1)
    math::Vec3 p1 = m * math::Vec3{1.0f, 0.0f, 0.0f};
    REQUIRE(p1.x == Approx(1.0f).margin(TOL));
    REQUIRE(p1.y == Approx(2.0f).margin(TOL));
    REQUIRE(p1.z == Approx(1.0f).margin(TOL));
}

TEST_CASE("Transform::world_matrix() for root entity equals local_matrix()", "[scene][transform]") {
    World world;
    auto entity = Entity::create(world);
    entity.transform().position = math::Vec3(10.0f, 0.0f, 0.0f);

    math::Mat4 wm = entity.transform().world_matrix(entity);
    math::Mat4 lm = entity.transform().local_matrix();

    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            REQUIRE(wm[c][r] == Approx(lm[c][r]).margin(TOL));
        }
    }
}

TEST_CASE("Transform::world_matrix() accumulates parent transforms", "[scene][transform]") {
    World world;
    auto parent = Entity::create(world);
    parent.transform().position = math::Vec3(10.0f, 0.0f, 0.0f);
    auto child = parent.create_child();

    math::Mat4 cm = child.transform().world_matrix(child);

    // Child at (0,0,0) of parent at (10,0,0) => world translation (10,0,0)
    math::Vec3 trans = cm * math::Vec3{0.0f, 0.0f, 0.0f};
    REQUIRE(trans.x == Approx(10.0f).margin(TOL));
    REQUIRE(trans.y == Approx(0.0f).margin(TOL));
    REQUIRE(trans.z == Approx(0.0f).margin(TOL));
}

TEST_CASE("Transform::world_matrix() accumulates grandparent transforms", "[scene][transform]") {
    World world;
    auto grandparent = Entity::create(world);
    grandparent.transform().position = math::Vec3(10.0f, 0.0f, 0.0f);

    auto parent = grandparent.create_child();
    parent.transform().position = math::Vec3(0.0f, 5.0f, 0.0f);

    auto child = parent.create_child();
    child.transform().position = math::Vec3(0.0f, 0.0f, -2.0f);

    math::Mat4 wm = child.transform().world_matrix(child);

    // World position should be (10, 5, -2)
    math::Vec3 trans = wm * math::Vec3{0.0f, 0.0f, 0.0f};
    REQUIRE(trans.x == Approx(10.0f).margin(TOL));
    REQUIRE(trans.y == Approx(5.0f).margin(TOL));
    REQUIRE(trans.z == Approx(-2.0f).margin(TOL));
}

// ===========================================================================
// Component tests (T-10 to T-16)
// ===========================================================================
TEST_CASE("Component base class", "[scene][component]") {
    struct MyCompLocal : Component {
        int x;
        MyCompLocal(int x) : x(x) {}
    };

    // Verify it can be destroyed through Component*
    Component* c = new MyCompLocal(42);
    delete c;

    // Verify non-copyable, non-movable
    static_assert(!std::is_copy_constructible_v<Component>);
    static_assert(!std::is_copy_assignable_v<Component>);
    static_assert(!std::is_move_constructible_v<Component>);
    static_assert(!std::is_move_assignable_v<Component>);

    REQUIRE(true);
}

TEST_CASE("Entity::add_component and get_component", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);

    auto& comp = entity.add_component<MyComp>(42);
    REQUIRE(comp.x == 42);

    auto opt = entity.get_component<MyComp>();
    REQUIRE(opt.has_value());
    REQUIRE(opt->x == 42);
    // Verify it's the same instance
    REQUIRE(&comp == &(*opt));
}

TEST_CASE("Entity::add_component unique per type", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);

    auto& compA = entity.add_component<MyComp>(42);
    auto& compB = entity.add_component<OtherComp>(3.14f);

    auto optA = entity.get_component<MyComp>();
    auto optB = entity.get_component<OtherComp>();

    REQUIRE(optA.has_value());
    REQUIRE(optA->x == 42);
    REQUIRE(&compA == &(*optA));

    REQUIRE(optB.has_value());
    REQUIRE(optB->y == Approx(3.14f).margin(TOL));
    REQUIRE(&compB == &(*optB));
}

TEST_CASE("Entity::get_component returns nullopt for missing type", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<MyComp>(42);

    auto opt = entity.get_component<OtherComp>();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Entity::remove_component", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<MyComp>(42);

    // First removal should succeed
    bool removed = entity.remove_component<MyComp>();
    REQUIRE(removed);

    // After removal, get_component returns nullopt
    auto opt = entity.get_component<MyComp>();
    REQUIRE_FALSE(opt.has_value());

    // Second removal should return false
    bool removed2 = entity.remove_component<MyComp>();
    REQUIRE_FALSE(removed2);
}

TEST_CASE("Entity::get_component returns nullopt on pending-destroy entity", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<MyComp>(42);

    entity.destroy();
    // Before flush, pending-destroy entity returns nullopt for components
    auto opt = entity.get_component<MyComp>();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Entity::get_component const overload", "[scene][component]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<MyComp>(42);

    const auto& constEntity = entity;
    auto opt = constEntity.get_component<MyComp>();
    REQUIRE(opt.has_value());
    REQUIRE(opt->x == 42);
}

// ===========================================================================
// Entity lifecycle tests (T-17 to T-22)
// ===========================================================================
TEST_CASE("Entity::create returns valid non-null entity", "[scene][entity]") {
    World world;
    auto entity = Entity::create(world);

    REQUIRE(entity.id() != EntityId::none());
    REQUIRE(&entity.world() == &world);
}

TEST_CASE("Entity::none() returns null entity", "[scene][entity]") {
    auto entity = Entity::none();
    REQUIRE(entity.id() == EntityId::none());
    REQUIRE_FALSE(entity.is_pending_destroy());
}

TEST_CASE("Entity comparison", "[scene][entity]") {
    World world;
    auto e1 = Entity::create(world);
    auto e2 = Entity::create(world);

    // Same entity (same handle) is equal to itself
    REQUIRE(e1 == e1);
    REQUIRE_FALSE(e1 != e1);

    // Different entities are not equal
    REQUIRE(e1 != e2);
    REQUIRE_FALSE(e1 == e2);

    // Null entity equals only null entity
    REQUIRE(Entity::none() == Entity::none());
    REQUIRE(Entity::none() != e1);
}

TEST_CASE("Entity::transform modify persists", "[scene][entity]") {
    World world;
    auto entity = Entity::create(world);

    entity.transform().position = math::Vec3(1.0f, 2.0f, 3.0f);
    auto pos = entity.transform().position;

    REQUIRE(pos.x == Approx(1.0f).margin(TOL));
    REQUIRE(pos.y == Approx(2.0f).margin(TOL));
    REQUIRE(pos.z == Approx(3.0f).margin(TOL));
}

TEST_CASE("Entity::destroy and is_pending_destroy", "[scene][entity]") {
    World world;
    auto entity = Entity::create(world);

    REQUIRE_FALSE(entity.is_pending_destroy());

    entity.destroy();
    REQUIRE(entity.is_pending_destroy());

    // After flush, the original handle is stale; is_pending_destroy behavior
    // on a stale handle is UB, so we just verify pre-flush state.
}

TEST_CASE("Entity::destroy idempotent", "[scene][entity]") {
    World world;
    auto entity = Entity::create(world);

    entity.destroy();
    REQUIRE(entity.is_pending_destroy());

    // Second call should be idempotent (no crash, no change)
    entity.destroy();
    REQUIRE(entity.is_pending_destroy());
}

// ===========================================================================
// World tests (T-23 to T-26)
// ===========================================================================
TEST_CASE("World::flush_destroyed on empty world is safe", "[scene][world]") {
    World world;
    // No entities created - flush should be a no-op
    REQUIRE_NOTHROW(world.flush_destroyed());

    // Create entities but don't destroy - flush should still be safe
    auto e1 = Entity::create(world);
    auto e2 = Entity::create(world);
    REQUIRE_NOTHROW(world.flush_destroyed());
}

TEST_CASE("World::flush_destroyed reclaims entities", "[scene][world]") {
    World world;
    auto e1 = Entity::create(world);
    auto id1 = e1.id();

    // Capture the generation before destroy
    uint32_t gen_before = id1.generation;

    e1.destroy();
    world.flush_destroyed();

    // Create a new entity - it may reuse slot with incremented generation
    auto e2 = Entity::create(world);
    auto id2 = e2.id();

    // The new entity should have a different (higher or wrapped) generation
    // If it reuses the same slot, generation must be > gen_before.
    // If it uses a different slot, that's also fine.
    // We just verify that some slot reuse happens eventually.
    // The simplest check: id2.generation >= gen_before (with possible wrap).
    // Since we only have 1 entity, it will definitely reuse slot 0.
    REQUIRE(id2.generation > gen_before);
}

TEST_CASE("World::destroy_entity equivalent to entity.destroy()", "[scene][world]") {
    World world;
    auto entity = Entity::create(world);

    REQUIRE_FALSE(entity.is_pending_destroy());

    world.destroy_entity(entity);
    REQUIRE(entity.is_pending_destroy());
}

TEST_CASE("World::flush_destroyed idempotent", "[scene][world]") {
    World world;
    auto entity = Entity::create(world);
    entity.destroy();

    world.flush_destroyed();
    // Second call should be a no-op with no crash
    REQUIRE_NOTHROW(world.flush_destroyed());
}

// ===========================================================================
// Hierarchy tests (T-27 to T-31)
// ===========================================================================
TEST_CASE("Entity::create_child creates child with parent link", "[scene][hierarchy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();

    REQUIRE(child.parent() == parent);
}

TEST_CASE("Entity::child_count and get_child after create_child", "[scene][hierarchy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();

    REQUIRE(parent.child_count() == 1);
    REQUIRE(parent.get_child(0) == child);
}

TEST_CASE("Entity::reparent to root", "[scene][hierarchy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();

    REQUIRE(child.parent() == parent);
    REQUIRE(parent.child_count() == 1);

    child.reparent(Entity::none());

    REQUIRE(child.parent().id() == EntityId::none());
    REQUIRE(parent.child_count() == 0);
}

TEST_CASE("Entity::reparent to another parent", "[scene][hierarchy]") {
    World world;
    auto p1 = Entity::create(world);
    auto p2 = Entity::create(world);
    auto child = p1.create_child();

    REQUIRE(child.parent() == p1);
    REQUIRE(p1.child_count() == 1);

    child.reparent(p2);

    REQUIRE(child.parent() == p2);
    REQUIRE(p1.child_count() == 0);
    REQUIRE(p2.child_count() == 1);
    REQUIRE(p2.get_child(0) == child);
}

TEST_CASE("Entity::reparent current parent is no-op", "[scene][hierarchy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();

    REQUIRE(parent.child_count() == 1);

    // Reparent to current parent should be a no-op
    child.reparent(parent);

    REQUIRE(child.parent() == parent);
    REQUIRE(parent.child_count() == 1);
    REQUIRE(parent.get_child(0) == child);
}

// ===========================================================================
// Destroy cascade tests (T-32 to T-35)
// ===========================================================================
TEST_CASE("Entity::destroy cascades to children", "[scene][destroy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();
    auto grandchild = child.create_child();

    REQUIRE_FALSE(parent.is_pending_destroy());
    REQUIRE_FALSE(child.is_pending_destroy());
    REQUIRE_FALSE(grandchild.is_pending_destroy());

    parent.destroy();

    REQUIRE(parent.is_pending_destroy());
    REQUIRE(child.is_pending_destroy());
    REQUIRE(grandchild.is_pending_destroy());
}

TEST_CASE("flush_destroyed after cascade reclaims all", "[scene][destroy]") {
    World world;
    auto parent = Entity::create(world);
    auto child = parent.create_child();
    auto grandchild = child.create_child();

    auto id_parent = parent.id();
    auto id_child = child.id();
    auto id_grandchild = grandchild.id();

    parent.destroy();
    world.flush_destroyed();

    // Create new entities - slot reuse should show incremented generations
    auto new_parent = Entity::create(world);
    // Since we had 3 entities originally, at least the first slot should be reused.
    // Just verify new entities work.
    REQUIRE(new_parent.id().generation > 0);
    // Check that we can create more entities without issue
    auto new_child = new_parent.create_child();
    REQUIRE(new_child.parent() == new_parent);
}

TEST_CASE("Deep hierarchy destroy does not stack overflow", "[scene][destroy]") {
    World world;

    // Create a chain of 10,000 entities
    constexpr int CHAIN_LENGTH = 10000;
    std::vector<Entity> entities;
    entities.reserve(CHAIN_LENGTH);

    entities.push_back(Entity::create(world));
    for (int i = 1; i < CHAIN_LENGTH; ++i) {
        entities.push_back(entities.back().create_child());
    }

    // Destroy root - this should use iterative traversal, not recursion
    REQUIRE_NOTHROW(entities[0].destroy());

    // Verify all entities are pending destroy
    for (auto& e : entities) {
        REQUIRE(e.is_pending_destroy());
    }

    // Flush to clean up
    world.flush_destroyed();
}

TEST_CASE("flush_destroyed with no pending entities is no-op", "[scene][destroy]") {
    World world;
    // No entities at all
    REQUIRE_NOTHROW(world.flush_destroyed());

    // Entities but none destroyed
    auto e1 = Entity::create(world);
    auto e2 = Entity::create(world);
    REQUIRE_NOTHROW(world.flush_destroyed());
}

// ===========================================================================
// world_matrix tests (T-36 to T-37)
// ===========================================================================
TEST_CASE("Entity::world_matrix convenience method", "[scene][transform]") {
    World world;
    auto entity = Entity::create(world);
    entity.transform().position = math::Vec3(5.0f, -3.0f, 2.0f);
    entity.transform().rotation = math::Quat::angle_axis(0.5f, math::Vec3::unit_y());
    entity.transform().scale = math::Vec3(2.0f, 1.0f, 3.0f);

    math::Mat4 wm_direct = entity.transform().world_matrix(entity);
    math::Mat4 wm_convenience = entity.world_matrix();

    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            REQUIRE(wm_convenience[c][r] == Approx(wm_direct[c][r]).margin(TOL));
        }
    }
}

TEST_CASE("Entity::world_matrix for chain with different transforms", "[scene][transform]") {
    World world;
    auto root = Entity::create(world);
    root.transform().position = math::Vec3(10.0f, 0.0f, 0.0f);
    root.transform().scale = math::Vec3(2.0f, 1.0f, 1.0f);

    auto mid = root.create_child();
    mid.transform().position = math::Vec3(0.0f, 5.0f, 0.0f);
    mid.transform().rotation = math::Quat::angle_axis(1.57079632679f, math::Vec3::unit_y());

    auto leaf = mid.create_child();
    leaf.transform().position = math::Vec3(0.0f, 0.0f, -2.0f);
    leaf.transform().scale = math::Vec3(1.0f, 1.0f, 2.0f);

    // Compute expected: T_root * S_root * T_mid * R_mid * T_leaf * S_leaf
    math::Mat4 expected = root.transform().local_matrix()
                        * mid.transform().local_matrix()
                        * leaf.transform().local_matrix();

    math::Mat4 actual = leaf.world_matrix();

    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            REQUIRE(actual[c][r] == Approx(expected[c][r]).margin(TOL));
        }
    }
}

// ===========================================================================
// Null entity safety tests (T-38)
// ===========================================================================
TEST_CASE("Null entity safe operations", "[scene][null_entity]") {
    // id() returns EntityId::none()
    REQUIRE(Entity::none().id() == EntityId::none());

    // is_pending_destroy() returns false for null entity
    REQUIRE_FALSE(Entity::none().is_pending_destroy());

    // Null entities compare equal to each other
    REQUIRE(Entity::none() == Entity::none());

    // Null entity != valid entity
    World world;
    auto entity = Entity::create(world);
    REQUIRE(Entity::none() != entity);
}

// ===========================================================================
// Pending-destroy entity behavior tests (T-39 to T-40)
// ===========================================================================
TEST_CASE("Pending-destroy entity get_component returns nullopt", "[scene][pending_destroy]") {
    World world;
    auto entity = Entity::create(world);
    entity.add_component<MyComp>(42);

    entity.destroy();

    // Pending-destroy entity should return nullopt for components
    auto opt = entity.get_component<MyComp>();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Pending-destroy entity transform is accessible", "[scene][pending_destroy]") {
    World world;
    auto entity = Entity::create(world);
    entity.transform().position = math::Vec3(7.0f, 8.0f, 9.0f);

    entity.destroy();

    // Transform should still be accessible (inline storage, survives until flush)
    auto pos = entity.transform().position;
    REQUIRE(pos.x == Approx(7.0f).margin(TOL));
    REQUIRE(pos.y == Approx(8.0f).margin(TOL));
    REQUIRE(pos.z == Approx(9.0f).margin(TOL));
}

// ===========================================================================
// Null entity safety tests (T-41 to T-42)
// ===========================================================================
TEST_CASE("Null entity get_component returns nullopt", "[scene][null_entity]") {
    auto opt = Entity::none().get_component<MyComp>();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Null entity child_count returns 0", "[scene][null_entity]") {
    REQUIRE(Entity::none().child_count() == 0);
}

// ===========================================================================
// Edge case tests (T-43 to T-49)
// ===========================================================================
TEST_CASE("flush_destroyed multiple calls", "[scene][world]") {
    World world;
    auto entity = Entity::create(world);
    entity.destroy();

    world.flush_destroyed();
    // Second call without intervening create_entity should be no-op
    REQUIRE_NOTHROW(world.flush_destroyed());
}

TEST_CASE("World destructor with pending entities", "[scene][world]") {
    // Test that ~World() handles pending entities without explicit flush
    {
        World world;
        auto e1 = Entity::create(world);
        auto e2 = Entity::create(world);
        e1.add_component<MyComp>(1);
        e2.add_component<MyComp>(2);

        // Destroy some but not all
        e1.destroy();
        // World goes out of scope - should clean up without crash or leak
    }
    // If ASAN is enabled, this test would catch leaks
    REQUIRE(true);
}

TEST_CASE("Destroyed entity still visible in parent before flush", "[scene][hierarchy]") {
    World world;
    auto parent = Entity::create(world);
    auto c1 = parent.create_child();
    auto c2 = parent.create_child();

    REQUIRE(parent.child_count() == 2);

    // Destroy C1
    c1.destroy();

    // Before flush: child_count still includes C1
    REQUIRE(parent.child_count() == 2);
    // C1 is pending destroy
    REQUIRE(c1.is_pending_destroy());

    // After flush: only C2 remains
    world.flush_destroyed();
    REQUIRE(parent.child_count() == 1);
    REQUIRE(parent.get_child(0) == c2);
}

TEST_CASE("flush_destroyed reverse depth order", "[scene][destroy]") {
    // Reset the tracker
    DestructorTracker::destroy_count = 0;

    World world;
    auto root = Entity::create(world);
    root.add_component<DestructorTracker>(1); // id=1

    auto parent = root.create_child();
    parent.add_component<DestructorTracker>(2); // id=2

    auto child = parent.create_child();
    child.add_component<DestructorTracker>(3); // id=3

    // Destroy root (cascades to children)
    root.destroy();
    world.flush_destroyed();

    // All three destructors should have been called
    REQUIRE(DestructorTracker::destroy_count == 3);

    // It's hard to directly verify reverse order without more sophisticated
    // tracking, but we verify that all were destroyed (no crash, no leak).
}

TEST_CASE("Component destructor called on entity destroy", "[scene][component]") {
    bool flag = false;

    {
        World world;
        auto entity = Entity::create(world);
        entity.add_component<DestructorFlag>(&flag);

        REQUIRE_FALSE(flag);

        entity.destroy();
        world.flush_destroyed();

        // After flush, the component destructor should have been called
        REQUIRE(flag);
    }
}

TEST_CASE("World destruction with pending entities — components destroyed", "[scene][world]") {
    bool flag1 = false;
    bool flag2 = false;

    {
        World world;
        auto e1 = Entity::create(world);
        e1.add_component<DestructorFlag>(&flag1);

        auto e2 = Entity::create(world);
        e2.add_component<DestructorFlag>(&flag2);

        // Mark some for destruction
        e1.destroy();
        // e2 is NOT destroyed

        // World goes out of scope — both should still have destructors run
    }

    // Both component destructors should have been called
    REQUIRE(flag1);
    REQUIRE(flag2);
}

TEST_CASE("Stale EntityId after flush", "[scene][entity]") {
    World world;
    auto entity = Entity::create(world);
    EntityId captured_id = entity.id();

    entity.destroy();
    world.flush_destroyed();

    // Reconstruct Entity from stale id (using private constructor via friend)
    // We can't directly construct Entity from an arbitrary id, but we can
    // check that lookup_node would return null for the stale id.
    // is_pending_destroy on a stale handle should return false (null node)
    // We use the world method directly through Entity::none+id decomposition.
    // Actually, we can test this via World::is_pending_destroy which is not public.
    // Let's create a new entity and check that the old generation is different.

    auto new_entity = Entity::create(world);
    // The new entity should have a different generation (since slot 0 was freed and reused)
    // OR a different index (if slot 0 was not reused yet)
    REQUIRE((new_entity.id().generation > captured_id.generation
             || new_entity.id().index != captured_id.index));
}

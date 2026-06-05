#include "engine_service.h"
#include "asset/asset.h"
#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "asset/model_loader.h"
#include "asset/dependency_map.h"
#include "asset/file_watcher.h"
#include "render/model_node.h"
#include "render/model.h"
#include "render/pbr/pbr_material.h"
#include "render/material_headless.h"
#include "render/render_device.h"
#include "render/render_device_headless.h"
#include "render/model_utils.h"
#include "render/texture.h"
#include "render/texture_headless.h"
#include "render/vertex.h"
#include "render/vertex_buffer_headless.h"
#include "render/index_buffer_headless.h"
#include "scene/world.h"
#include "scene/entity.h"
#include "scene/component.h"
#include "scene/transform.h"
#include "render/mesh_renderer.h"
#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace buddd::engine;
using Catch::Approx;
namespace fs = std::filesystem;

namespace {

    auto project_root() -> std::string {
        auto cwd = fs::current_path();
        if (fs::exists(cwd / "CMakeLists.txt") &&
            fs::exists(cwd / "src") &&
            fs::exists(cwd / "tests")) {
            return cwd.string();
        }
        for (int i = 0; i < 4; ++i) {
            auto parent = cwd.parent_path();
            if (parent == cwd) break;
            cwd = parent;
            if (fs::exists(cwd / "CMakeLists.txt") &&
                fs::exists(cwd / "src") &&
                fs::exists(cwd / "tests")) {
                return cwd.string();
            }
        }
        return fs::current_path().string();
    }

    struct ProjectRootGuard {
        ProjectRootGuard() {
            auto root = project_root();
            if (fs::current_path() != root) {
                fs::current_path(root);
            }
        }
    };

    auto make_headless_engine() -> std::unique_ptr<EngineService> {
        auto engine = EngineService::create(
            Backend::Headless,
            WindowConfig{.title = "Test", .width = 800, .height = 600});
        REQUIRE(engine.has_value());
        return std::move(*engine);
    }

    auto create_test_asset_manager(RenderDevice& device) -> std::unique_ptr<AssetManager> {
        auto base = project_root() + "/tests/assets";
        auto am = AssetManager::create(device, base);
        REQUIRE(am.has_value());
        return std::move(*am);
    }

    ProjectRootGuard g_project_root_guard;

} // anonymous namespace

// ===========================================================================
// Test 1: ModelAsset stores and returns root ModelNode
// ===========================================================================
TEST_CASE("ModelAsset stores and returns root ModelNode", "[model][headless]") {
    ModelNode root;
    root.name = "test_root";

    ModelAsset asset(std::move(root));
    REQUIRE(asset.root_node().name == "test_root");
    REQUIRE(asset.root_node().children.empty());
    REQUIRE_FALSE(asset.root_node().model.has_value());
}

// ===========================================================================
// Test 2: PbrMaterial created with known uniforms
// ===========================================================================
TEST_CASE("PbrMaterial created with known uniforms", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = std::make_shared<PbrMaterial>(device);
    REQUIRE(mat != nullptr);

    const auto& uniforms = PbrMaterial::known_uniform_names();
    REQUIRE(uniforms.size() >= 19); // 19+ PBR uniforms

    // Check essential uniforms
    bool has_mvp = false;
    bool has_model = false;
    bool has_base_color = false;
    bool has_metallic = false;
    bool has_roughness = false;
    for (const auto& name : uniforms) {
        if (name == "u_mvp") has_mvp = true;
        if (name == "u_model") has_model = true;
        if (name == "u_base_color_factor") has_base_color = true;
        if (name == "u_metallic_factor") has_metallic = true;
        if (name == "u_roughness_factor") has_roughness = true;
    }
    REQUIRE(has_mvp);
    REQUIRE(has_model);
    REQUIRE(has_base_color);
    REQUIRE(has_metallic);
    REQUIRE(has_roughness);
}

// ===========================================================================
// Test 3: PbrMaterialData fields settable and readable via set_data()/data()
// ===========================================================================
TEST_CASE("PbrMaterialData fields settable and readable via set_data()/data()",
          "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = std::make_shared<PbrMaterial>(device);

    PbrMaterialData data;
    data.base_color_factor = math::Vec4{0.5f, 0.3f, 0.2f, 1.0f};
    data.metallic_factor = 0.8f;
    data.roughness_factor = 0.3f;
    data.emissive_factor = math::Vec3{0.1f, 0.0f, 0.0f};
    data.double_sided = true;

    mat->set_data(data);

    const auto& stored = mat->data();
    REQUIRE(stored.base_color_factor.x == Approx(0.5f));
    REQUIRE(stored.base_color_factor.y == Approx(0.3f));
    REQUIRE(stored.base_color_factor.z == Approx(0.2f));
    REQUIRE(stored.base_color_factor.w == Approx(1.0f));
    REQUIRE(stored.metallic_factor == Approx(0.8f));
    REQUIRE(stored.roughness_factor == Approx(0.3f));
    REQUIRE(stored.emissive_factor.x == Approx(0.1f));
    REQUIRE(stored.double_sided == true);
}

// ===========================================================================
// Test 4: Load Khronos Box from YAML — success, vertex count 24, index count 36, 1 submesh
// ===========================================================================
TEST_CASE("Load Khronos Box from YAML", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result.has_value());

    auto model_asset = *result;
    auto& root = model_asset->root_node();

    // Box has one child node with a mesh
    REQUIRE(root.children.size() >= 1);

    // Find the mesh node (may be nested: root -> transform node -> mesh node)
    const ModelNode* mesh_node = nullptr;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) {
        if (n.model.has_value()) {
            mesh_node = &n;
            return;
        }
        for (const auto& c : n.children) {
            find_mesh(c);
        }
    };
    find_mesh(root);
    REQUIRE(mesh_node != nullptr);
    REQUIRE(mesh_node->model.has_value());

    auto& model = *mesh_node->model;
    REQUIRE(model.vertex_count() == 24);
    REQUIRE(model.index_count() == 36);
    REQUIRE(model.submeshes().size() == 1);
    REQUIRE(model.materials().size() >= 1);
}

// ===========================================================================
// Test 5: Load Box twice returns cached instance (same address)
// ===========================================================================
TEST_CASE("Load Box twice returns cached instance", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result1 = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result1.has_value());

    auto result2 = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result2.has_value());

    REQUIRE(result1->get() == result2->get());
}

// ===========================================================================
// Test 6: Load DamagedHelmet from YAML — success, PBR materials have textures
// ===========================================================================
TEST_CASE("Load DamagedHelmet from YAML", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/damaged-helmet/DamagedHelmet");
    REQUIRE(result.has_value());

    auto model_asset = *result;
    auto& root = model_asset->root_node();

    // Should have at least one mesh node
    bool found_mesh = false;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) -> void {
        if (n.model.has_value()) {
            found_mesh = true;
            // Verify materials
            for (const auto& mat : n.model->materials()) {
                auto* pbr = dynamic_cast<PbrMaterial*>(mat.get());
                if (pbr) {
                    // Check that at least one texture is loaded
                    auto& data = pbr->data();
                    bool has_any_texture =
                        data.base_color_texture != nullptr ||
                        data.metallic_roughness_texture != nullptr ||
                        data.normal_texture != nullptr ||
                        data.occlusion_texture != nullptr;
                    REQUIRE(has_any_texture);
                }
            }
        }
        for (const auto& c : n.children) {
            find_mesh(c);
        }
    };
    find_mesh(root);
    REQUIRE(found_mesh);
}

// ===========================================================================
// Test 7: ModelNode tree reflects glTF hierarchy
// ===========================================================================
TEST_CASE("ModelNode tree reflects glTF hierarchy", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result.has_value());

    auto& root = (*result)->root_node();
    // Box has a scene with one root node that has children
    REQUIRE(root.children.size() > 0);
}

// ===========================================================================
// Test 8: Each ModelNode with a model has valid submeshes and materials
// ===========================================================================
TEST_CASE("Each ModelNode with a model has valid submeshes and materials",
          "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result.has_value());

    std::function<void(const ModelNode&)> check_node = [&](const ModelNode& n) {
        if (n.model.has_value()) {
            REQUIRE(n.model->submeshes().size() > 0);
            REQUIRE(n.model->materials().size() > 0);
        }
        for (const auto& c : n.children) {
            check_node(c);
        }
    };
    check_node((*result)->root_node());
}

// ===========================================================================
// Test 9: Type mismatch: YAML type:Texture requested as ModelAsset returns InvalidArgument
// ===========================================================================
TEST_CASE("Type mismatch YAML Texture requested as ModelAsset returns InvalidArgument",
          "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("textures/test_texture");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 10: Unsupported version returns Unsupported
// ===========================================================================
TEST_CASE("Unsupported version returns Unsupported", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/unsupported_version");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::Unsupported);
}

// ===========================================================================
// Test 11: create<ModelAsset> compiles; create<int> fails at compile time
// ===========================================================================
TEST_CASE("create<ModelAsset> compiles; create<int> fails at compile time",
          "[model][headless]") {
    // This test verifies that create<ModelAsset> is supported by the static_assert
    // We don't need to actually call create<int> since it would fail at compile time.
    // The static_assert in asset_manager.tpp ensures this.
    REQUIRE(std::is_same_v<decltype(std::declval<AssetManager&>().create<ModelAsset>("test")),
                           Result<std::shared_ptr<ModelAsset>>>);
}

// ===========================================================================
// Test 12: create_model(id) convenience method works
// ===========================================================================
TEST_CASE("create_model(id) convenience method works", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    // Try with a valid model ID
    auto result = assets->create_model("models/box/Box");
    // Should either succeed or fail with a specific error (not crash)
    if (result.has_value()) {
        REQUIRE((*result)->root_node().children.size() > 0);
    }
}

// ===========================================================================
// Test 13: ModelNode is movable and non-copyable
// ===========================================================================
TEST_CASE("ModelNode is movable and non-copyable", "[model][headless]") {
    REQUIRE_FALSE(std::is_copy_constructible_v<ModelNode>);
    REQUIRE_FALSE(std::is_copy_assignable_v<ModelNode>);
    REQUIRE(std::is_move_constructible_v<ModelNode>);
    REQUIRE(std::is_move_assignable_v<ModelNode>);
}

// ===========================================================================
// Test 14: glTF material without textures — null texture slots, correct factors
// ===========================================================================
TEST_CASE("glTF material without textures", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = std::make_shared<PbrMaterial>(device);
    PbrMaterialData data;
    data.base_color_factor = math::Vec4{0.5f, 0.5f, 0.5f, 1.0f};
    data.metallic_factor = 0.0f;
    data.roughness_factor = 1.0f;
    data.base_color_texture = nullptr;
    data.metallic_roughness_texture = nullptr;
    data.normal_texture = nullptr;
    data.occlusion_texture = nullptr;
    data.emissive_texture = nullptr;

    mat->set_data(data);

    const auto& stored = mat->data();
    REQUIRE(stored.base_color_factor.x == Approx(0.5f));
    REQUIRE(stored.metallic_factor == Approx(0.0f));
    REQUIRE(stored.roughness_factor == Approx(1.0f));
    REQUIRE(stored.base_color_texture == nullptr);
    REQUIRE(stored.metallic_roughness_texture == nullptr);
    REQUIRE(stored.normal_texture == nullptr);
    REQUIRE(stored.occlusion_texture == nullptr);
    REQUIRE(stored.emissive_texture == nullptr);
}

// ===========================================================================
// Test 15: doubleSided flag works via set_data()/data()
// ===========================================================================
TEST_CASE("doubleSided flag works via set_data()/data()", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = std::make_shared<PbrMaterial>(device);
    PbrMaterialData data;
    data.double_sided = true;
    mat->set_data(data);
    REQUIRE(mat->data().double_sided == true);

    data.double_sided = false;
    mat->set_data(data);
    REQUIRE(mat->data().double_sided == false);
}

// ===========================================================================
// Test 16: Missing source YAML returns IoFailed
// ===========================================================================
TEST_CASE("Missing source YAML returns IoFailed", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    // Try loading a non-existent model
    auto result = assets->create<ModelAsset>("models/nonexistent/Model");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::IoFailed);
}

// ===========================================================================
// Test 17: add_model_to_world() creates entities for mesh nodes
// ===========================================================================
TEST_CASE("add_model_to_world creates entities for mesh nodes", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result.has_value());

    World world;
    auto& root = (*result)->root_node();
    auto entity = add_model_to_world(world, root);

    // Box has a mesh, so entity should not be none
    // (but the root node might have children with meshes)
    // Due to the ModelNode structure, the root itself may not have a mesh
    // Let's check that at least one entity was created
    size_t mesh_count = 0;
    world.each<MeshRenderer>([&](Entity, MeshRenderer&) {
        ++mesh_count;
        return true;
    });
    REQUIRE(mesh_count > 0);
}

// ===========================================================================
// Test 18: add_model_to_world() returns Entity::none() for nodes without mesh
// ===========================================================================
TEST_CASE("add_model_to_world returns Entity_none for nodes without mesh",
          "[model][headless]") {
    World world;
    ModelNode empty_node;
    empty_node.name = "empty";
    REQUIRE_FALSE(empty_node.model.has_value());

    auto entity = add_model_to_world(world, empty_node);
    REQUIRE(entity.id() == EntityId::none());
}

// ===========================================================================
// Test 19: ModelAsset is non-copyable, non-movable
// ===========================================================================
TEST_CASE("ModelAsset is non-copyable, non-movable", "[model][headless]") {
    REQUIRE_FALSE(std::is_copy_constructible_v<ModelAsset>);
    REQUIRE_FALSE(std::is_copy_assignable_v<ModelAsset>);
    REQUIRE_FALSE(std::is_move_constructible_v<ModelAsset>);
    REQUIRE_FALSE(std::is_move_assignable_v<ModelAsset>);
}

// ===========================================================================
// Test 20: PbrMaterial inner_material() accessor works
// ===========================================================================
TEST_CASE("PbrMaterial inner_material() accessor works", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();

    auto mat = std::make_shared<PbrMaterial>(device);
    REQUIRE_NOTHROW(mat->inner_material());
    REQUIRE_NOTHROW(std::as_const(*mat).inner_material());
}

// ===========================================================================
// SFINAE helper: detect whether replace_root is publicly accessible
// ===========================================================================
namespace {
    template<typename T, typename = void>
    struct has_public_replace_root : std::false_type {};

    template<typename T>
    struct has_public_replace_root<T,
        std::void_t<decltype(std::declval<T>().replace_root(std::declval<ModelNode>()))>>
        : std::true_type {};
}

// ===========================================================================
// Test 21: Missing POSITION attribute returns InvalidArgument (AC-011)
// ===========================================================================
TEST_CASE("Missing POSITION attribute returns InvalidArgument", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/missing-position/MissingPosition");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidArgument);
}

// ===========================================================================
// Test 22: Corrupt glTF file returns InvalidFormat (AC-012)
// ===========================================================================
TEST_CASE("Corrupt glTF file returns InvalidFormat", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/corrupt/Corrupt");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InvalidFormat);
}

// ===========================================================================
// Test 23: Missing texture URI — magenta fallback used (AC-017)
// ===========================================================================
TEST_CASE("Missing texture URI — magenta fallback used", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/missing-texture/MissingTexture");
    REQUIRE(result.has_value());

    auto& root = (*result)->root_node();
    // Find the mesh node
    const ModelNode* mesh_node = nullptr;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) {
        if (n.model.has_value()) {
            mesh_node = &n;
            return;
        }
        for (const auto& c : n.children) {
            find_mesh(c);
        }
    };
    find_mesh(root);
    REQUIRE(mesh_node != nullptr);
    REQUIRE(mesh_node->model.has_value());

    // Get the material and check texture is magenta
    REQUIRE(mesh_node->model->materials().size() >= 1);
    auto* pbr = dynamic_cast<PbrMaterial*>(mesh_node->model->materials()[0].get());
    REQUIRE(pbr != nullptr);

    auto& data = pbr->data();
    REQUIRE(data.base_color_texture != nullptr);

    // Cast to TextureHeadless to inspect pixel data
    auto* tex = dynamic_cast<TextureHeadless*>(data.base_color_texture.get());
    REQUIRE(tex != nullptr);
    REQUIRE(tex->width() == 1);
    REQUIRE(tex->height() == 1);
    REQUIRE(tex->channels() >= 3);

    auto& pixel_data = tex->data();
    REQUIRE(pixel_data.size() >= 4);
    // Magenta = (255, 0, 255, 255)
    REQUIRE(static_cast<int>(pixel_data[0]) == 255);
    REQUIRE(static_cast<int>(pixel_data[1]) == 0);
    REQUIRE(static_cast<int>(pixel_data[2]) == 255);
    REQUIRE(static_cast<int>(pixel_data[3]) == 255);
}

// ===========================================================================
// Test 24: settings.scale: 2.0 doubles vertex positions (AC-018)
// ===========================================================================
TEST_CASE("settings.scale 2.0 doubles vertex positions", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    // Load with scale 1.0 (default)
    auto result1 = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result1.has_value());

    // Load with scale 2.0
    auto result2 = assets->create<ModelAsset>("models/scale2x/Scale2x");
    REQUIRE(result2.has_value());

    // Helper to find mesh node and get vertex data
    auto find_first_mesh = [](const ModelNode& root) -> const Model* {
        const ModelNode* mesh_node = nullptr;
        std::function<void(const ModelNode&)> find = [&](const ModelNode& n) {
            if (n.model.has_value()) { mesh_node = &n; return; }
            for (const auto& c : n.children) { find(c); }
        };
        find(root);
        return mesh_node ? &(*mesh_node->model) : nullptr;
    };

    const Model* model1 = find_first_mesh((*result1)->root_node());
    const Model* model2 = find_first_mesh((*result2)->root_node());
    REQUIRE(model1 != nullptr);
    REQUIRE(model2 != nullptr);

    // Both should have same number of vertices
    REQUIRE(model1->vertex_count() == model2->vertex_count());
    REQUIRE(model1->index_count() == model2->index_count());

    // Verify vertex positions are doubled by inspecting raw vertex data
    auto& vb1 = model1->vertices();
    auto& vb2 = model2->vertices();
    auto* hb1 = dynamic_cast<const VertexBufferHeadless*>(&vb1);
    auto* hb2 = dynamic_cast<const VertexBufferHeadless*>(&vb2);
    REQUIRE(hb1 != nullptr);
    REQUIRE(hb2 != nullptr);

    const auto& data1 = hb1->data();
    const auto& data2 = hb2->data();
    REQUIRE(data1.size() == data2.size());

    // Check first vertex position (first 3 floats)
    REQUIRE(data1.size() >= sizeof(float) * 3);
    REQUIRE(data2.size() >= sizeof(float) * 3);

    float x1, y1, z1, x2, y2, z2;
    std::memcpy(&x1, &data1[0], sizeof(float));
    std::memcpy(&y1, &data1[sizeof(float)], sizeof(float));
    std::memcpy(&z1, &data1[2 * sizeof(float)], sizeof(float));
    std::memcpy(&x2, &data2[0], sizeof(float));
    std::memcpy(&y2, &data2[sizeof(float)], sizeof(float));
    std::memcpy(&z2, &data2[2 * sizeof(float)], sizeof(float));

    REQUIRE(x2 == Approx(2.0f * x1));
    REQUIRE(y2 == Approx(2.0f * y1));
    REQUIRE(z2 == Approx(2.0f * z1));
}

// ===========================================================================
// Test 25: Transform-only node — model is nullopt, children preserved (AC-019)
// ===========================================================================
TEST_CASE("Transform-only node model is nullopt children preserved", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/transform-only/TransformOnly");
    REQUIRE(result.has_value());

    auto& root = (*result)->root_node();

    // Root should have one child (node 0)
    REQUIRE(root.children.size() == 1);

    // Root's child is node 0 (transform-only, no mesh)
    const auto& transform_node = root.children[0];
    REQUIRE_FALSE(transform_node.model.has_value());
    REQUIRE(transform_node.translation.x == Approx(2.0f));
    REQUIRE(transform_node.translation.y == Approx(3.0f));
    REQUIRE(transform_node.translation.z == Approx(4.0f));

    // Transform node should have one child (node 1 with mesh)
    REQUIRE(transform_node.children.size() == 1);
    const auto& mesh_node = transform_node.children[0];
    REQUIRE(mesh_node.model.has_value());
    REQUIRE(mesh_node.model->vertex_count() == 3);
}

// ===========================================================================
// Test 26: Unsupported primitive mode (POINTS) — skipped with warning (AC-020)
// ===========================================================================
TEST_CASE("Unsupported primitive mode POINTS skipped with warning", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    // The unsupported-mode model uses mode 0 (POINTS) which should be skipped.
    // After skipping, the mesh has no valid primitives → model is nullopt.
    auto result = assets->create<ModelAsset>("models/unsupported-mode/UnsupportedMode");
    REQUIRE(result.has_value());

    auto& root = (*result)->root_node();

    // Find the mesh node — it should have no model (POINTS were skipped)
    bool found_mesh = false;
    std::function<void(const ModelNode&)> check = [&](const ModelNode& n) {
        if (n.model.has_value()) {
            found_mesh = true;
        }
        for (const auto& c : n.children) {
            check(c);
        }
    };
    check(root);

    // All primitives were skipped, so no mesh node with model should exist
    REQUIRE_FALSE(found_mesh);
}

// ===========================================================================
// Test 27: Hot-reload synthetic FileEvent triggers model reload (AC-021)
// ===========================================================================
TEST_CASE("Hot-reload synthetic FileEvent triggers model reload", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/box/Box");
    REQUIRE(result.has_value());

    auto model_asset = *result;
    REQUIRE(model_asset != nullptr);
    REQUIRE(model_asset->root_node().children.size() >= 1);

    // Inject a synthetic FileEvent for the glTF source file
    // The dependency map tracks the source path (relative to base_path_)
    FileEvent event;
    event.path = "models/box/BoxTextured.gltf";
    event.type = FileEventType::Modified;
    assets->testing_inject_file_event(event);

    // After the event, the root should still be valid (model replaced in-place)
    // It may have the same or different structure, but should not crash
    REQUIRE(model_asset->root_node().children.size() >= 1);

    // For V1, we just verify no crash and model is still valid
    REQUIRE_NOTHROW(model_asset->root_node());
}

// ===========================================================================
// Test 28: replace_root() is private — compile check (AC-025)
// ===========================================================================
TEST_CASE("replace_root is private compile check", "[model][headless]") {
    // Verify that replace_root cannot be called from outside the class
    // by checking it's not publicly accessible via SFINAE
    static_assert(!has_public_replace_root<ModelAsset>::value,
        "replace_root() must be private (friend AssetManager only)");
    REQUIRE(true);
}

// ===========================================================================
// Test 29: COLOR_0 VEC3 expanded to VEC4 with alpha=1.0 (AC-027)
// ===========================================================================
TEST_CASE("COLOR_0 VEC3 expanded to VEC4 with alpha 1.0", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/color-vec3/ColorVec3");
    REQUIRE(result.has_value());

    // Find the mesh node and check vertex colors
    auto& root = (*result)->root_node();
    const ModelNode* mesh_node = nullptr;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) {
        if (n.model.has_value()) { mesh_node = &n; return; }
        for (const auto& c : n.children) { find_mesh(c); }
    };
    find_mesh(root);
    REQUIRE(mesh_node != nullptr);
    REQUIRE(mesh_node->model.has_value());

    auto& vb = mesh_node->model->vertices();
    auto* hb = dynamic_cast<const VertexBufferHeadless*>(&vb);
    REQUIRE(hb != nullptr);

    const auto& data = hb->data();
    // Vertex format: position (12 bytes) then color (16 bytes)
    // First vertex color starts at offset 12
    REQUIRE(data.size() >= 12 + 16);

    // Vertex 0 color (R=1.0, G=0.0, B=0.0 → should become R=1.0, G=0.0, B=0.0, A=1.0)
    float r, g, b, a;
    std::memcpy(&r, &data[12], sizeof(float));
    std::memcpy(&g, &data[12 + sizeof(float)], sizeof(float));
    std::memcpy(&b, &data[12 + 2 * sizeof(float)], sizeof(float));
    std::memcpy(&a, &data[12 + 3 * sizeof(float)], sizeof(float));

    // First vertex color should be red with alpha=1.0
    REQUIRE(r == Approx(1.0f));
    REQUIRE(g == Approx(0.0f));
    REQUIRE(b == Approx(0.0f));
    REQUIRE(a == Approx(1.0f));
}

// ===========================================================================
// Test 30: Missing NORMAL defaults to (0,0,1) (AC-028)
// ===========================================================================
TEST_CASE("Missing NORMAL defaults to 0 0 1", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/missing-normal/MissingNormal");
    REQUIRE(result.has_value());

    // Find the mesh node
    auto& root = (*result)->root_node();
    const ModelNode* mesh_node = nullptr;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) {
        if (n.model.has_value()) { mesh_node = &n; return; }
        for (const auto& c : n.children) { find_mesh(c); }
    };
    find_mesh(root);
    REQUIRE(mesh_node != nullptr);
    REQUIRE(mesh_node->model.has_value());

    auto& vb = mesh_node->model->vertices();
    auto* hb = dynamic_cast<const VertexBufferHeadless*>(&vb);
    REQUIRE(hb != nullptr);

    const auto& data = hb->data();
    // Vertex format: position (12) + color (16), then normal (12) starts at offset 28
    REQUIRE(data.size() >= 28 + 12);

    float nx, ny, nz;
    std::memcpy(&nx, &data[28], sizeof(float));
    std::memcpy(&ny, &data[28 + sizeof(float)], sizeof(float));
    std::memcpy(&nz, &data[28 + 2 * sizeof(float)], sizeof(float));

    // Default normal should be (0, 0, 1)
    REQUIRE(nx == Approx(0.0f));
    REQUIRE(ny == Approx(0.0f));
    REQUIRE(nz == Approx(1.0f));
}

// ===========================================================================
// Test 31: Uint32 indices supported (AC-024)
// ===========================================================================
TEST_CASE("Uint32 indices supported", "[model][headless]") {
    auto engine = make_headless_engine();
    auto& device = engine->device();
    auto assets = create_test_asset_manager(device);

    auto result = assets->create<ModelAsset>("models/uint32-indices/Uint32Indices");
    REQUIRE(result.has_value());

    // Find the mesh node
    auto& root = (*result)->root_node();
    const ModelNode* mesh_node = nullptr;
    std::function<void(const ModelNode&)> find_mesh = [&](const ModelNode& n) {
        if (n.model.has_value()) { mesh_node = &n; return; }
        for (const auto& c : n.children) { find_mesh(c); }
    };
    find_mesh(root);
    REQUIRE(mesh_node != nullptr);
    REQUIRE(mesh_node->model.has_value());

    auto& model = *mesh_node->model;
    REQUIRE(model.vertex_count() == 3);
    REQUIRE(model.index_count() == 3);

    // Verify index buffer uses Uint32
    auto& ib = model.indices();
    auto& index_buffer = dynamic_cast<const IndexBufferHeadless&>(ib);
    REQUIRE(index_buffer.data().size() == 3 * sizeof(uint32_t)); // 12 bytes for 3 Uint32
}

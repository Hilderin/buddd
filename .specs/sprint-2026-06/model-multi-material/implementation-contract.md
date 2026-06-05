# IMPL-020 — Multi-Material Model, Primitive Helpers & API Cleanup

## Source spec

`.specs/sprint-2026-06/model-multi-material/spec.md` (SPEC-020)

## Goal

Implement the redesigned Model with SubMesh.material_index, unified factory, primitive helpers, full app migration, and multi-material demo.

## Files to create

1. `src/engine/render/primitives.h` — `create_cube()`, `create_triangle()`, `create_quad()` declarations
2. `src/engine/render/primitives.cpp` — implementations
3. `src/cmd/apps/multi_material_app.h` — `MultiMaterialApp` declaration
4. `src/cmd/apps/multi_material_app.cpp` — implementation

## Files to modify

1. `src/engine/render/model.h` — new SubMesh struct, new Model API
2. `src/engine/render/model.cpp` — new factory, draw, removed methods
3. `src/engine/render/render_device.h` — add `fallback_material()` pure virtual
4. `src/engine/render/render_device_opengl.h` — declare fallback override + member
5. `src/engine/render/render_device_opengl.cpp` — implement fallback (magenta shader)
6. `src/engine/render/render_device_headless.h` — declare fallback override + member
7. `src/engine/render/render_device_headless.cpp` — implement fallback
8. `src/engine/render/render_system.cpp` — migrate from `model.material()` to `model.materials()[0]`
9. `src/cmd/main.cpp` — add multi-material scene dispatch + usage text
10. `src/cmd/demo/demo_helpers.h` — remove `CubeResources`, `setup_cube()`, `setup_triangle()`
11. `src/cmd/demo/demo_helpers.cpp` — remove implementations
12. `src/cmd/apps/cube_app.h` — remove `demo::CubeResources` dependency
13. `src/cmd/apps/cube_app.cpp` — migrate to `create_cube()`
14. `src/cmd/apps/triangle_app.h` — migrate to `create_triangle()`
15. `src/cmd/apps/triangle_app.cpp` — migrate
16. `src/cmd/apps/cube_scene_app.h` — migrate
17. `src/cmd/apps/cube_scene_app.cpp` — migrate
18. `src/cmd/apps/phong_app.h` — migrate
19. `src/cmd/apps/phong_app.cpp` — migrate
20. `src/cmd/apps/free_camera_app.h` — migrate
21. `src/cmd/apps/free_camera_app.cpp` — migrate
22. `src/cmd/apps/textured_cube_app.h` — migrate
23. `src/cmd/apps/textured_cube_app.cpp` — migrate
24. `src/cmd/apps/asset_demo_app.h` — migrate
25. `src/cmd/apps/asset_demo_app.cpp` — migrate
26. `src/cmd/apps/hot_reload_app.h` — migrate
27. `src/cmd/apps/hot_reload_app.cpp` — migrate
28. `src/cmd/apps/run_app.h` — migrate
29. `src/cmd/apps/run_app.cpp` — migrate
30. `src/cmd/apps/triangle_app.h` — migrate

## Detailed implementation steps

### Step 1: Update model.h

**Keep** `#include "render/material.h"` — still needed for `std::shared_ptr<Material>`.

**Remove:**
- `material()` non-const and const declarations
- `has_indices()` declaration
- `Model::create()` (non-indexed) declaration
- `material_` private member
- Any existing factory overloads that take a single `shared_ptr<Material>`

**Changes:**

1. Define `SubMesh` struct before `class Model`:
```cpp
struct SubMesh {
    uint32_t index_start;
    uint32_t index_count;
    uint32_t material_index;
};
```

2. Replace `class Model` body:
```cpp
class Model {
public:
    Model() noexcept = default;

    [[nodiscard]] static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    auto draw(RenderDevice& device) const -> void;

    auto submeshes() const noexcept -> const std::vector<SubMesh>&;
    auto materials() const noexcept -> const std::vector<std::shared_ptr<Material>>&;
    auto vertices() const noexcept -> const VertexBuffer&;
    auto indices() const noexcept -> const IndexBuffer&;
    auto vertex_count() const noexcept -> uint32_t;
    auto index_count() const noexcept -> uint32_t;

    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;
    Model(Model&& other) noexcept;
    auto operator=(Model&& other) noexcept -> Model&;
    ~Model() = default;

private:
    Model(
        std::unique_ptr<VertexBuffer> vb,
        std::unique_ptr<IndexBuffer> ib,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;

    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::vector<SubMesh> submeshes_;
    std::vector<std::shared_ptr<Material>> materials_;
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
    uint32_t vertex_count_{0};
    uint32_t index_count_{0};
};
```

3. Remove `static_assert` for non-copyable (still true, no change needed).

4. Add `#include <vector>` if not already present.

### Step 2: Update model.cpp

**Remove existing implementation.** Write new implementation:

**Constructor:**
```cpp
Model::Model(
    std::unique_ptr<VertexBuffer> vb,
    std::unique_ptr<IndexBuffer> ib,
    std::vector<SubMesh> submeshes,
    std::vector<std::shared_ptr<Material>> materials,
    PrimitiveTopology topology,
    uint32_t vertex_count,
    uint32_t index_count
) noexcept
    : vb_(std::move(vb))
    , ib_(std::move(ib))
    , submeshes_(std::move(submeshes))
    , materials_(std::move(materials))
    , topology_(topology)
    , vertex_count_(vertex_count)
    , index_count_(index_count)
{}
```

**Factory:**
```cpp
auto Model::create_indexed(
    RenderDevice& device,
    const VertexFormat& vertex_format,
    std::span<const std::byte> vertex_data,
    std::span<const std::byte> index_data,
    IndexType index_type,
    std::vector<SubMesh> submeshes,
    std::vector<std::shared_ptr<Material>> materials,
    PrimitiveTopology topology
) -> Result<Model> {
    if (vertex_data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex data is empty");
    }
    if (index_data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }
    if (vertex_format.stride() == 0 || vertex_format.attributes().empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format must have a positive stride and at least one attribute");
    }

    auto vb = device.create_vertex_buffer(vertex_format, vertex_data);
    if (!vb) {
        return make_error(vb.error());
    }

    auto ib = device.create_index_buffer(index_data, index_type);
    if (!ib) {
        return make_error(ib.error());
    }

    // Count vertices/indices from spans
    uint32_t vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride());
    auto index_size = (index_type == IndexType::Uint16) ? sizeof(uint16_t) : sizeof(uint32_t);
    uint32_t icount = static_cast<uint32_t>(index_data.size() / index_size);

    return Model(
        std::move(*vb),
        std::move(*ib),
        std::move(submeshes),
        std::move(materials),
        topology,
        vcount,
        icount
    );
}
```

**draw():**
```cpp
auto Model::draw(RenderDevice& device) const -> void {
    if (!vb_ || !ib_ || submeshes_.empty()) {
        return;
    }

    for (const auto& sm : submeshes_) {
        const Material* mat = nullptr;
        if (sm.material_index < materials_.size()) {
            mat = materials_[sm.material_index].get();
        }
        const auto& material = mat ? *mat : device.fallback_material();
        device.draw_indexed(topology_, *vb_, *ib_, material,
            sm.index_count, sm.index_start);
    }
}
```

**Move constructor:**
```cpp
Model::Model(Model&& other) noexcept
    : vb_(std::move(other.vb_))
    , ib_(std::move(other.ib_))
    , submeshes_(std::move(other.submeshes_))
    , materials_(std::move(other.materials_))
    , topology_(other.topology_)
    , vertex_count_(other.vertex_count_)
    , index_count_(other.index_count_)
{
    other.topology_ = PrimitiveTopology::Triangles;
    other.vertex_count_ = 0;
    other.index_count_ = 0;
}
```

**Move assignment:**
```cpp
auto Model::operator=(Model&& other) noexcept -> Model& {
    if (this != &other) {
        vb_ = std::move(other.vb_);
        ib_ = std::move(other.ib_);
        submeshes_ = std::move(other.submeshes_);
        materials_ = std::move(other.materials_);
        topology_ = other.topology_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        other.topology_ = PrimitiveTopology::Triangles;
        other.vertex_count_ = 0;
        other.index_count_ = 0;
    }
    return *this;
}
```

**Accessors:**
```cpp
auto Model::submeshes() const noexcept -> const std::vector<SubMesh>& {
    return submeshes_;
}

auto Model::materials() const noexcept -> const std::vector<std::shared_ptr<Material>>& {
    return materials_;
}

auto Model::vertices() const noexcept -> const VertexBuffer& {
    return *vb_;
}

auto Model::indices() const noexcept -> const IndexBuffer& {
    return *ib_;
}

auto Model::vertex_count() const noexcept -> uint32_t {
    return vertex_count_;
}

auto Model::index_count() const noexcept -> uint32_t {
    return index_count_;
}
```

### Step 3: Create primitives.h

```cpp
#pragma once

#include "render/material.h"
#include "render/model.h"

#include <memory>

namespace buddd::engine {

/// Creates a unit cube (2×2×2, centred at origin).
/// 24 vertices (Float3 position + Float3 colour), 36 indices (Uint16).
/// One submesh. The provided material is stored at materials()[0].
[[nodiscard]] auto create_cube(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a coloured right triangle.
/// 3 vertices (Float3 position + Float3 colour), 3 indices (Uint16).
/// One submesh.
[[nodiscard]] auto create_triangle(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a unit quad (2 triangles) in the XY plane.
/// 4 vertices (Float3 position + Float3 colour), 6 indices (Uint16).
/// One submesh.
[[nodiscard]] auto create_quad(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

} // namespace buddd::engine
```

### Step 4: Create primitives.cpp

Each helper is a **geometry factory** — it creates vertex/index data for a primitive shape and wraps it in a Model:

1. Defines vertex data (Float3 position + Float3 colour, stride 24 bytes)
2. Defines index data (Uint16)
3. Calls `Model::create_indexed()` with one SubMesh covering all indices and the caller-provided material in the materials vector
4. Returns the Model (or propagates buffer creation errors)

**The helpers do NOT create materials.** The caller must create a material and pass it in. This gives the caller full control over shaders, uniforms, and material type. Without a material the model cannot render, so it is a required parameter.

**create_cube()** — Use the same 24-vertex, 36-index cube data from SPEC-009's `setup_cube()`.

**create_triangle()** — Use the same 3-vertex, 3-index triangle data from SPEC-005's `setup_triangle()`.

**create_quad()** — Define a unit quad:
- 4 vertices: (-0.5,-0.5,0), (0.5,-0.5,0), (0.5,0.5,0), (-0.5,0.5,0) with distinct colours.
- 6 indices: {0,1,2, 0,2,3} (Uint16).

Implementation example for create_cube:
```cpp
auto create_cube(RenderDevice& device, std::shared_ptr<Material> material)
    -> Result<Model>
{
    constexpr float verts[] = { /* 24 vertices × 6 floats = 144 floats */ };
    constexpr uint16_t idxs[] = { /* 36 indices */ };

    auto vertex_bytes = std::as_bytes(std::span{verts});
    auto index_bytes = std::as_bytes(std::span{idxs});

    return Model::create_indexed(
        device,
        k_cube_format,
        vertex_bytes,
        index_bytes,
        IndexType::Uint16,
        { SubMesh{0, 36, 0} },  // one submesh covering all indices, material_index=0
        { std::move(material) } // one material
    );
}
```

### Step 5: Update render_device.h

Add:
```cpp
virtual auto fallback_material() noexcept -> Material& = 0;
```

### Step 6: Update OpenGL backend

In `render_device_opengl.h`:
```cpp
class RenderDeviceOpenGL : public RenderDevice {
    // ...
    auto fallback_material() noexcept -> Material& override;
    // ...
private:
    std::unique_ptr<Material> fallback_material_;
};
```

In `render_device_opengl.cpp`, implement `fallback_material()`:
- Lazily create a material with minimal shaders on first call:
  - Vertex shader: pass-through `a_position` (location 0), output `gl_Position`
  - Fragment shader: output `vec4(1.0, 0.0, 1.0, 1.0)` (magenta)
  - No uniforms.
- Cache in `fallback_material_`.
- Return reference.

### Step 7: Update Headless backend

Same pattern as OpenGL but with `MaterialHeadless`.

### Step 8: Update render_system.cpp

Change all `mr.model().material()` calls to `mr.model().materials()[0]`.

### Step 9: Remove demo_helpers

Delete all contents of `demo_helpers.h` and `demo_helpers.cpp` except maybe keep the file with just a placeholder or remove entirely. 

In `demo_helpers.h`:
- Remove `#include "render/material.h"`, `#include "render/model.h"`, `#include "render/vertex_buffer.h"`
- Remove `namespace buddd::cmd::demo` block
- Remove `setup_triangle()` declaration
- Remove `CubeResources` struct
- Remove `setup_cube()` declaration

The file can be deleted entirely if nothing else references it. Check all includes.

In `demo_helpers.cpp`: Remove entire file content.

### Step 10: Migrate cube_app

The app now creates its own material and passes it to `create_cube()`.

```cpp
// cube_app.h changes:
// Remove #include "demo/demo_helpers.h"
// Change std::unique_ptr<demo::CubeResources> cube_ → Model model_;
// Add: std::shared_ptr<engine::Material> material_;

// cube_app.cpp changes:
// setup():
//   auto vs = device.create_shader(Vertex, vertex_src);
//   if (!vs) return make_error(vs.error());
//   auto fs = device.create_shader(Fragment, fragment_src);
//   if (!fs) return make_error(fs.error());
//   auto mat = device.create_material(std::move(*vs), std::move(*fs), {"u_mvp"});
//   if (!mat) return make_error(mat.error());
//   material_ = std::shared_ptr<engine::Material>(std::move(*mat));
//   
//   auto cube_result = engine::create_cube(device, material_);
//   if (!cube_result) return make_error(cube_result.error());
//   model_ = std::move(*cube_result);
//   
// render():
//   auto result = material_->set_uniform("u_mvp", mvp);
//   model_.draw(device);
```

Where `vertex_src` and `fragment_src` are the same pass-through/per-vertex-colour shaders that `demo::setup_cube()` used internally.

### Step 11: Migrate triangle_app

Similar to cube_app but using `engine::create_triangle()` with its own material setup.

### Step 12: Migrate cube_scene_app

Uses `demo::setup_cube()` → replace with `engine::create_cube(device, material)`. The app must create its own material (with pass-through vertex + per-vertex colour fragment shader) and store it as a `shared_ptr<engine::Material>` member, same pattern as Step 10.

### Step 13: Migrate phong_app

`create_phong_cube()` currently creates a cube manually with PhongMaterial. Options:
- Keep manual cube creation but use `Model::create_indexed()` with the new API.
- Or use `engine::create_cube()` and replace the material afterwards.

Since Model is immutable after creation, `create_phong_cube()` should create the VertexBuffer/IndexBuffer manually (like it does now) then use `Model::create_indexed()`. Or it could use `create_cube()` and create a different Model with PhongMaterial substituted.

Simplest: Keep manual cube creation but adapt to new API. The `create_phong_cube()` function creates its own PhongMaterial and already has vertex/index data. Just call `Model::create_indexed()` instead of the old factory.

### Step 14: Migrate remaining apps

For each app:
1. Create and store a `shared_ptr<engine::Material>` with appropriate shaders (e.g., pass-through vertex + solid/per-vertex colour fragment).
2. Replace `demo::setup_cube()` with `engine::create_cube(device, material)`.
3. Replace `demo::setup_triangle()` with `engine::create_triangle(device, material)`.
4. Replace `model.material()` with `model.materials()[0]`.
5. Replace `cube_->material->set_uniform(...)` with `material_->set_uniform(...)` (using the stored material member directly, not looking it up from the model each frame).

**Important:** Since `create_cube()` takes ownership of the material (stores in `shared_ptr`), the app can keep its own `shared_ptr` copy for setting uniforms, or look it up from `model.materials()[0]` each frame. Keeping a separate `shared_ptr` member is simpler and avoids the lookup.

### Step 15: Create multi_material_app

```cpp
// multi_material_app.h
#pragma once
#include "app.h"
#include "math/camera.h"
#include "render/model.h"
#include <chrono>
#include <memory>
#include <vector>

namespace buddd::engine { class RenderDevice; }
namespace buddd::cmd::app {

class MultiMaterialApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — multi-material", 1024, 768};
    }
    [[nodiscard]] auto setup(engine::RenderDevice& device) -> engine::Result<void> override;
    auto render(engine::RenderDevice& device, int frame) -> void override;
private:
    engine::Model model_;
    engine::math::Camera camera_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
```

```cpp
// multi_material_app.cpp
#include "apps/multi_material_app.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "render/render_device.h"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace be = buddd::engine;

namespace {

struct ColoredVertex {
    float px, py, pz;
    float cx, cy, cz;
};

constexpr ColoredVertex k_vertices[24] = {
    // +X (red)
    { 1,-1,-1, 1,0,0 }, { 1, 1,-1, 1,0,0 }, { 1, 1, 1, 1,0,0 }, { 1,-1, 1, 1,0,0 },
    // -X (green)
    {-1,-1, 1, 0,1,0 }, {-1, 1, 1, 0,1,0 }, {-1, 1,-1, 0,1,0 }, {-1,-1,-1, 0,1,0 },
    // +Y (blue)
    {-1, 1, 1, 0,0,1 }, { 1, 1, 1, 0,0,1 }, { 1, 1,-1, 0,0,1 }, {-1, 1,-1, 0,0,1 },
    // -Y (yellow)
    {-1,-1,-1, 1,1,0 }, { 1,-1,-1, 1,1,0 }, { 1,-1, 1, 1,1,0 }, {-1,-1, 1, 1,1,0 },
    // +Z (cyan)
    { 1,-1, 1, 0,1,1 }, { 1, 1, 1, 0,1,1 }, {-1, 1, 1, 0,1,1 }, {-1,-1, 1, 0,1,1 },
    // -Z (magenta)
    {-1,-1,-1, 1,0,1 }, {-1, 1,-1, 1,0,1 }, { 1, 1,-1, 1,0,1 }, { 1,-1,-1, 1,0,1 },
};

constexpr uint16_t k_indices[36] = {
     0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,  12,13,14, 12,14,15,
    16,17,18, 16,18,19,  20,21,22, 20,22,23,
};

// Vertex format: Float3 position + Float3 colour
constexpr auto k_format = be::VertexFormat::create(24)
    .add(be::AttributeType::Float3)  // a_position
    .add(be::AttributeType::Float3)  // a_colour
    .build();

} // anonymous namespace

auto buddd::cmd::app::MultiMaterialApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // Create 3 materials (red, green, blue)
    auto red_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!red_vs) return make_error(red_vs.error());

    auto red_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(1.0, 0.0, 0.0, 1.0); }
    )");
    if (!red_fs) return make_error(red_fs.error());

    auto red_mat = device.create_material(std::move(*red_vs), std::move(*red_fs), {"u_mvp"});

    auto green_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!green_vs) return make_error(green_vs.error());

    auto green_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(0.0, 1.0, 0.0, 1.0); }
    )");
    if (!green_fs) return make_error(green_fs.error());

    auto green_mat = device.create_material(std::move(*green_vs), std::move(*green_fs), {"u_mvp"});

    auto blue_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!blue_vs) return make_error(blue_vs.error());

    auto blue_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(0.0, 0.0, 1.0, 1.0); }
    )");
    if (!blue_fs) return make_error(blue_fs.error());

    auto blue_mat = device.create_material(std::move(*blue_vs), std::move(*blue_fs), {"u_mvp"});

    // Shared vertex/index data as bytes
    auto vertex_bytes = std::as_bytes(std::span{k_vertices});
    auto index_bytes = std::as_bytes(std::span{k_indices});

    // Convert unique_ptr<Material> to shared_ptr<Material>
    auto red_shared = std::shared_ptr<be::Material>(std::move(*red_mat));
    auto green_shared = std::shared_ptr<be::Material>(std::move(*green_mat));
    auto blue_shared = std::shared_ptr<be::Material>(std::move(*blue_mat));

    // 3 submeshes: 12 indices each (2 faces per submesh)
    auto model_result = be::Model::create_indexed(
        device, k_format, vertex_bytes, index_bytes, be::IndexType::Uint16,
        {
            {0, 12, 0},   // first 2 faces → red
            {12, 12, 1},  // next 2 faces → green
            {24, 12, 2},  // last 2 faces → blue
        },
        {red_shared, green_shared, blue_shared}
    );
    if (!model_result) return make_error(model_result.error());
    model_ = std::move(*model_result);

    camera_.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera_.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f, 100.0f
    );

    start_time_ = std::chrono::steady_clock::now();
    return {};
}

auto buddd::cmd::app::MultiMaterialApp::render(be::RenderDevice& device, int) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    be::math::Mat4 mvp = camera_.projection_matrix()
        * camera_.view_matrix()
        * be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());

    // Set u_mvp on all materials
    for (auto& mat : model_.materials()) {
        if (mat) {
            auto r = mat->set_uniform("u_mvp", mvp);
            if (!r) {
                std::cerr << "u_mvp set failed: " << be::to_string(r.error()) << "\n";
            }
        }
    }

    model_.draw(device);
}
```

### Step 16: Update main.cpp

Add `#include "apps/multi_material_app.h"` near the other app includes.

Add `else if (scene == "multi-material") app = std::make_unique<bc::app::MultiMaterialApp>();` in the scene dispatch chain.

Add usage text line:
```
"  multi-material  Multi-material cube: 3 submeshes with red, green, blue materials (120 frames)\n"
```

### Step 17: Update existing Model tests

Existing unit tests for `Model` (in `tests/` or similar) that use the old API (`Model::create()`, `model.material()`, etc.) must be updated to use the new API:
- Replace `Model::create(device, format, data, material)` with `Model::create_indexed(device, format, vertex_data, index_data, type, {SubMesh{0, N, 0}}, {material})`.
- Replace `model.material()` with `model.materials()[0]`.
- Remove tests for `has_indices()`, `material()` edge cases that no longer apply.

File locations depend on the project's test directory structure — search for existing Model tests.

### Step 18: Build changes

No explicit CMake changes needed — `GLOB_RECURSE` picks up new `.cpp` files in `apps/` and `src/engine/render/*.cpp` picks up `primitives.cpp`.

## Test plan

| AC | Test approach |
|---|---|
| AC-001 | Compile check: SubMesh struct exists with 3 uint32_t fields. |
| AC-002 | Headless: create model with 2 submeshes, 2 materials. Verify submeshes and materials match. |
| AC-003 | Headless: empty vertex data → error. |
| AC-004 | Headless: empty index data → error. |
| AC-005 | Headless: 3 submeshes → draw_call_count +3. |
| AC-006 | Headless: verify materials used via material tracking. |
| AC-007 | Headless: null material in vector → fallback used. |
| AC-008 | Headless: out-of-bounds material_index → fallback used. |
| AC-009 | Headless: empty submeshes → no draw calls. |
| AC-010 | Headless: moved-from model → no crash on draw. |
| AC-011 | Headless: create_cube → 1 submesh, 1 material, 36 indices. |
| AC-012 | Headless: create_triangle → 1 submesh, 1 material, 3 indices. |
| AC-013 | Headless: create_quad → 1 submesh, 1 material, 6 indices. |
| AC-014 | Headless: fallback_material() returns valid material. |
| AC-015 | File inspection: demo_helpers cleaned. |
| AC-016 | File inspection: CubeResources removed. |
| AC-017 | Compile check: no `material()` in Model. |
| AC-018 | Compile check: no `create()` in Model. |
| AC-019 | Compile check: no `has_indices()` in Model. |
| AC-020–022 | Run demos, verify exit code 0. |
| AC-023 | Move test: moved model has original submeshes/materials, source is empty. |
| AC-024 | Const-correctness: `const auto& sm = model.submeshes()` compiles. |

## Build and test commands

```bash
cmake --build --preset debug
ctest --preset debug
```

All existing and new tests must pass.

## Edge case coverage

From spec edge case table:
- Empty submeshes → no-op in draw() (implemented)
- Empty materials → all submeshes use fallback (implemented — material_index >= size() path)
- Null material → fallback (implemented)
- Out-of-bounds material_index → fallback (implemented)
- Shared material_index → same material used for multiple draw calls (natural, no special code)
- Moved-from → null vb_/ib_ → no-op (implemented)
- Buffer creation failure → error returned (implemented)
- create_cube called twice → independent Models (natural)

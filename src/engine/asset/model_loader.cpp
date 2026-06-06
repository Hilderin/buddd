#define GLM_ENABLE_EXPERIMENTAL  // Must be before any GLM includes
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NOEXCEPTION

#include "asset/model_loader.h"
#include "stb_image.h"
#include <tiny_gltf.h>
#include "render/render_device.h"
#include "render/pbr/pbr_material.h"
#include "render/vertex.h"
#include "render/texture.h"
#include "image/image.h"
#include "image/image_buffer.h"
#include "log/log.h"
#include "math/quat.h"

BUDDD_LOG_TAG("Asset:ModelLoader");

#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace be = buddd::engine;

namespace buddd::engine::detail {

namespace {

// ============================================================================
// Magenta fallback texture (1x1 RGBA8, 255,0,255,255)
// ============================================================================
auto get_magenta_fallback_texture(RenderDevice& device) -> std::shared_ptr<Texture> {
    static std::shared_ptr<Texture> fallback;
    static bool created = false;
    if (!created) {
        std::vector<std::byte> pixel_data = {
            std::byte{255}, std::byte{0}, std::byte{255}, std::byte{255}
        };
        auto buffer = ImageBuffer{1, 1, 4, std::move(pixel_data)};
        auto image = Image::create(buffer);
        if (image) {
            auto tex = device.create_texture(*image);
            if (tex) {
                fallback = std::shared_ptr<Texture>(std::move(*tex));
            }
        }
        created = true;
    }
    return fallback;
}

// ============================================================================
// Helper: read a float accessor value
// ============================================================================
auto read_accessor_float(const tinygltf::Model& model, int accessor_idx, int component)
    -> std::optional<float>
{
    if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size()))
        return std::nullopt;
    const auto& accessor = model.accessors[accessor_idx];
    if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
        return std::nullopt;
    const auto& bv = model.bufferViews[accessor.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size()))
        return std::nullopt;
    const auto& buffer = model.buffers[bv.buffer];

    const size_t byte_offset = static_cast<size_t>(bv.byteOffset + accessor.byteOffset + component * sizeof(float));
    if (byte_offset + sizeof(float) > buffer.data.size())
        return std::nullopt;
    float val;
    std::memcpy(&val, &buffer.data[byte_offset], sizeof(float));
    return val;
}

// ============================================================================
// Helper: extract vertex data from glTF accessor to a Vertex array
// ============================================================================
struct AccessorInfo {
    int accessor_idx = -1;
    int type = 0;       // TINYGLTF_TYPE_VEC2, VEC3, VEC4
    int component_type = 5126; // FLOAT
};

auto read_vertex_data(
    const tinygltf::Model& model,
    int mesh_index,
    int primitive_index,
    const tinygltf::Primitive& prim,
    float scale)
    -> Result<std::vector<Vertex>>
{
    // POSITION is required
    auto pos_it = prim.attributes.find("POSITION");
    if (pos_it == prim.attributes.end()) {
        return make_error(Error::Category::InvalidArgument,
            "Mesh " + std::to_string(mesh_index) + " primitive " +
            std::to_string(primitive_index) + " has no POSITION attribute");
    }

    int pos_accessor_idx = pos_it->second;
    if (pos_accessor_idx < 0 || pos_accessor_idx >= static_cast<int>(model.accessors.size())) {
        return make_error(Error::Category::InvalidFormat,
            "Invalid POSITION accessor index");
    }

    const auto& pos_accessor = model.accessors[pos_accessor_idx];
    size_t vertex_count = static_cast<size_t>(pos_accessor.count);

    // Helper lambda to read attribute data into a float buffer
    auto read_attribute = [&](const std::string& semantic, int expected_components,
                               std::vector<float>& output, float default_val) -> bool {
        output.clear();
        auto it = prim.attributes.find(semantic);
        if (it == prim.attributes.end()) {
            return false;
        }
        int accessor_idx = it->second;
        if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size())) {
            return false;
        }
        const auto& accessor = model.accessors[accessor_idx];
        if (accessor.bufferView < 0) return false;
        const auto& bv = model.bufferViews[accessor.bufferView];
        if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) return false;
        const auto& buffer = model.buffers[bv.buffer];

        int num_components = 1;
        if (accessor.type == TINYGLTF_TYPE_VEC2) num_components = 2;
        else if (accessor.type == TINYGLTF_TYPE_VEC3) num_components = 3;
        else if (accessor.type == TINYGLTF_TYPE_VEC4) num_components = 4;
        else if (accessor.type == TINYGLTF_TYPE_SCALAR) num_components = 1;

        size_t count = static_cast<size_t>(accessor.count);
        output.resize(count * static_cast<size_t>(expected_components));

        size_t stride = bv.byteStride > 0
            ? static_cast<size_t>(bv.byteStride)
            : static_cast<size_t>(num_components) * sizeof(float);
        size_t offset = static_cast<size_t>(bv.byteOffset + accessor.byteOffset);

        for (size_t i = 0; i < count; ++i) {
            for (int c = 0; c < num_components && c < expected_components; ++c) {
                size_t byte_offset = offset + i * stride + static_cast<size_t>(c) * sizeof(float);
                if (byte_offset + sizeof(float) <= buffer.data.size()) {
                    float val;
                    std::memcpy(&val, &buffer.data[byte_offset], sizeof(float));
                    output[i * static_cast<size_t>(expected_components) + static_cast<size_t>(c)] = val;
                } else {
                    output[i * static_cast<size_t>(expected_components) + static_cast<size_t>(c)] = default_val;
                }
            }
            for (int c = num_components; c < expected_components; ++c) {
                output[i * static_cast<size_t>(expected_components) + static_cast<size_t>(c)] = default_val;
            }
        }

        // Handle COLOR_0 VEC3 -> VEC4 expansion
        if (semantic == "COLOR_0" && num_components == 3) {
            // Already handled via expected_components=4 and default_val=1.0 for alpha
        }

        return true;
    };

    // Read positions
    std::vector<float> positions(vertex_count * 3);
    {
        const auto& accessor = model.accessors[pos_accessor_idx];
        if (accessor.bufferView < 0) {
            return make_error(Error::Category::InvalidFormat,
                "POSITION accessor has no bufferView");
        }
        const auto& bv = model.bufferViews[accessor.bufferView];
        if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
            return make_error(Error::Category::InvalidFormat,
                "Invalid buffer in POSITION accessor");
        }
        const auto& buffer = model.buffers[bv.buffer];
        size_t stride = bv.byteStride > 0
            ? static_cast<size_t>(bv.byteStride)
            : 3 * sizeof(float);
        size_t offset = static_cast<size_t>(bv.byteOffset + accessor.byteOffset);

        for (size_t i = 0; i < vertex_count; ++i) {
            for (int c = 0; c < 3; ++c) {
                size_t byte_offset = offset + i * stride + static_cast<size_t>(c) * sizeof(float);
                if (byte_offset + sizeof(float) <= buffer.data.size()) {
                    float val;
                    std::memcpy(&val, &buffer.data[byte_offset], sizeof(float));
                    positions[i * 3 + static_cast<size_t>(c)] = val * scale;
                } else {
                    positions[i * 3 + static_cast<size_t>(c)] = 0.0f;
                }
            }
        }
    }

    // Read optional attributes
    std::vector<float> normals;
    bool has_normals = read_attribute("NORMAL", 3, normals, 0.0f);
    if (!has_normals) {
        normals.resize(vertex_count * 3);
        std::fill(normals.begin(), normals.end(), 0.0f);
        // Default normal is (0,0,1) if missing
        for (size_t i = 0; i < vertex_count; ++i) {
            normals[i * 3 + 2] = 1.0f;
        }
    }

    std::vector<float> texcoords;
    bool has_texcoords = read_attribute("TEXCOORD_0", 2, texcoords, 0.0f);
    if (!has_texcoords) {
        texcoords.resize(vertex_count * 2, 0.0f);
    }

    std::vector<float> colors;
    bool has_colors = read_attribute("COLOR_0", 4, colors, 1.0f);
    if (!has_colors) {
        colors.resize(vertex_count * 4, 1.0f);
    }

    std::vector<float> tangents;
    bool has_tangents = read_attribute("TANGENT", 4, tangents, 0.0f);
    if (!has_tangents) {
        tangents.resize(vertex_count * 4, 0.0f);
    }

    std::vector<float> texcoords2;
    bool has_texcoords2 = read_attribute("TEXCOORD_1", 2, texcoords2, 0.0f);
    if (!has_texcoords2) {
        texcoords2.resize(vertex_count * 2, 0.0f);
    }

    // Fill vertex array
    std::vector<Vertex> vertices(vertex_count);
    for (size_t i = 0; i < vertex_count; ++i) {
        vertices[i].position = math::Vec3{
            positions[i * 3],
            positions[i * 3 + 1],
            positions[i * 3 + 2]
        };
        vertices[i].color = math::Vec4{
            colors[i * 4],
            colors[i * 4 + 1],
            colors[i * 4 + 2],
            colors[i * 4 + 3]
        };
        vertices[i].normal = math::Vec3{
            normals[i * 3],
            normals[i * 3 + 1],
            normals[i * 3 + 2]
        };
        vertices[i].texcoord = math::Vec2{
            texcoords[i * 2],
            texcoords[i * 2 + 1]
        };
        vertices[i].tangent = math::Vec4{
            tangents[i * 4],
            tangents[i * 4 + 1],
            tangents[i * 4 + 2],
            tangents[i * 4 + 3]
        };
        vertices[i].texcoord2 = math::Vec2{
            texcoords2[i * 2],
            texcoords2[i * 2 + 1]
        };
    }

    return vertices;
}

// ============================================================================
// Helper: read index data from glTF accessor
// ============================================================================
struct IndexResult {
    std::vector<std::byte> data;
    IndexType type;
    size_t count;
};

auto read_index_data(const tinygltf::Model& model, const tinygltf::Primitive& prim)
    -> Result<IndexResult>
{
    if (prim.indices < 0) {
        return make_error(Error::Category::InvalidFormat,
            "Primitive has no indices");
    }

    const auto& accessor = model.accessors[prim.indices];
    if (accessor.bufferView < 0) {
        return make_error(Error::Category::InvalidFormat,
            "Index accessor has no bufferView");
    }

    const auto& bv = model.bufferViews[accessor.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
        return make_error(Error::Category::InvalidFormat,
            "Invalid buffer in index accessor");
    }

    const auto& buffer = model.buffers[bv.buffer];

    IndexType index_type;
    size_t component_size;

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: // 5123
            index_type = IndexType::Uint16;
            component_size = 2;
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: // 5125
            index_type = IndexType::Uint32;
            component_size = 4;
            break;
        default:
            return make_error(Error::Category::Unsupported,
                "Unsupported index component type: " + std::to_string(accessor.componentType));
    }

    size_t count = static_cast<size_t>(accessor.count);
    size_t byte_length = count * component_size;
    size_t stride = bv.byteStride > 0 ? static_cast<size_t>(bv.byteStride) : component_size;
    size_t offset = static_cast<size_t>(bv.byteOffset + accessor.byteOffset);

    if (offset + (count - 1) * stride + component_size > buffer.data.size()) {
        return make_error(Error::Category::InvalidFormat,
            "Index data out of bounds");
    }

    std::vector<std::byte> index_data(byte_length);
    if (stride == component_size) {
        // Contiguous — copy in one shot
        std::memcpy(index_data.data(), &buffer.data[offset], byte_length);
    } else {
        // Non-contiguous — copy per index
        for (size_t i = 0; i < count; ++i) {
            std::memcpy(&index_data[i * component_size],
                        &buffer.data[offset + i * stride],
                        component_size);
        }
    }

    return IndexResult{std::move(index_data), index_type, count};
}

// ============================================================================
// Helper: load a texture from glTF texture info
// ============================================================================
auto load_gltf_texture(RenderDevice& device,
                       const tinygltf::Model& model,
                       int texture_idx,
                       const std::string& base_dir)
    -> std::shared_ptr<Texture>
{
    if (texture_idx < 0 || texture_idx >= static_cast<int>(model.textures.size())) {
        return nullptr;
    }

    const auto& gltf_tex = model.textures[texture_idx];
    if (gltf_tex.source < 0 || gltf_tex.source >= static_cast<int>(model.images.size())) {
        return nullptr;
    }

    const auto& image = model.images[gltf_tex.source];

    // Check for texCoord > 0 — skip with warning (V1 only supports TEXCOORD_0)
    // (texCoord is stored in the textureInfo, not in the texture itself,
    //  so we check it at the call site)

    // Try embedded (buffer view) first
    if (image.bufferView >= 0 && image.bufferView < static_cast<int>(model.bufferViews.size())) {
        const auto& bv = model.bufferViews[image.bufferView];
        if (bv.buffer >= 0 && bv.buffer < static_cast<int>(model.buffers.size())) {
            const auto& buffer = model.buffers[bv.buffer];
            size_t offset = static_cast<size_t>(bv.byteOffset);
            size_t size = static_cast<size_t>(bv.byteLength);

            if (offset + size <= buffer.data.size()) {
                // Use stb_image to decode from raw data
                int w, h, comp;
                unsigned char* pixels = stbi_load_from_memory(
                    reinterpret_cast<const unsigned char*>(&buffer.data[offset]),
                    static_cast<int>(size), &w, &h, &comp, 4);
                if (pixels) {
                    std::vector<std::byte> pixel_data(static_cast<size_t>(w * h * 4));
                    std::memcpy(pixel_data.data(), pixels, pixel_data.size());
                    stbi_image_free(pixels);

                    ImageBuffer img_buf{w, h, 4, std::move(pixel_data)};
                    auto img = Image::create(img_buf);
                    if (img) {
                        auto tex = device.create_texture(*img);
                        if (tex) {
                            return std::shared_ptr<Texture>(std::move(*tex));
                        }
                    }
                }
            }
        }
    }

    // Try external URI
    if (!image.uri.empty()) {
        std::string resolved_path;
        if (image.uri.find("://") != std::string::npos) {
            // Data URI — skip (tinygltf already decoded if possible)
            BUDDD_LOG_WARN("data URI in glTF texture not supported");
            return nullptr;
        }

        // Resolve relative to glTF file directory
        auto fs_path = std::filesystem::path(base_dir) / image.uri;
        resolved_path = fs_path.string();

        auto img = Image::load(resolved_path);
        if (img) {
            auto tex = device.create_texture(*img);
            if (tex) {
                return std::shared_ptr<Texture>(std::move(*tex));
            }
        }

        BUDDD_LOG_WARN("texture load failed for {} \u2014 using magenta fallback", image.uri);
    }

    return nullptr;
}

// ============================================================================
// Helper: create PbrMaterial from glTF material
// ============================================================================
auto create_pbr_material(RenderDevice& device,
                         const tinygltf::Model& model,
                         int material_idx,
                         const std::string& base_dir)
    -> std::shared_ptr<PbrMaterial>
{
    auto pbr_mat = std::make_shared<PbrMaterial>(device);
    PbrMaterialData data;

    if (material_idx >= 0 && material_idx < static_cast<int>(model.materials.size())) {
        const auto& gltf_mat = model.materials[material_idx];

        // Read pbrMetallicRoughness
        const auto& pbr = gltf_mat.pbrMetallicRoughness;
        if (pbr.baseColorFactor.size() >= 4) {
            data.base_color_factor = math::Vec4{
                static_cast<float>(pbr.baseColorFactor[0]),
                static_cast<float>(pbr.baseColorFactor[1]),
                static_cast<float>(pbr.baseColorFactor[2]),
                static_cast<float>(pbr.baseColorFactor[3])
            };
        }
        data.metallic_factor = static_cast<float>(pbr.metallicFactor);
        data.roughness_factor = static_cast<float>(pbr.roughnessFactor);

        if (gltf_mat.emissiveFactor.size() >= 3) {
            data.emissive_factor = math::Vec3{
                static_cast<float>(gltf_mat.emissiveFactor[0]),
                static_cast<float>(gltf_mat.emissiveFactor[1]),
                static_cast<float>(gltf_mat.emissiveFactor[2])
            };
        }

        data.double_sided = gltf_mat.doubleSided;

        // Load textures
        auto load_tex = [&](int tex_idx) -> std::shared_ptr<Texture> {
            if (tex_idx < 0) return nullptr;
            return load_gltf_texture(device, model, tex_idx, base_dir);
        };

        // Replace any null textures with magenta fallback
        auto ensure_texture = [&](std::shared_ptr<Texture>& tex) {
            if (!tex) {
                tex = get_magenta_fallback_texture(device);
            }
        };

        // Base color texture
        if (pbr.baseColorTexture.index >= 0) {
            if (pbr.baseColorTexture.texCoord > 0) {
                BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1");
            }
            data.base_color_texture = load_tex(pbr.baseColorTexture.index);
            ensure_texture(data.base_color_texture);
        }

        // Metallic-roughness texture
        if (pbr.metallicRoughnessTexture.index >= 0) {
            if (pbr.metallicRoughnessTexture.texCoord > 0) {
                BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1");
            }
            data.metallic_roughness_texture = load_tex(pbr.metallicRoughnessTexture.index);
            ensure_texture(data.metallic_roughness_texture);
        }

        // Normal texture
        if (gltf_mat.normalTexture.index >= 0) {
            if (gltf_mat.normalTexture.texCoord > 0) {
                BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1");
            }
            data.normal_texture = load_tex(gltf_mat.normalTexture.index);
            ensure_texture(data.normal_texture);
        }

        // Occlusion texture
        if (gltf_mat.occlusionTexture.index >= 0) {
            if (gltf_mat.occlusionTexture.texCoord > 0) {
                BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1");
            }
            data.occlusion_texture = load_tex(gltf_mat.occlusionTexture.index);
            ensure_texture(data.occlusion_texture);
        }

        // Emissive texture
        if (gltf_mat.emissiveTexture.index >= 0) {
            if (gltf_mat.emissiveTexture.texCoord > 0) {
                BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1");
            }
            data.emissive_texture = load_tex(gltf_mat.emissiveTexture.index);
            ensure_texture(data.emissive_texture);
        }

        // Check for unsupported extensions
        for (const auto& ext : gltf_mat.extensions) {
            if (ext.first == "KHR_materials_pbrSpecularGlossiness") {
                BUDDD_LOG_WARN("material ext not supported: {} (KHR_materials_pbrSpecularGlossiness) \u2014 using default PBR factors", gltf_mat.name);
                // Reset to default since we can't convert
                data.base_color_factor = math::Vec4{1.0f, 1.0f, 1.0f, 1.0f};
                data.metallic_factor = 0.0f;
                data.roughness_factor = 1.0f;
            }
        }

        // Check alpha mode
        if (!gltf_mat.alphaMode.empty() && gltf_mat.alphaMode != "OPAQUE") {
            BUDDD_LOG_WARN("alphaMode '{}' not supported in V1 \u2014 treating as opaque", gltf_mat.alphaMode);
        }
    }

    pbr_mat->set_data(data);

    if (material_idx >= 0 && material_idx < static_cast<int>(model.materials.size())) {
        BUDDD_LOG_DEBUG("PbrMaterial created: {}", model.materials[material_idx].name);
    } else {
        BUDDD_LOG_DEBUG("PbrMaterial created: (default material, no glTF material)");
    }

    return pbr_mat;
}

// ============================================================================
// Helper: build a Model from a glTF mesh
// ============================================================================
auto build_model_from_mesh(RenderDevice& device,
                           const tinygltf::Model& model,
                           int mesh_idx,
                           const std::string& base_dir,
                           float scale,
                           std::vector<std::shared_ptr<Material>>& out_materials)
    -> Result<std::optional<Model>>
{
    if (mesh_idx < 0 || mesh_idx >= static_cast<int>(model.meshes.size())) {
        return std::optional<Model>{};
    }

    const auto& mesh = model.meshes[mesh_idx];

    if (mesh.primitives.empty()) {
        return std::optional<Model>{};
    }

    // Process all primitives
    std::vector<Vertex> all_vertices;
    std::vector<std::byte> all_index_data;
    IndexType index_type = IndexType::Uint16;
    size_t total_indices = 0;
    std::vector<SubMesh> submeshes;
    std::vector<std::shared_ptr<Material>> materials;
    bool has_uint32 = false;

    // Map from glTF material index to engine material index
    std::unordered_map<int, int> material_map;

    for (int p = 0; p < static_cast<int>(mesh.primitives.size()); ++p) {
        const auto& prim = mesh.primitives[p];

        // Check primitive mode
        if (prim.mode != 4) { // TRIANGLES
            BUDDD_LOG_WARN("unsupported primitive mode {} in mesh {} \u2014 skipping", prim.mode, mesh.name);
            continue;
        }

        // Read vertices
        auto vertex_result = read_vertex_data(model, mesh_idx, p, prim, scale);
        if (!vertex_result) {
            return std::unexpected(vertex_result.error());
        }
        auto& prim_vertices = *vertex_result;
        size_t vertex_offset = all_vertices.size();

        // Append vertices
        all_vertices.insert(all_vertices.end(),
                            std::make_move_iterator(prim_vertices.begin()),
                            std::make_move_iterator(prim_vertices.end()));

        // Read indices
        auto index_result = read_index_data(model, prim);
        if (!index_result) {
            return std::unexpected(index_result.error());
        }

        size_t index_count = index_result->count;

        // Adjust indices by vertex offset and append
        if (index_result->type == IndexType::Uint16) {
            auto* src = reinterpret_cast<const uint16_t*>(index_result->data.data());
            size_t adjusted_size = index_count * sizeof(uint16_t);
            std::vector<std::byte> adjusted(adjusted_size);
            auto* dst = reinterpret_cast<uint16_t*>(adjusted.data());
            for (size_t i = 0; i < index_count; ++i) {
                dst[i] = static_cast<uint16_t>(src[i] + static_cast<uint16_t>(vertex_offset));
            }

            if (has_uint32) {
                // Need to convert to uint32 and redo previous
                // (too complex for V1 — just use uint32 for everything)
            }

            if (!has_uint32) {
                size_t prev_size = all_index_data.size();
                all_index_data.resize(prev_size + adjusted_size);
                std::memcpy(&all_index_data[prev_size], adjusted.data(), adjusted_size);
            }
        } else if (index_result->type == IndexType::Uint32) {
            auto* src = reinterpret_cast<const uint32_t*>(index_result->data.data());
            size_t adjusted_size = index_count * sizeof(uint32_t);
            std::vector<std::byte> adjusted(adjusted_size);
            auto* dst = reinterpret_cast<uint32_t*>(adjusted.data());
            for (size_t i = 0; i < index_count; ++i) {
                dst[i] = src[i] + static_cast<uint32_t>(vertex_offset);
            }

            if (!has_uint32 && !all_index_data.empty()) {
                // Previous data was uint16 — convert to uint32
                size_t old_count = all_index_data.size() / sizeof(uint16_t);
                std::vector<std::byte> converted(old_count * sizeof(uint32_t));
                auto* old_data = reinterpret_cast<const uint16_t*>(all_index_data.data());
                auto* new_data = reinterpret_cast<uint32_t*>(converted.data());
                for (size_t i = 0; i < old_count; ++i) {
                    new_data[i] = old_data[i];
                }
                all_index_data = std::move(converted);
                has_uint32 = true;
            }

            // Append the adjusted uint32 data
            if (all_index_data.empty()) {
                // First primitive — set directly
                all_index_data = std::move(adjusted);
            } else {
                size_t prev_size = all_index_data.size();
                all_index_data.resize(prev_size + adjusted_size);
                std::memcpy(&all_index_data[prev_size], adjusted.data(), adjusted_size);
            }
            index_type = IndexType::Uint32;
            has_uint32 = true;
        } else {
            return make_error(Error::Category::Unsupported,
                "Unsupported index type in mesh " + mesh.name);
        }

        // Determine material index
        int mat_idx = prim.material;
        if (mat_idx < 0) mat_idx = -1; // Use default

        int engine_mat_idx;
        auto map_it = material_map.find(mat_idx);
        if (map_it != material_map.end()) {
            engine_mat_idx = map_it->second;
        } else {
            // Create material
            auto material = create_pbr_material(device, model, mat_idx, base_dir);
            engine_mat_idx = static_cast<int>(materials.size());
            materials.push_back(material);
            material_map[mat_idx] = engine_mat_idx;
        }

        submeshes.push_back(SubMesh{
            static_cast<uint32_t>(total_indices),   // index_start
            static_cast<uint32_t>(index_count),      // index_count
            static_cast<uint32_t>(engine_mat_idx)    // material_index
        });

        total_indices += index_count;
    }

    if (all_vertices.empty() || total_indices == 0) {
        return std::optional<Model>{};
    }

    // Build vertex data
    std::vector<std::byte> vertex_data(all_vertices.size() * sizeof(Vertex));
    std::memcpy(vertex_data.data(), all_vertices.data(), vertex_data.size());

    // Create Model
    auto model_result = Model::create_indexed(
        device,
        k_standard_vertex_format,
        std::span<const std::byte>(vertex_data),
        std::span<const std::byte>(all_index_data),
        index_type,
        std::move(submeshes),
        std::move(materials));

    if (!model_result) {
        return std::unexpected(model_result.error());
    }

    // Add materials to out_materials
    for (auto& mat : materials) {
        out_materials.push_back(mat);
    }

    return std::move(*model_result);
}

// ============================================================================
// Recursive node builder
// ============================================================================
auto build_node(const tinygltf::Model& model,
                int node_idx,
                RenderDevice& device,
                const std::string& base_dir,
                float scale,
                std::vector<std::shared_ptr<Material>>& out_materials)
    -> Result<ModelNode>
{
    if (node_idx < 0 || node_idx >= static_cast<int>(model.nodes.size())) {
        return make_error(Error::Category::InvalidFormat,
            "Invalid node index: " + std::to_string(node_idx));
    }

    const auto& gltf_node = model.nodes[node_idx];
    ModelNode node;
    node.name = gltf_node.name;

    // Handle matrix transform (takes precedence over TRS per glTF spec)
    if (gltf_node.matrix.size() == 16) {
        // Decompose matrix into TRS using GLM
        glm::mat4 m;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = static_cast<float>(gltf_node.matrix[i * 4 + j]);

        glm::vec3 scale_glm;
        glm::quat rotation_glm;
        glm::vec3 translation_glm;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(m, scale_glm, rotation_glm, translation_glm, skew, perspective);

        node.translation = math::Vec3{translation_glm.x, translation_glm.y, translation_glm.z};
        node.rotation = math::Quat{rotation_glm.w, rotation_glm.x, rotation_glm.y, rotation_glm.z};
        node.scale = math::Vec3{scale_glm.x, scale_glm.y, scale_glm.z};
    } else {
        // Extract TRS
        if (gltf_node.translation.size() == 3) {
            node.translation = math::Vec3{
                static_cast<float>(gltf_node.translation[0]),
                static_cast<float>(gltf_node.translation[1]),
                static_cast<float>(gltf_node.translation[2])
            };
        }

        if (gltf_node.rotation.size() == 4) {
            // glTF stores quaternions as (x, y, z, w)
            // Engine Quat constructor is (w, x, y, z)
            node.rotation = math::Quat{
                static_cast<float>(gltf_node.rotation[3]), // w
                static_cast<float>(gltf_node.rotation[0]), // x
                static_cast<float>(gltf_node.rotation[1]), // y
                static_cast<float>(gltf_node.rotation[2])  // z
            };
        }

        if (gltf_node.scale.size() == 3) {
            node.scale = math::Vec3{
                static_cast<float>(gltf_node.scale[0]),
                static_cast<float>(gltf_node.scale[1]),
                static_cast<float>(gltf_node.scale[2])
            };
        }
    }

    // Build mesh if node has a mesh
    if (gltf_node.mesh >= 0) {
        auto model_result = build_model_from_mesh(
            device, model, gltf_node.mesh, base_dir, scale, out_materials);
        if (model_result) {
            if (*model_result) {
                node.model = std::move(**model_result);
            }
        } else {
            return std::unexpected(model_result.error());
        }
    }

    // Build children recursively
    for (int child_idx : gltf_node.children) {
        auto child_result = build_node(model, child_idx, device, base_dir, scale, out_materials);
        if (child_result) {
            node.children.push_back(std::move(*child_result));
        } else {
            return std::unexpected(child_result.error());
        }
    }

    return node;
}

} // anonymous namespace

// ============================================================================
// Custom image loader callback for tinygltf (uses project's stb_image)
// ============================================================================
namespace {

bool load_image_data_callback(tinygltf::Image* image, const int image_idx,
                              std::string* err, std::string* /*warn*/,
                              int /*req_width*/, int /*req_height*/,
                              const unsigned char* bytes, int size,
                              void* /*user_data*/)
{
    (void)image_idx;
    int w, h, comp;
    unsigned char* pixels = stbi_load_from_memory(bytes, size, &w, &h, &comp, 4);
    if (!pixels) {
        if (err) {
            *err = "Failed to decode image data: " + std::string(stbi_failure_reason());
        }
        return false;
    }

    image->width = w;
    image->height = h;
    image->component = 4;
    image->image.resize(static_cast<size_t>(w * h * 4));
    std::memcpy(image->image.data(), pixels, image->image.size());
    stbi_image_free(pixels);

    return true;
}

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================
auto load_gltf_model(RenderDevice& device,
                     const std::string& gltf_path,
                     float scale) -> Result<ModelLoadResult>
{
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(&load_image_data_callback, nullptr);
    tinygltf::Model gltf_model;
    std::string err;
    std::string warn;

    bool success = false;
    auto path_lower = gltf_path;
    std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (path_lower.size() >= 4 &&
        path_lower.substr(path_lower.size() - 4) == ".glb") {
        success = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, gltf_path);
    } else {
        success = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, gltf_path);
    }

    if (!warn.empty()) {
        BUDDD_LOG_WARN("glTF warning: {}", warn);
    }

    if (!success) {
        return make_error(Error::Category::InvalidFormat,
            "glTF parse error: " + err);
    }

    if (!err.empty()) {
        BUDDD_LOG_ERROR("glTF error: {}", err);
    }

    // Compute base directory for resolving relative texture paths
    std::string base_dir;
    auto fs_path = std::filesystem::path(gltf_path);
    base_dir = fs_path.parent_path().string();
    if (base_dir.empty()) {
        base_dir = ".";
    }

    // Determine scene
    std::vector<int> root_nodes;
    if (!gltf_model.scenes.empty()) {
        int scene_idx = gltf_model.defaultScene;
        if (scene_idx < 0 || scene_idx >= static_cast<int>(gltf_model.scenes.size())) {
            scene_idx = 0;
        }
        root_nodes = gltf_model.scenes[scene_idx].nodes;
    } else {
        // No scenes — all root-level nodes
        for (int i = 0; i < static_cast<int>(gltf_model.nodes.size()); ++i) {
            bool is_child = false;
            for (const auto& gltf_node : gltf_model.nodes) {
                for (int child : gltf_node.children) {
                    if (child == i) {
                        is_child = true;
                        break;
                    }
                }
                if (is_child) break;
            }
            if (!is_child) {
                root_nodes.push_back(i);
            }
        }
    }

    // Build root node
    ModelNode root;
    root.name = "";

    ModelLoadResult result;
    std::vector<std::shared_ptr<Material>> all_materials;

    for (int node_idx : root_nodes) {
        auto child_result = build_node(gltf_model, node_idx, device, base_dir, scale, all_materials);
        if (child_result) {
            root.children.push_back(std::move(*child_result));
        } else {
            return std::unexpected(child_result.error());
        }
    }

    result.root = std::move(root);
    result.materials = std::move(all_materials);

    return result;
}

} // namespace buddd::engine::detail

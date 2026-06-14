#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/property.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/camera_component.h"
#include "scene/free_camera_movement.h"
#include "scene/point_light_component.h"
#include "scene/directional_light_component.h"
#include "scene/spot_light_component.h"
#include "render/mesh_renderer.h"
#include "render/model.h"
#include "asset/asset_manager.h"
#include "math/vec3_yaml.h"
#include "math/vec4_yaml.h"
#include "math/quat_yaml.h"
#include "math/color_yaml.h"

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <string>

BUDDD_LOG_TAG("ComponentRegistry");

namespace buddd::engine {

void register_builtin_types() {
    // ── float ──
    TypeRegistry::register_type<float>({
        .yaml_encode = [](const float& v, const SerializationContext&) -> YAML::Node { return YAML::Node(v); },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<float> {
            try { return n.as<float>(); }
            catch (const YAML::Exception& e) {
                return make_error(Error::Category::InvalidArgument,
                    "float: expected scalar, got " + std::string(e.what()));
            }
        },
        .to_string = [](const float& v, const SerializationContext&) -> std::string { return std::to_string(v); },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<float> {
            float v;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "float: cannot parse '" + s + "'");
            }
            return v;
        },
        .validate = [](const float&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── int32_t ──
    TypeRegistry::register_type<int32_t>({
        .yaml_encode = [](const int32_t& v, const SerializationContext&) -> YAML::Node { return YAML::Node(v); },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<int32_t> {
            try { return n.as<int32_t>(); }
            catch (const YAML::Exception& e) {
                return make_error(Error::Category::InvalidArgument,
                    "int32_t: expected scalar, got " + std::string(e.what()));
            }
        },
        .to_string = [](const int32_t& v, const SerializationContext&) -> std::string { return std::to_string(v); },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<int32_t> {
            int32_t v;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "int32_t: cannot parse '" + s + "'");
            }
            return v;
        },
        .validate = [](const int32_t&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── bool ──
    TypeRegistry::register_type<bool>({
        .yaml_encode = [](const bool& v, const SerializationContext&) -> YAML::Node { return YAML::Node(v); },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<bool> {
            try { return n.as<bool>(); }
            catch (const YAML::Exception& e) {
                return make_error(Error::Category::InvalidArgument,
                    "bool: expected scalar, got " + std::string(e.what()));
            }
        },
        .to_string = [](const bool& v, const SerializationContext&) -> std::string {
            return v ? "true" : "false";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<bool> {
            if (s == "true") return true;
            if (s == "false") return false;
            return make_error(Error::Category::InvalidArgument,
                "bool: cannot parse '" + s + "' (expected 'true' or 'false')");
        },
        .validate = [](const bool&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── std::string ──
    TypeRegistry::register_type<std::string>({
        .yaml_encode = [](const std::string& v, const SerializationContext&) -> YAML::Node { return YAML::Node(v); },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<std::string> {
            try { return n.as<std::string>(); }
            catch (const YAML::Exception& e) {
                return make_error(Error::Category::InvalidArgument,
                    "string: expected scalar, got " + std::string(e.what()));
            }
        },
        .to_string = [](const std::string& v, const SerializationContext&) -> std::string { return v; },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<std::string> {
            return s;
        },
        .validate = [](const std::string&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── math::Vec3 ──
    TypeRegistry::register_type<math::Vec3>({
        .yaml_encode = [](const math::Vec3& v, const SerializationContext&) -> YAML::Node {
            return YAML::convert<math::Vec3>::encode(v);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Vec3> {
            math::Vec3 v;
            if (!YAML::convert<math::Vec3>::decode(n, v)) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec3: failed to decode YAML node (expected mapping with x, y, z)");
            }
            return v;
        },
        .to_string = [](const math::Vec3& v, const SerializationContext&) -> std::string {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Vec3> {
            if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
                return make_error(Error::Category::InvalidArgument,
                    "Vec3: cannot parse '" + s + "' (expected format '(x, y, z)')");
            }
            auto inner = s.substr(1, s.size() - 2);
            float x, y, z;
            auto [px, ex] = std::from_chars(inner.data(), inner.data() + inner.size(), x);
            if (ex != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec3: cannot parse '" + s + "' (expected format '(x, y, z)')");
            }
            while (px < inner.data() + inner.size() && (*px == ' ' || *px == ',')) ++px;
            auto [py, ey] = std::from_chars(px, inner.data() + inner.size(), y);
            if (ey != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec3: cannot parse '" + s + "' (expected format '(x, y, z)')");
            }
            while (py < inner.data() + inner.size() && (*py == ' ' || *py == ',')) ++py;
            auto [pz, ez] = std::from_chars(py, inner.data() + inner.size(), z);
            if (ez != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec3: cannot parse '" + s + "' (expected format '(x, y, z)')");
            }
            return math::Vec3{x, y, z};
        },
        .validate = [](const math::Vec3&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── math::Vec4 ──
    TypeRegistry::register_type<math::Vec4>({
        .yaml_encode = [](const math::Vec4& v, const SerializationContext&) -> YAML::Node {
            return YAML::convert<math::Vec4>::encode(v);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Vec4> {
            math::Vec4 v;
            if (!YAML::convert<math::Vec4>::decode(n, v)) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: failed to decode YAML node (expected mapping with x, y, z, w)");
            }
            return v;
        },
        .to_string = [](const math::Vec4& v, const SerializationContext&) -> std::string {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", "
                   + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Vec4> {
            if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            auto inner = s.substr(1, s.size() - 2);
            float x, y, z, w;
            auto [px, ex] = std::from_chars(inner.data(), inner.data() + inner.size(), x);
            if (ex != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (px < inner.data() + inner.size() && (*px == ' ' || *px == ',')) ++px;
            auto [py, ey] = std::from_chars(px, inner.data() + inner.size(), y);
            if (ey != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (py < inner.data() + inner.size() && (*py == ' ' || *py == ',')) ++py;
            auto [pz, ez] = std::from_chars(py, inner.data() + inner.size(), z);
            if (ez != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (pz < inner.data() + inner.size() && (*pz == ' ' || *pz == ',')) ++pz;
            auto [pw, ew] = std::from_chars(pz, inner.data() + inner.size(), w);
            if (ew != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Vec4: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            return math::Vec4{x, y, z, w};
        },
        .validate = [](const math::Vec4&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── math::Quat ──
    TypeRegistry::register_type<math::Quat>({
        .yaml_encode = [](const math::Quat& v, const SerializationContext&) -> YAML::Node {
            return YAML::convert<math::Quat>::encode(v);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Quat> {
            math::Quat v;
            if (!YAML::convert<math::Quat>::decode(n, v)) {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: failed to decode YAML node (expected mapping with x, y, z, w)");
            }
            return v;
        },
        .to_string = [](const math::Quat& v, const SerializationContext&) -> std::string {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", "
                   + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Quat> {
            if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            auto inner = s.substr(1, s.size() - 2);
            float x, y, z, w;
            auto [px, ex] = std::from_chars(inner.data(), inner.data() + inner.size(), x);
            if (ex != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (px < inner.data() + inner.size() && (*px == ' ' || *px == ',')) ++px;
            auto [py, ey] = std::from_chars(px, inner.data() + inner.size(), y);
            if (ey != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (py < inner.data() + inner.size() && (*py == ' ' || *py == ',')) ++py;
            auto [pz, ez] = std::from_chars(py, inner.data() + inner.size(), z);
            if (ez != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            while (pz < inner.data() + inner.size() && (*pz == ' ' || *pz == ',')) ++pz;
            auto [pw, ew] = std::from_chars(pz, inner.data() + inner.size(), w);
            if (ew != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Quat: cannot parse '" + s + "' (expected format '(x, y, z, w)')");
            }
            return math::Quat{w, x, y, z};
        },
        .validate = [](const math::Quat&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── math::Color ──
    TypeRegistry::register_type<math::Color>({
        .yaml_encode = [](const math::Color& v, const SerializationContext&) -> YAML::Node {
            return YAML::convert<math::Color>::encode(v);
        },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<math::Color> {
            math::Color v;
            if (!YAML::convert<math::Color>::decode(n, v)) {
                return make_error(Error::Category::InvalidArgument,
                    "Color: failed to decode YAML node (expected [r, g, b] or [r, g, b, a])");
            }
            return v;
        },
        .to_string = [](const math::Color& v, const SerializationContext&) -> std::string {
            return "(" + std::to_string(v.r) + ", " + std::to_string(v.g) + ", "
                   + std::to_string(v.b) + ", " + std::to_string(v.a) + ")";
        },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<math::Color> {
            if (s.size() < 2 || s.front() != '(' || s.back() != ')') {
                return make_error(Error::Category::InvalidArgument,
                    "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
            }
            auto inner = s.substr(1, s.size() - 2);
            float r, g, b, a;
            auto [pr, er] = std::from_chars(inner.data(), inner.data() + inner.size(), r);
            if (er != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
            }
            while (pr < inner.data() + inner.size() && (*pr == ' ' || *pr == ',')) ++pr;
            auto [pg, eg] = std::from_chars(pr, inner.data() + inner.size(), g);
            if (eg != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
            }
            while (pg < inner.data() + inner.size() && (*pg == ' ' || *pg == ',')) ++pg;
            auto [pb, eb] = std::from_chars(pg, inner.data() + inner.size(), b);
            if (eb != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
            }
            while (pb < inner.data() + inner.size() && (*pb == ' ' || *pb == ',')) ++pb;
            auto [pa, ea] = std::from_chars(pb, inner.data() + inner.size(), a);
            if (ea != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "Color: cannot parse '" + s + "' (expected format '(r, g, b, a)')");
            }
            return math::Color{r, g, b, a};
        },
        .validate = [](const math::Color&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // ── std::shared_ptr<Model> ──
    TypeRegistry::register_type<std::shared_ptr<Model>>({
        .yaml_encode = [](const std::shared_ptr<Model>& model, const SerializationContext& ctx) -> YAML::Node {
            if (!model) return YAML::Node("");
            return YAML::Node(ctx.assets.find_asset_id(*model));
        },
        .yaml_decode = [](const YAML::Node& node, const SerializationContext& ctx) -> Result<std::shared_ptr<Model>> {
            auto id = node.as<std::string>();
            if (id.empty()) return std::shared_ptr<Model>(nullptr);
            return ctx.assets.resolve_model(id);
        },
        .to_string = [](const std::shared_ptr<Model>& model, const SerializationContext& ctx) -> std::string {
            if (!model) return "";
            return ctx.assets.find_asset_id(*model);
        },
        .from_string = [](const std::string& str, const SerializationContext& ctx) -> Result<std::shared_ptr<Model>> {
            if (str.empty()) return std::shared_ptr<Model>(nullptr);
            return ctx.assets.resolve_model(str);
        },
        .validate = [](const std::shared_ptr<Model>&, const SerializationContext&) -> Result<void> {
            return {};  // always valid
        }
    });
}

void register_all_components(ComponentRegistry& registry) {
    // ── CameraComponent: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<CameraComponent>("camera");

        info.add_property<float>("fov_y",
            [](const CameraComponent& c) { return c.fov_y(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(v, c.aspect(), c.near_plane(), c.far_plane());
                return {};
            },
            PropertyFlags{}.min(0.001f).max(3.14159f)
        );

        info.add_property<float>("aspect",
            [](const CameraComponent& c) { return c.aspect(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), v, c.near_plane(), c.far_plane());
                return {};
            }
        );

        info.add_property<float>("near",
            [](const CameraComponent& c) { return c.near_plane(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), c.aspect(), v, c.far_plane());
                return {};
            }
        );

        info.add_property<float>("far",
            [](const CameraComponent& c) { return c.far_plane(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), c.aspect(), c.near_plane(), v);
                return {};
            }
        );
    }

    // ── PointLightComponent: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<PointLightComponent>("point_light");

        info.add_property<math::Color>("color",
            [](const PointLightComponent& c) -> math::Color { return c.color(); },
            [](PointLightComponent& c, const math::Color& v) -> Result<void> { c.color() = v; return {}; },
            PropertyFlags{}.tag("rgb")
        );

        info.add_property<float>("intensity",
            [](const PointLightComponent& c) { return c.intensity(); },
            [](PointLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("range",
            [](const PointLightComponent& c) { return c.range(); },
            [](PointLightComponent& c, float v) -> Result<void> { c.range() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── DirectionalLightComponent: uses overload (B) ──
    {
        auto& info = registry.register_component<DirectionalLightComponent>("directional_light");

        info.add_property<math::Color>("color",
            [](const DirectionalLightComponent& c) -> math::Color { return c.color(); },
            [](DirectionalLightComponent& c, const math::Color& v) -> Result<void> { c.color() = v; return {}; },
            PropertyFlags{}.tag("rgb")
        );

        info.add_property<float>("intensity",
            [](const DirectionalLightComponent& c) { return c.intensity(); },
            [](DirectionalLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── SpotLightComponent: uses overload (B) ──
    {
        auto& info = registry.register_component<SpotLightComponent>("spot_light");

        info.add_property<math::Color>("color",
            [](const SpotLightComponent& c) -> math::Color { return c.color(); },
            [](SpotLightComponent& c, const math::Color& v) -> Result<void> { c.color() = v; return {}; },
            PropertyFlags{}.tag("rgb")
        );

        info.add_property<float>("intensity",
            [](const SpotLightComponent& c) { return c.intensity(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("range",
            [](const SpotLightComponent& c) { return c.range(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.range() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("inner_angle",
            [](const SpotLightComponent& c) { return c.inner_angle(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.inner_angle() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("outer_angle",
            [](const SpotLightComponent& c) { return c.outer_angle(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.outer_angle() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── MeshRenderer: uses overload (C) — context needed for shared_ptr<Model> resolution ──
    {
        auto& info = registry.register_component<MeshRenderer>("mesh_renderer");

        info.add_property<std::shared_ptr<Model>>("model",
            [](const MeshRenderer& c, const SerializationContext&) -> std::shared_ptr<Model> {
                return c.model_ptr();
            },
            [](MeshRenderer& c, std::shared_ptr<Model> model, const SerializationContext&) -> Result<void> {
                c.set_model(std::move(model));
                return {};
            }
        );
    }

    // ── FreeCameraMovement: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<FreeCameraMovement>("free_camera_movement");

        info.add_property<float>("move_speed",
            [](const FreeCameraMovement& c) { return c.move_speed; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.move_speed = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("mouse_sensitivity",
            [](const FreeCameraMovement& c) { return c.mouse_sensitivity; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.mouse_sensitivity = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("pitch_clamp_degrees",
            [](const FreeCameraMovement& c) { return c.pitch_clamp_degrees; },
            [](FreeCameraMovement& c, float v) -> Result<void> { c.pitch_clamp_degrees = v; return {}; }
        );

        info.add_property<bool>("invert_yaw",
            [](const FreeCameraMovement& c) { return c.invert_yaw; },
            [](FreeCameraMovement& c, bool v) -> Result<void> { c.invert_yaw = v; return {}; }
        );

        info.add_property<bool>("invert_pitch",
            [](const FreeCameraMovement& c) { return c.invert_pitch; },
            [](FreeCameraMovement& c, bool v) -> Result<void> { c.invert_pitch = v; return {}; }
        );
    }
}

} // namespace buddd::engine

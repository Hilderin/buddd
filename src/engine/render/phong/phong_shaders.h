#pragma once

#include <string_view>

namespace buddd::engine::detail {

constexpr std::string_view k_phong_vertex_shader_source = R"(#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_texcoord;

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_mat;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    v_normal = normalize(mat3(u_normal_mat) * a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr std::string_view k_phong_fragment_shader_source = R"(#version 450 core

#define MAX_LIGHTS 8

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_texcoord;

out vec4 frag_color;

uniform int      u_light_count;
uniform vec4     u_light_positions_or_dir[MAX_LIGHTS];
uniform vec4     u_light_colors[MAX_LIGHTS];
uniform float    u_light_ranges[MAX_LIGHTS];
uniform vec4     u_light_spot_directions[MAX_LIGHTS];
uniform float    u_light_inner_cones[MAX_LIGHTS];
uniform float    u_light_outer_cones[MAX_LIGHTS];

uniform vec3     u_camera_pos;

uniform vec3     u_material_ambient   = vec3(0.1);
uniform vec4     u_material_specular  = vec4(1.0);
uniform float    u_material_shininess = 32.0;

uniform sampler2D u_diffuse_texture;
uniform vec4      u_material_diffuse_tint = vec4(1.0);

float spot_cone_attenuation(float cos_angle, float cos_inner, float cos_outer) {
    return clamp((cos_angle - cos_outer) / (cos_inner - cos_outer), 0.0, 1.0);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_camera_pos - v_world_pos);

    // Sample diffuse colour from texture, apply global tint
    vec3 diffuse_colour = texture(u_diffuse_texture, v_texcoord).rgb;
    diffuse_colour *= u_material_diffuse_tint.rgb;

    // Ambient term — constant, applied regardless of light count
    vec3 final_colour = u_material_ambient * diffuse_colour;

    for (int i = 0; i < u_light_count; ++i) {
        vec4 pos_or_dir = u_light_positions_or_dir[i];
        vec3 light_col  = u_light_colors[i].rgb;
        float range     = u_light_ranges[i];
        vec3 L;
        float attenuation = 1.0;

        if (pos_or_dir.w == 0.0) {
            // Directional light
            L = normalize(pos_or_dir.xyz);
        } else if (pos_or_dir.w == 1.0) {
            // Point light
            vec3 light_to_frag = pos_or_dir.xyz - v_world_pos;
            float dist = length(light_to_frag);
            L = light_to_frag / dist;

            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;
        } else {
            // Spot light (w == 2.0)
            vec3 light_to_frag = pos_or_dir.xyz - v_world_pos;
            float dist = length(light_to_frag);
            L = light_to_frag / dist;

            float normalized_dist = clamp(dist / range, 0.0, 1.0);
            attenuation = 1.0 - normalized_dist * normalized_dist;

            // Spot cone falloff
            vec3 spot_dir = normalize(u_light_spot_directions[i].xyz);
            float cos_angle = max(dot(-L, spot_dir), 0.0);
            float cos_inner = u_light_inner_cones[i];
            float cos_outer = u_light_outer_cones[i];
            attenuation *= spot_cone_attenuation(cos_angle, cos_inner, cos_outer);
        }

        // Diffuse (Lambert)
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = diffuse_colour * light_col * NdotL;

        // Specular (Blinn-Phong)
        vec3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        vec3 specular = u_material_specular.rgb * light_col * pow(NdotH, u_material_shininess);

        final_colour += (diffuse + specular) * attenuation;
    }

    // Reinhard tone mapping with exposure compensation.
    // Exposure amplifies the signal before compression so that the
    // resulting image has a natural brightness. Without exposure,
    // a typical diffuse-lit surface (value ~1.0) would map to 0.5 (gray).
    // ------------------------------------------------------------------
    // Key mappings (exposure = 3.0):
    //   0.0 → 0.0  |  0.33 → 0.50 (128)  |  0.5 → 0.60 (153)
    //   1.0 → 0.75 (191)  |  2.0 → 0.86 (219)  |  5.0 → 0.94 (239)
    //   ∞ → 1.0 (never reached)
    // ------------------------------------------------------------------
    // The higher exposure keeps mid-tones brighter while still preventing
    // hard white clip. Specular highlights from multiple strong lights
    // accumulating to extreme values (5.0+) compress smoothly to ~239+
    // without ever saturating to pure 255.
    const float k_exposure = 3.0;
    vec3 exposed = final_colour * k_exposure;
    vec3 tonemapped = exposed / (exposed + vec3(1.0));

    // Safety net: replace any NaN with black to prevent GPU-dependent
    // artifacts at silhouette edges.
    if (isnan(tonemapped.x) || isnan(tonemapped.y) || isnan(tonemapped.z)) {
        tonemapped = vec3(0.0);
    }

    frag_color = vec4(tonemapped, 1.0);
}
)";

} // namespace buddd::engine::detail
#pragma once

#include <string_view>

namespace buddd::engine::detail {

constexpr std::string_view k_pbr_vertex_shader_source = R"(#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord;
layout(location = 4) in vec4 a_tangent;   // Reserved, not used in V1
layout(location = 5) in vec2 a_texcoord2;  // Reserved, not used in V1

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_mat;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_texcoord;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    v_normal = normalize(mat3(u_normal_mat) * a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr std::string_view k_pbr_fragment_shader_source = R"(#version 450 core

#define MAX_LIGHTS 8

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_texcoord;

out vec4 frag_color;

// Material parameters
uniform vec4  u_base_color_factor;
uniform float u_metallic_factor;
uniform float u_roughness_factor;
uniform vec3  u_emissive_factor;

// Textures
uniform sampler2D u_base_color_texture;
uniform sampler2D u_metallic_roughness_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_occlusion_texture;
uniform sampler2D u_emissive_texture;

// Has-texture flags (1.0 = texture present, 0.0 = use factor)
uniform float u_has_base_color_texture;
uniform float u_has_metallic_roughness_texture;
uniform float u_has_normal_texture;
uniform float u_has_occlusion_texture;
uniform float u_has_emissive_texture;

// Lighting (same as Phong for V1 compatibility)
uniform vec3 u_camera_pos;
uniform int  u_light_count;
uniform vec4 u_light_positions_or_dir[MAX_LIGHTS];
uniform vec4 u_light_colours[MAX_LIGHTS];
uniform float u_light_ranges[MAX_LIGHTS];
uniform vec4 u_light_spot_directions[MAX_LIGHTS];
uniform float u_light_inner_cones[MAX_LIGHTS];
uniform float u_light_outer_cones[MAX_LIGHTS];

const float PI = 3.14159265359;
const float k_exposure = 3.0;

float spot_cone_attenuation(float cos_angle, float cos_inner, float cos_outer) {
    return clamp((cos_angle - cos_outer) / (cos_inner - cos_outer), 0.0, 1.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometry_schlick_ggx(NdotV, roughness)
         * geometry_schlick_ggx(NdotL, roughness);
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_camera_pos - v_world_pos);

    // Sample base colour
    vec4 base_color = u_base_color_factor;
    if (u_has_base_color_texture > 0.5) {
        base_color *= texture(u_base_color_texture, v_texcoord);
    }

    // Sample metallic-roughness
    float metallic = u_metallic_factor;
    float roughness = u_roughness_factor;
    if (u_has_metallic_roughness_texture > 0.5) {
        vec4 mr = texture(u_metallic_roughness_texture, v_texcoord);
        roughness *= mr.g;
        metallic *= mr.b;
    }

    // Sample occlusion
    float occlusion = 1.0;
    if (u_has_occlusion_texture > 0.5) {
        occlusion = texture(u_occlusion_texture, v_texcoord).r;
    }

    // Sample emissive
    vec3 emissive = u_emissive_factor;
    if (u_has_emissive_texture > 0.5) {
        emissive *= texture(u_emissive_texture, v_texcoord).rgb;
    }

    // F0 for dielectrics (0.04) or metals (base_color.rgb)
    vec3 F0 = mix(vec3(0.04), base_color.rgb, metallic);

    // Loop over lights
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_light_count; ++i) {
        vec4 pos_or_dir = u_light_positions_or_dir[i];
        vec3 light_col  = u_light_colours[i].rgb;
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

        vec3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);

        // Cook-Torrance BRDF
        float D = distribution_ggx(N, H, roughness);
        float G = geometry_smith(N, V, L, roughness);
        vec3  F = fresnel_schlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        vec3 specular = D * G * F / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);
        vec3 diffuse = kD * base_color.rgb / PI;

        Lo += (diffuse + specular) * u_light_colours[i].rgb * NdotL * attenuation;
    }

    // Ambient lighting
    vec3 ambient = base_color.rgb * occlusion * 0.1;
    ambient += base_color.rgb * 0.05;
    vec3 color = ambient + Lo + emissive;

    // Tone mapping (Reinhard)
    vec3 exposed = color * k_exposure;
    vec3 tonemapped = exposed / (exposed + vec3(1.0));
    if (isnan(tonemapped.x) || isnan(tonemapped.y) || isnan(tonemapped.z)) {
        tonemapped = vec3(0.0);
    }

    frag_color = vec4(tonemapped, 1.0);
}
)";

} // namespace buddd::engine::detail

#version 450 core
in vec2 v_texcoord;
out vec4 color;
uniform float roughness;
uniform sampler2D albedo;
void main(){
    color = vec4(roughness);
}

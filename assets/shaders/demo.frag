#version 450 core
in vec2 v_texcoord;
out vec4 frag_color;
uniform sampler2D u_tex;
void main() {
    frag_color = texture(u_tex, v_texcoord);
}

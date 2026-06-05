#version 450 core
layout(location = 0) in vec3 pos;
out vec2 v_texcoord;
void main(){
    gl_Position = vec4(pos,1);
    v_texcoord = vec2(0.0);
}

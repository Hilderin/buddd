#version 450 core
#error this shader has a compile error
layout(location = 0) in vec3 pos;
void main(){
    gl_Position = vec4(pos,1);
}

#version 420 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uvs;

out vec2 UVs;

void main() {

    UVs = uvs;

    gl_Position = vec4(pos, 0.0, 1.0);    
}

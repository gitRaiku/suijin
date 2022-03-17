#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

out vec3 passCol;

void main() {
    passCol = col;
    gl_Position = vec4(pos, 1.0f);
}

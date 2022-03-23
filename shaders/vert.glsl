#version 460 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 col;

out vec4 passCol;

uniform mat3 fn_mat;

void main() {
    passCol = col;
    gl_Position = vec4(fn_mat * vec3(pos, 1.0f), 1.0f);
}

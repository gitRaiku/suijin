#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

out vec3 passCol;

uniform mat3 affine_transform;

void main() {
    passCol = col;
    gl_Position = vec4(pos * affine_transform, 1.0f);
}

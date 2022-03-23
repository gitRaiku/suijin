#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

out vec3 passCol;

//uniform mat3 tl_mat;
//uniform mat3 rs_mat;
uniform mat3 fn_mat;

void main() {
    passCol = col;
    gl_Position = vec4(fn_mat * pos, 1.0f);
}

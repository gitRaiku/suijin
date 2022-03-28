#version 460 core
layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 col;

out vec4 passCol;

uniform mat4 fn_mat;

void main() {
    passCol = col;
    gl_Position = fn_mat * pos;

    float l = length(gl_Position);
    gl_PointSize = (1 / (l)) * 20.0f;
    //gl_PointSize = l;
}

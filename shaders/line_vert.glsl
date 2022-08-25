#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 col;

uniform mat4 fn_mat;

out vec4 pcol;

void main() {
  gl_Position = fn_mat * vec4(pos, 1.0f);
  pcol = col;
}

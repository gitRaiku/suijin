#version 460 core
layout (location = 0) in vec3 pos;

uniform mat4 fn_mat;
uniform vec4 col;

out vec4 pcol;

void main() {
  gl_Position = fn_mat * vec4(pos, 1.0f);
  pcol = col;
}

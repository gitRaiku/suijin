#version 460 core
layout (location = 0) in vec3 pos;

uniform mat4 fn_mat;
uniform float ps;

void main() {
  if (ps <= 1.0f) {
    gl_Position = fn_mat * vec4(pos, 1.0f);
    gl_PointSize = 300 / length(gl_Position);
  } else {
    gl_Position = vec4(pos, 1.0f);
    gl_PointSize = ps;
  }
}

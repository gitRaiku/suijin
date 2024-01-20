#version 460 core
layout (location = 0) in vec3 pos;

out vec3 pp;
out vec3 lp;

uniform mat4 affine;
uniform mat4 fn;

void main() {
  gl_Position = fn * affine * vec4(pos, 1.0f);
  lp = pos;
  pp = (affine * vec4(pos, 1.0)).xyz;
}

#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 tc;

uniform mat4 affine;
uniform mat4 fn;

void main() {
  gl_Position = fn * affine * vec4(pos, 1.0f);
  tc = tex;
}

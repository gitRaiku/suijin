#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec3 passNorm;
out vec3 passPos;

uniform mat4 fn_mat;

void main() {
  passNorm = norm;
  vec4 magn = vec4(pos.x * 100, pos.y * 100, pos.z * 100, 1.0f);
  passPos = magn.xyz;
  // vec4 magn = vec4(pos, 1.0f);
  gl_Position = fn_mat * magn;
}

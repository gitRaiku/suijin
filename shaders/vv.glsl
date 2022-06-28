#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec3 passNorm;
out vec3 passPos;
out vec2 passTex;

uniform mat4 fn_mat;
uniform mat4 affine;

void main() {
  passPos = pos;
  passNorm = norm;
  passTex = tex;

  gl_Position = fn_mat * affine * vec4(pos, 1.0f);
}

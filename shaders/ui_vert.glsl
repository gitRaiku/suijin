#version 460 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 tex;

out vec2 texCoords;

uniform mat4 affine;

void main() {
  gl_Position = affine * vec4(pos.x, pos.y, 0.0f, 1.0f);
  texCoords = tex;
}

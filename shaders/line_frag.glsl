#version 460 core

out vec4 FragColor;

in vec4 pcol;

void main() {
  FragColor = pcol;
}

#version 460 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;
uniform bool hasTexture;
uniform vec4 col;

void main() {
  if (hasTexture) {
    FragColor = vec4(texture(tex, texCoords).r);
  } else {
    FragColor = col;
  }
}

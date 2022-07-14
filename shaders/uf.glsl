#version 460 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;
uniform bool hasTexture;

void main() {
  if (hasTexture) {
    FragColor = vec4(texture(tex, texCoords).r);
  } else {
    //FragColor = vec4(0.094, 0.094, 0.094, 1.0f);
    FragColor = vec4(0.5, 0.5, 0.5, 0.2f);
  }
}

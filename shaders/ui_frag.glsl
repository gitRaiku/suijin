#version 460 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;
uniform bool hasTexture;
uniform vec4 col;

void main() {
  if (hasTexture) {
    //FragColor = vec4(vec3(texture(tex, texCoords).r), 1.0f);
    FragColor = vec4(texture(tex, texCoords).rgb, 1.0f);
  } else {
    FragColor = col;
  }
}

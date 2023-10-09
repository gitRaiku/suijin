#version 460 core
#define PI 3.14159265

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;
uniform sampler3D tex3;
uniform float z;
uniform int ch;
uniform vec4 col;
uniform int type; // 0: No Texture, colour only
                  // 1: 2d Read from the texture
                  // 2: 2d Heat colours
                  //
                  // 8: 3d

vec3 h2c3(int c, int x) {
  return vec3(((c & 0xFF00) >> 8) / 255.0f, ((c & 0x00FF) >>  0) / 255.0f, ((x & 0x00FF) >>  0) / 255.0f);
}

vec3 get_heat(float v) {
  float k = 0.1;
  float s = 1 / (1 + exp(-(v * 2 - 1) / k));
  return mix(h2c3(0x0100, 0x4f), h2c3(0xff46, 0x0a), s);
}

void main() {
  if (type == 0) {
    FragColor = col;
  } else if (type == 1) {
    FragColor = vec4(vec3(texture(tex, texCoords).r), 1.0f);
  } else if (type == 2) {
    FragColor = vec4(get_heat(texture(tex, texCoords).r), 1.0f);
  } else if (type == 8) {
    vec4 c = texture(tex3, vec3(texCoords.x, texCoords.y, z));
    double v;
    if (ch == 0) {
      v = c.r;
    } else if (ch == 1) {
      v = c.g;
    } else if (ch == 2) {
      v = c.b;
    } else {
      v = c.a;
    }
    FragColor = vec4(vec3(v), 1.0f);
  }
}

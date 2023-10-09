#version 460 core
#define PI 3.14159265

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;
uniform vec4 bgcol;
uniform vec4 fgcol;

float ablend(float alpha, float o1, float o2) {
  return alpha * o1 + (1 - alpha) * o2;
}

void main() {
  vec3 ct = texture(tex, texCoords).rgb;

  FragColor = vec4(pow(ablend(ct.r, fgcol.r, bgcol.r), 1.8),
                   pow(ablend(ct.r, fgcol.g, bgcol.g), 1.8),
                   pow(ablend(ct.r, fgcol.b, bgcol.b), 1.8),
                   ablend(ct.r, fgcol.a, bgcol.a));
}

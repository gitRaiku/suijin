#version 460 core

out vec4 FragColor;

in vec3 pp;
in vec3 lp;
uniform vec3 camPos;
uniform vec3 scale;
uniform vec3 offs;

uniform sampler3D tex;

vec3 fix(vec3 p) { return vec3(p + vec3(1.0)) / 2.0; }

float p2(float x) { return x * x; }

#define PI 3.1415

void main() {
  float px = abs(pp.x - camPos.x);
  vec3 dist = camPos - pp;
  float latDist = sqrt(p2(dist.x) + p2(dist.z));
  float vertAngle = atan(-dist.y / latDist);
  int quad;
  if (dist.z >= 0) { if (dist.x >= 0) { quad = 3; } else { quad = 2; } } 
              else { if (dist.x >= 0) { quad = 0; } else { quad = 1; } }
  float ha = atan(abs(dist.x) / abs(dist.z));
  float horiAngle = 0;
  horiAngle = (((quad & 2)!=0) ?  1 : 3) * PI / 2 + 
              (((quad & 1)!=0) ? -1 : 1) * ha;

  vec3 orientVec = normalize(vec3(cos(vertAngle) * cos(horiAngle), sin(vertAngle), cos(vertAngle) * sin(horiAngle)));
  FragColor = vec4(orientVec, 1.0);
  //FragColor = vec4(texture(tex, vec3(pp.x / 10, pp.y / 10, pp.z / 10 )).xyz, 1.0);
  vec3 p = fix(lp);
  FragColor = vec4(vec3(texture(tex, p).x, 0.0, 0.0), 1.0);

  return;
  /*
  vec3 p = fix(pp);
  vec4 text = vec4(texture(tex, p).xyz, 1.0);
  vec4 rec = text * clamp(sin(p.z * 5 - 0.3), 0, 1) * p.z * 2;

  FragColor = rec;*/
}

#version 460 core

out vec4 FragColor;

in vec3 pp;
in vec3 lp;
uniform vec3 centerPos;
uniform vec3 centerScale;
uniform vec3 camPos;

uniform sampler3D tex;

vec3 fix(vec3 p) { return vec3(p + vec3(1.0)) / 2.0; }

float p2(float x) { return x * x; }

#define PI 3.1415

vec3 dist(vec3 p) {
  return abs(p - centerPos) / centerScale;
}

float stepLen = 0.01;
int maxSteps = 200;
vec4 start_march(vec3 spos, vec3 dir) {
  vec3 cd = dist(spos);
  float cm = max(max(cd.z, cd.y), cd.x);
  vec3 cpos = spos;
  int cstep = 0;
  while (cm > 2.0 && cstep < maxSteps) {
    ++cstep;
    cpos += dir * stepLen;
    cd = dist(cpos);
    cm = max(max(cd.z, cd.y), cd.x);
    if (cm < 1.0) {
      return vec4(1.0);
    }
  }
  //float cm = max(max(cd.x, cd.y), cd.z);
  return vec4(vec3(0.0), 0.6);
}

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
  FragColor = start_march(pp, orientVec);
  return;
  /*
  vec3 p = fix(pp);
  vec4 text = vec4(texture(tex, p).xyz, 1.0);
  vec4 rec = text * clamp(sin(p.z * 5 - 0.3), 0, 1) * p.z * 2;

  FragColor = rec;*/
}

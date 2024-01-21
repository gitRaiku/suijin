#version 460 core

out vec4 FragColor;

in vec3 pp;
in vec3 lp;
uniform vec3 centerPos;
uniform vec3 centerScale;
uniform vec3 camPos;

uniform sampler3D tex;

vec3 fix(vec3 p) { return vec3(p + vec3(1.0)) / 2.0; }

#define PI 3.1415

vec3 dist(vec3 p) {
  return (centerPos - p) / centerScale;
}

float stepLen = 0.01;
float okdist = 0.5;
int maxSteps = 100;
vec4 gcol(float x) { return vec4(vec3(x), 1.0); }

//float cm = max(max(cd.z, cd.y), cd.x);
vec4 start_march(vec3 spos, vec3 dir) {
  vec3 cd = dist(spos);
  float cm = length(cd);
  int cstep = 0;

  while (cm > okdist && cstep < maxSteps) {
    ++cstep;
    cd += dir * stepLen;
    cm = length(cd);
    if (cm < okdist) {
      return vec4(dir, 1.0);
    }
  }
  if (cstep == maxSteps) {
    return vec4(vec3(0.1), 0.2);
  } else {
    return vec4(dir, 1.0);
  }
  //float cm = max(max(cd.x, cd.y), cd.z);
  return vec4(vec3(cstep / maxSteps), 0.6);
}

void main() {
  float px = abs(pp.x - camPos.x);
  vec3 dist = camPos - pp;
  float latDist = length(dist.xz);
  float vertAngle = atan(-dist.y / latDist);
  int quad;
  if (dist.z >= 0) { if (dist.x >= 0) { quad = 3; } else { quad = 2; } } 
              else { if (dist.x >= 0) { quad = 0; } else { quad = 1; } }
  float ha = atan(abs(dist.x) / abs(dist.z));
  float horiAngle = 0;
  horiAngle = (((quad & 2)!=0) ?  1 : 3) * PI / 2 + 
              (((quad & 1)!=0) ? -1 : 1) * ha;

  vec3 orientVec = normalize(vec3(cos(vertAngle) * cos(horiAngle), -sin(vertAngle), cos(vertAngle) * sin(horiAngle)));
  FragColor = start_march(pp, orientVec);
  return;
  /*
  vec3 p = fix(pp);
  vec4 text = vec4(texture(tex, p).xyz, 1.0);
  vec4 rec = text * clamp(sin(p.z * 5 - 0.3), 0, 1) * p.z * 2;

  FragColor = rec;*/
}

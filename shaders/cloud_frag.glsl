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

float stepLen = 0.10;
float okdist = 1.01;
int maxSteps = 10000;
vec4 gcol(float x) { return vec4(vec3(x), 1.0); }

//float cm = max(max(cd.z, cd.y), cd.x);

float gm(vec3 p) {
  vec3 pp = abs(p);
  return max(max(pp.x, pp.y), pp.z);
}

vec4 start_march(vec3 spos, vec3 dir) {
  vec3 cp = spos;
  float cm = gm(dist(cp));
  int cstep = 0;
  float ccol = 0.0;

  while (cm <= okdist && cstep < maxSteps) {
    ++cstep;
    cp -= dir * stepLen;
    cm = gm(dist(cp));
    float c = texture(tex, abs(fix(dist(cp)))).x; 
    ccol += c;
  }
  /*
  if (cstep < maxSteps) {
    return vec4(vec3(1.0), 1.0);
  } else {
    return vec4(vec3(0.1), 0.1);
  }*/
  //return vec4(vec3(exp(-ccol)), 0.1);
  float ldir = 1.0;//dot(dir, normalize(vec3(0.5, -1.0, 0.0)));
  //return vec4(vec3(ldir), 1.0);
  ccol = (ccol / cstep) * (cstep * stepLen) * 0.07;
  return vec4(vec3(ccol), 1.0);
  float ccp = exp(-(ccol * 0.3)) * ldir;
  return vec4(vec3(ccp), 1.0);
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

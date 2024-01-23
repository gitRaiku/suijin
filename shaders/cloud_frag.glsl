#version 460 core

out vec4 FragColor;

in vec3 pp;
in vec3 lp;
uniform vec3 centerPos;
uniform vec3 centerScale;
uniform vec3 camPos;

uniform sampler3D tex;

#define PI 3.1415
vec3 fix(vec3 p) { return vec3(p + vec3(1.0)) / 2.0; }
vec3 dist(vec3 p) { return (centerPos - p) / centerScale; }

int maxSteps = 10000000;
float stepLen = centerScale.y * 1.01;
float okdist = 1.01;

float gm(vec3 p) {
  vec3 pp = abs(p);
  return max(max(pp.x, pp.y), pp.z);
}

vec4 start_march(vec3 spos, vec3 dir) {
  dir = vec3(dir.x, -dir.y, dir.z);
  vec3 cp = spos;
  float cm = gm(dist(cp));
  int cstep = 0;
  float ccol = 0.0;

  while (cm <= okdist && cstep < maxSteps) {
    ++cstep;
    cp += dir * stepLen;
    cm = gm(dist(cp));
    float c = texture(tex, abs(fix(dist(cp)))).x; 
    ccol += c;
  }

  float ldir = dot(dir, normalize(vec3(1.5, -1.0, 0.0)));
  ccol = (ccol / cstep) * (cstep * stepLen) * 0.08;
  float ccp = 1 - exp(-ccol) * ldir;
  return vec4(vec3(ccp) * vec3(253 / 255.0, 134 / 255.0, 149 / 255.0), ccp * 2);
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
  //FragColor = start_march(pp, orientVec);
  //return;
  FragColor = start_march(pp, orientVec);
  return;
  /*
  vec3 p = fix(pp);
  vec4 text = vec4(texture(tex, p).xyz, 1.0);
  vec4 rec = text * clamp(sin(p.z * 5 - 0.3), 0, 1) * p.z * 2;

  FragColor = rec;*/
}

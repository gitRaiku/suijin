#version 460 core

out vec4 FragColor;

in vec3 pp;
in vec3 lp;
uniform vec3 centerPos;
uniform vec3 centerScale;
uniform vec3 camPos;

uniform sampler3D tex;

uniform float minDensity = 0.2;
uniform float densityMultiplier = 3.4;
uniform int numSteps = 10;
uniform int numStepsLight = 10;

uniform float lightDarknessThresh = 0.1;
uniform float lightAbsorptionTowardSun = 10.8;
uniform float lightAbsorptionThroughCloud = 1.0;
uniform vec3 sunDir = normalize(vec3(0.3, 0.6, 0.4));

vec3 boundsMin = centerPos - centerScale;
vec3 boundsMax = centerPos + centerScale;

#define PI 3.1415
vec3 fix(vec3 p) { vec3 res = vec3(p + vec3(1.0)) / 2.0; return vec3(res.x, 1 - res.y, res.z); }
vec3 dist(vec3 p) { return (centerPos - p) / centerScale; }

float gm(vec3 p) {
  vec3 pp = abs(p);
  return max(max(pp.x, pp.y), pp.z);
}

float g = 0.8;
float heg(float a) { return (1 - g*g)/((4 * PI) * pow((1 + g*g - 2*g*a), 3/3)); }

float offL = 0.50;
float offP = 0.2;
float mata(float c) { return 1; return min(max(offL - abs(c + offP), 0.0) * 20, 1.0); }

vec3 resetScale = vec3(1.0, centerScale.y / centerScale.x, centerScale.z / centerScale.x) * 1.0;
vec3 cloudScale = vec3(1.0, 1.0, 1.0);
float cp1(float x, int y) { if ((y & 1) != 0) { return x - y; } else { return 1.0 - x + y; } }
vec3 cp3(vec3 x, ivec3 y) { x.x = cp1(x.x, y.x); x.y = cp1(x.y, y.y); x.z = cp1(x.z, y.z); return x; }

float sample_dens(vec3 pos) { /// TODO: Nu uita de mata
  vec3 texPos = pos * resetScale * cloudScale;
  ivec3 iv = ivec3(texPos);
  texPos = cp3(texPos, iv);
  float res = texture(tex, texPos).r;
  res = max(0, res - minDensity) * densityMultiplier;
  return res;
}

vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, vec3 rayOrigin, vec3 invRaydir) {
  vec3 t0 = (boundsMin - rayOrigin) * invRaydir;
  vec3 t1 = (boundsMax - rayOrigin) * invRaydir;
  vec3 tmin = min(t0, t1);
  vec3 tmax = max(t0, t1);
  
  float dstA = max(max(tmin.x, tmin.y), tmin.z);
  float dstB = min(min(tmax.x, tmax.y), tmax.z);

  float dstToBox = max(0, dstA);
  float dstInsideBox = max(0, dstB - dstToBox);
  return vec2(dstToBox, dstInsideBox);
}

float light_march(vec3 pos) {
  float dstInsideBox = rayBoxDst(boundsMin, boundsMax, pos, 1 / sunDir).y;
  
  float stepSize = dstInsideBox / numStepsLight;
  float totalDensity = 0;

  for (int i = 0; i < numStepsLight; ++i) {
    pos += sunDir * stepSize;
    totalDensity += max(0, sample_dens(fix(dist(pos))) * stepSize);
  }

  float transmittance = exp(-totalDensity * lightAbsorptionTowardSun);
  return lightDarknessThresh + transmittance * (1 - lightDarknessThresh);
}

vec4 start_march(vec3 spos, vec3 dir, float rdist) {
  float stepLen = rdist / numSteps;
  dir = vec3(dir.x, dir.y, dir.z) * stepLen;
  vec3 d;
  vec3 cp = spos;
  float cm = 0.0;
  float cd, ld;

  float lightEnergy = 0.0;
  float transmittance = 1.0;
  float dstTravelled = 0.0;
  for (int i = 0; i < numSteps; ++i) {
    dstTravelled += stepLen;
    d = dist(cp);
    cd = sample_dens(fix(d));
    if (cd > 0.001) {
      ld = light_march(cp);
      lightEnergy += cd * ld * stepLen * transmittance * 1.0; /// Add hg
      transmittance *= exp(-cd * stepLen * lightAbsorptionThroughCloud);

      if (transmittance < 0.01) { break; }
    }

    cp += dir;
    cm = gm(d);
  }

  return vec4(lightEnergy * vec4(1.0, 1.0, 1.0, 1.0));
}

vec3 getOrientation(vec3 pp, vec3 cpos) {
  float px = abs(pp.x - cpos.x);
  vec3 dist = cpos - pp;
  float latDist = length(dist.xz);
  float vertAngle = atan(-dist.y / latDist);
  int quad;
  if (dist.z >= 0) { if (dist.x >= 0) { quad = 3; } else { quad = 2; } } 
              else { if (dist.x >= 0) { quad = 0; } else { quad = 1; } }
  float ha = atan(abs(dist.x) / abs(dist.z));
  float horiAngle = 0;
  horiAngle = (((quad & 2)!=0) ?  1 : 3) * PI / 2 + 
              (((quad & 1)!=0) ? -1 : 1) * ha;

  return normalize(-vec3(
          cos(vertAngle) * cos(horiAngle), 
         -sin(vertAngle)                 , 
          cos(vertAngle) * sin(horiAngle)));
}

void main() {
  vec3 orientVec = getOrientation(pp, camPos);
  vec2 rdist = rayBoxDst(boundsMin, boundsMax, camPos, 1 / orientVec);  

  FragColor = start_march(pp, orientVec, rdist.y);
}

#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint dimensions;
uniform uint w;
uniform uint h;
uniform uint d;
uniform float scale;

layout(rgba32f, binding = 0) uniform image2D img2d;
layout(rgba32f, binding = 2) uniform image3D img3d;
layout (std430, binding = 1) buffer Pos { float a[]; } pts;

#define T(A, a, b, c, w, h, o) A[o + ((a) + (b) * (w) + (c) * (w * h)) * 3]
#define D(A, a, b, w, o) A[o + ((a) + (b) * (w)) * 2]
#define P2(x) ((x)*(x))

float dist3(float x, float y, float z, vec3 v) { return P2(v.x - x) + P2(v.y - y) + P2(v.z - z); }
float gnear3(uint x, uint y, uint z, uint udx, uint udy, uint udz) {
  uint ux = uint(x / scale);
  uint uy = uint(y / scale);
  uint uz = uint(z / scale);
  float md = 99999999999999.9;

  int i, j, k;
  for (k = 0; k <= 2; ++k) {
    for (j = 0; j <= 2; ++j) {
      for (i = 0; i <= 2; ++i) {
        vec3 p = vec3(T(pts.a, ux + i - 1, uy + j - 1, uz + k - 1, udx, udy, 0), 
                      T(pts.a, ux + i - 1, uy + j - 1, uz + k - 1, udx, udy, 1), 
                      T(pts.a, ux + i - 1, uy + j - 1, uz + k - 1, udx, udy, 2)); 
        md = min(md, dist3(x, y, z, p));
      }
    }
  }

  return 1 - clamp(sqrt(md * 0.5) / scale, 0.0f, 1.0f);
}

float PHI = 1.61803398874989484820459;
float seed = 123.1;
float gnoise(vec2 xy) { return fract(sin(distance(vec2(xy.x * PHI + 9.124 * seed, xy.y * 8.5141261 + 2.145 * seed),xy.yx)*seed)*xy.x); }

float igud(ivec3 x) {
  if (10 <= x.x && x.x <= 15) {
    if (5 <= x.z && x.z <= 10) {
      return 1.0;
    }
  }

  if (20 <= x.x && x.x <= 23) {
    if (8 <= x.z && x.z <= 13) {
      return 1.0;
    }
  }

  return 0.0;
}

#define GI gl_GlobalInvocationID
void tmkw3d() { 
  float dx = w / scale;
  float dy = h / scale;
  float dz = d / scale;
  uint udx = uint(dx + 3);
  uint udy = uint(dy + 3);
  uint udz = uint(dz + 3);
  ivec3 texelCoord = ivec3(GI.xyz);
  //imageStore(img3d, texelCoord, vec4(vec3(igud(texelCoord)), 1.0)); return;

  imageStore(img3d, texelCoord, vec4(vec3(
          gnear3(uint(GI.x + scale + 1), 
                 uint(GI.y + scale + 1), 
                 uint(GI.z + scale + 1), 
                 udx, udy, udz)), 0.0));
}

float dist2(float x, float y, vec2 v) { return P2(v.x - x) + P2(v.y - y); }
float gnear2(uint x, uint y, uint udx, uint udy) {
  uint ux = uint(x / scale);
  uint uy = uint(y / scale);
  float md = 99999999999999.9;

  int i, j;
  for (j = 0; j <= 2; ++j) {
    for (i = 0; i <= 2; ++i) {
      vec2 p = vec2(D(pts.a, ux + i - 1, uy + j - 1, udx, 0), 
                    D(pts.a, ux + i - 1, uy + j - 1, udx, 1));
      md = min(md, dist2(x, y, p));
    }
  }

  return 1 - clamp(sqrt(md) / scale * 0.7, 0.0f, 1.0f);
}

void tmkw2d() { 
  float dx = w / scale;
  float dy = h / scale;
  uint udx = uint(dx + 3);
  uint udy = uint(dy + 3);
  ivec2 texelCoord = ivec2(GI.xy);

  imageStore(img2d, texelCoord, vec4(vec3(gnear2(uint(GI.x + scale + 1), uint(GI.y + scale + 1), udx, udy)), 0.0));
}

void main() {
  if (dimensions == 2) {
    tmkw2d();
  } else {
    tmkw3d();
  }
}

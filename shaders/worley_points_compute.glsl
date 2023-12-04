#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (std430, binding = 1) buffer Pos { float a[]; } a;
uniform float scale;
uniform float seed;
uniform uint udx;
uniform uint udy;
uniform uint dimensions;

float PHI = 1.61803398874989484820459;

float gnoise(vec2 xy) { return fract(sin(distance(vec2(xy.x * PHI + 9.124 * seed, xy.y * 8.5141261 + 2.145 * seed),xy.yx)*seed)*xy.x); }

#define gi gl_GlobalInvocationID 
#define T(A, a, b, c, w, h, o) A[o + ((a) + (b) * (w) + (c) * (w * h)) * 3]
#define D(A, a, b, w, o) A[o + ((a) + (b) * (w)) * 2]
#define S * vec2(5.1, 6.2) * scale

vec2 kmskms(uint t, float o1, float o2) {
  if (t == 1) {
    return gi.xy S * gi.yx S + gi.yz;
  } else if (t == 2 ) {
    return gi.yy S + gi.yz S * gi.xz;
  } else {
    return gi.zz S + gi.yy S * gi.xz;
  }
}

void main() {
  if (dimensions == 2) {
    D(a.a, gi.x, gi.y, udx, 0) = float(gi.x * scale + gnoise(gi.xy * scale + vec2(4.1, 9.1)) * scale);
    D(a.a, gi.x, gi.y, udx, 1) = float(gi.y * scale + gnoise(gi.yx * scale + vec2(4.1, 9.1) + vec2(100000)) * scale);
  } else if (dimensions == 3) {
    T(a.a, gi.x, gi.y, gi.z, udx, udy, 0) = float(gi.x * scale + gnoise(kmskms(1, 4.1, 9.1)) * scale);
    T(a.a, gi.x, gi.y, gi.z, udx, udy, 1) = float(gi.y * scale + gnoise(kmskms(2, 12.1, 31.2)) * scale);
    T(a.a, gi.x, gi.y, gi.z, udx, udy, 2) = float(gi.z * scale + gnoise(kmskms(3, 14.2, 1.1)) * scale);
  }
}

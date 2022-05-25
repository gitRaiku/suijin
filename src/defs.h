#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

struct v2 {
  float x;
  float y;
};
typedef struct v2 v2;
typedef struct v2 vec2;

struct v3 {
  float x;
  float y;
  float z;
};
typedef struct v3 v3;
typedef struct v3 vec3;

struct v4 {
  float x;
  float y;
  float z;
  float w;
};
typedef struct v4 v4;
typedef struct v4 vec4;
typedef struct v4 quat;

typedef float mat3[3][3];
typedef float mat4[4][4];

#endif

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

struct fbuf {
  char fb[2048];
  uint32_t fd;
  uint32_t bufp;
  uint32_t bufl;
  uint8_t flags;
};

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

struct uv4 {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t w;
};
typedef struct uv4 uv4;

struct uv3 {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};
typedef struct uv3 uv3;

typedef float mat3[3][3];
typedef float mat4[4][4];

#endif

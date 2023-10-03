#ifndef NESAKU_H
#define NESAKU_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <glad/glad.h>

#include "util.h"
#include "izanagi.h"

#define G(v, x, y, w) ((v)[(x) + (y) * (w)])
#define D(v, x, y, z, w, h) ((v)[(x) + (y) * (w) + (z) * (w * h)])

extern float pscale;
extern uint8_t updp;

struct fcol {
  float r;
  float g;
  float b;
};

struct rgba {
  float r;
  float g;
  float b;
  float a;
};

struct fcolu {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct i2df {
  float *__restrict v;
  uint32_t h;
  uint32_t w;
};

struct i3df {
  float *__restrict v;
  uint32_t h;
  uint32_t w;
  uint32_t d;
};

struct i2d {
  struct fcol *__restrict v;
  uint32_t h;
  uint32_t w;
};

struct i2du {
  struct fcolu *__restrict v;
  uint32_t h;
  uint32_t w;
};

struct i3d {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct fcol *__restrict v;
};

struct i3da {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct rgba *__restrict v;
};

void dump_image_to_file(char *__restrict fname, struct i2d *__restrict im);

void update_texture(struct i2d *__restrict im, struct texture *__restrict tex);

void update_texture_ub(struct i2du *__restrict im, struct texture *__restrict tex);

void noise_w2d(uint32_t h, uint32_t w, float scale, struct i2df *__restrict im);

void new_perlin_perms();

void noise_p2d(uint32_t h, uint32_t w, uint32_t octaves, float persistence, float scale, struct i2df *__restrict im);

void noise_pw3d(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float pscale, float wscale, struct i3df *__restrict im);

void noise_cloud3(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float pscale, float pwscale, float wscale, struct i3da *__restrict im);

#endif

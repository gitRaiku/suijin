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

extern float pscale;
extern uint8_t updp;

struct fcol {
  float r;
  float g;
  float b;
};

struct i2d {
  struct fcol *__restrict v;
  uint32_t h;
  uint32_t w;
  uint64_t padding;
};

struct i3d {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct fcol *__restrict v;
};

void dump_image_to_file(char *__restrict fname, struct i2d *__restrict im);

void update_texture(struct i2d *__restrict im, struct texture *__restrict tex);

void noise_w2d(uint32_t h, uint32_t w, float scale, struct i2d *__restrict im, uint8_t reset);

void new_perlin_perms();

void noise_p2d(uint32_t h, uint32_t w, uint32_t octaves, float persistence, float scale, struct i2d *__restrict im);

#endif

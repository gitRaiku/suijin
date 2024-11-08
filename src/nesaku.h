#ifndef NESAKU_H
#define NESAKU_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <glad/glad.h>
#include <stdarg.h>
#include <sys/time.h>

#include "util.h"
#include "izanagi.h"
#include "shaders.h"

#define TIME(c, s) {struct timeval _st, _ed; gettimeofday(&_st, NULL); c; gettimeofday(&_ed, NULL); fprintf(stdout, "Time " s ": %li\n", (_ed.tv_sec - _st.tv_sec) * 1000 + (_ed.tv_usec - _st.tv_usec) / 1000);}

#define TOSTR(x) #x
#define TOSTR1(x) TOSTR(X)
#define PCH(c) {if(c){fprintf(stderr, "Error executing %s at %s:%u\n", TOSTR(c), __FILE__, __LINE__);}}

#define G(v, x, y, w) ((v)[(x) + (y) * (w)])
#define D(v, x, y, z, w, h) ((v)[(x) + (y) * (w) + (z) * (w * h)])

extern float pscale;
extern uint8_t updp;
extern uint32_t wComp;

struct img {
  uint32_t w, h, d;
  uint32_t t;
};

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

struct i2da {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct rgba *__restrict v;
};

struct i3da {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct rgba *__restrict v;
};

void dump_image_to_file(char *__restrict fname, struct i2d *__restrict im);

void update_texture(struct i2d *__restrict im, struct texture *__restrict tex);

uint8_t prep_compute_shaders();
void update_texture_ub(struct i2du *__restrict im, struct texture *__restrict tex);

void noise_w(uint32_t w, uint32_t h, uint32_t d, float scale, struct img *__restrict i);

void noise_p(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float scale, struct img *__restrict i);


void noise_pw(uint32_t w, uint32_t h, uint32_t d, uint32_t octaves, float persistence, float pscale, float pwscale, float wscale, struct img *__restrict i);

void noise_ww(uint32_t w, uint32_t h, uint32_t d, float scale, struct img *__restrict i);

void noise_c(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float scale, struct img *__restrict i);

void new_perlin_perms();

struct img create_image24(uint32_t w, uint32_t h);
struct img create_image34(uint32_t w, uint32_t h, uint32_t d);
struct img create_image21(uint32_t w, uint32_t h);
struct img create_image31(uint32_t w, uint32_t h, uint32_t d);
#endif

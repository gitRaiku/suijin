#ifndef NESAKU_H
#define NESAKU_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "defs.h"

#define G(v, x, y, w) ((v)[(x) + (y) * (w)])

struct ucol {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct i2d {
  uint32_t h;
  uint32_t w;
  struct ucol *__restrict v;
};

struct i3d {
  uint32_t h;
  uint32_t w;
  uint32_t d;
  struct ucol *__restrict v;
};

#endif

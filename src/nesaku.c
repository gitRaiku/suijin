#include "nesaku.h"

float __inline__ __attribute((pure)) min(float o1, float o2) {
  return o1 < o2 ? o1 : o2;
}

float rf() {
  return (float)rand() / (float)RAND_MAX;
}

#define P2(x) ((x)*(x))
#define GP(dx, dy) (G(pts, (x + 1) + (dx), (y + 1) + (dy), udy + 2))
#define GD(dx, dy) (P2(GP(dx, dy).x - cx) + P2(GP(dx, dy).y - cy))
float wgd(uint32_t x, uint32_t y, uint32_t w, v2 *__restrict pts, uint32_t udx, uint32_t udy, float scale) {
  float d[8];
  float cx = x * scale;
  float cy = x * scale;
  d[0] = GD(-1, -1);
  d[1] = GD( 0, -1);
  d[2] = GD( 1, -1);
  d[3] = GD(-1,  0);
  d[4] = GD( 1,  0);
  d[5] = GD(-1,  1);
  d[6] = GD( 0,  1);
  d[7] = GD( 1,  1);
  float m = 100000000.0f;
  {
    int32_t i;
    for(i = 0; i < 8; ++i) {
      m = min(m, d[i]);
    }
  }
  return sqrtf(m);
}

struct ucol f2c(float v) {
  struct ucol res;
  res.r = v * 255;
  res.g = v * 255;
  res.b = v * 255;

  return res;
}

#define GI(px, py) G(pts, (px), (py), udy + 2).x = 1000000.0f; G(pts, (px), (py), udy + 2).y = 1000000.0f
struct i2d *__restrict noise_w2d(uint32_t h, uint32_t w, float scale) {
  struct i2d *__restrict im = malloc(sizeof(struct i2d));
  im->h = h;
  im->w = w;
  im->v = malloc(sizeof(im->v[0]) * h * w);

  float dx = w / scale;
  float dy = h / scale;
  uint32_t udx = (uint32_t)dx;
  uint32_t udy = (uint32_t)dy;

  v2 *__restrict pts = malloc(sizeof(v2) * (udx + 2) * (udy + 2));
  GI(0      , 0      );
  GI(udx + 1, 0      );
  GI(0      , udy + 1);
  GI(udx + 1, udy + 1);

  {
    uint32_t i, j;
    for (i = 0; i < udx; ++i) {
      GI(i + 1, 0      );
      GI(i + 1, udy + 1);
    }

    for (i = 0; i < udy; ++i) {
      GI(0      , i + 1);
      GI(udx + 1, i + 1);
    }

    for (i = 0; i < udx; ++i) {
      for (j = 0; j < udy; ++j) {
        G(pts, i + 1, j + 1, udy + 2).x = ((float)i + rf()) * scale;
        G(pts, i + 1, j + 1, udy + 2).y = ((float)j + rf()) * scale;
      }
    }

    for (i = 0; i < w; ++i) {
      for (j = 0; j < h; ++j) {
        G(im->v, i, j, w) = f2c(wgd(i, j, w, pts, udx, udy, scale));
      }
    }
  }

  free(pts);
  
  return im;
}

struct i2d *__restrict noise_p2d(uint32_t octave, uint32_t freq, uint32_t scale, uint32_t h, uint32_t w) {
  return NULL;
}

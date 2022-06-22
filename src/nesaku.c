#include "nesaku.h"

float pscale = 0.001f;

float __inline__ __attribute((pure)) min(float o1, float o2) {
  return o1 < o2 ? o1 : o2;
}

float rf() {
  return (float)rand() / (float)RAND_MAX;
}

struct fcol f2c(float v) {
  struct fcol res;
  res.r = v;
  res.g = v;
  res.b = v;

  return res;
}

struct fcol inv(struct fcol o) {
  struct fcol res;
  res.r = 1.0f - o.r;
  res.g = 1.0f - o.g;
  res.b = 1.0f - o.b;

  return res;
}

void update_texture(struct i2d *__restrict im, struct texture *__restrict tex) {
  tex->w = im->w;
  tex->h = im->h;

  glBindTexture(GL_TEXTURE_2D, tex->i);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->w, tex->h, 0, GL_RGB, GL_FLOAT, im->v);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

#define P2(x) ((x)*(x))
float dist2(float x, float y, v2 v) {
  return P2(v.x - x) + P2(v.y - y);
}

struct fcol gnear(uint32_t x, uint32_t y, float scale, v2 *__restrict pts, uint32_t udx, uint32_t udy) {
  uint32_t ux = (uint32_t) x / scale;
  uint32_t uy = (uint32_t) y / scale;
  float md = INFINITY;

  if (ux != 0) {
    if (uy != 0) {
      md = min(md, dist2(x, y, G(pts, ux - 1, uy - 1, udx)));
    }

      md = min(md, dist2(x, y, G(pts, ux - 1, uy    , udx)));

    if (uy != udy - 1) {
      md = min(md, dist2(x, y, G(pts, ux - 1, uy + 1, udx)));
    }
  }

  if (uy != 0) {
      md = min(md, dist2(x, y, G(pts, ux    , uy - 1, udx)));
  }

      md = min(md, dist2(x, y, G(pts, ux    , uy    , udx)));

  if (uy != udy - 1) {
      md = min(md, dist2(x, y, G(pts, ux    , uy + 1, udx)));
  }
  
  if (ux != udx - 1) {
    if (uy != 0) {
      md = min(md, dist2(x, y, G(pts, ux + 1, uy - 1, udx)));
    }

      md = min(md, dist2(x, y, G(pts, ux + 1, uy    , udx)));

    if (uy != udy - 1) {
      md = min(md, dist2(x, y, G(pts, ux + 1, uy + 1, udx)));
    }
  }

  /// NSCALE 43
  return inv(f2c(clamp(scale * pscale * sqrtf(md), 0.0f, 1.0f)));
}

void noise_w2d(uint32_t h, uint32_t w, float scale, struct i2d *__restrict im) {
  static struct i2d *__restrict tmp;
  if (tmp == NULL) {
    tmp = calloc(sizeof(struct i2d), 1);
    tmp->v = calloc(sizeof(tmp->v[0]), h * w);
  }
  srand(0);

  if (im == NULL) {
    im = malloc(sizeof(struct i2d));
  }

  im->h = h;
  im->w = w;
  if (im->v == NULL) {
    im->v = calloc(sizeof(im->v[0]), h * w);
  }

  float dx = w / scale;
  float dy = h / scale;
  uint32_t udx = (uint32_t)dx + 1;
  uint32_t udy = (uint32_t)dy + 1;

  v2 *__restrict pts = calloc(sizeof(v2), udx * udy);
  
  {
    int32_t i, j;
    for (i = 0; i < udx; ++i) {
      for (j = 0; j < udy; ++j) {
        G(pts, i, j, udx).x = (i + rf()) * scale;
        G(pts, i, j, udx).y = (j + rf()) * scale;
      }
    }
// AAAAAAAA 129 286

    for (i = 0; i < h; ++i) {
      for (j = 0; j < w; ++j) {
        G(im->v, i, j, w) = gnear(i, j, scale, pts, udx, udy);
      }
    }
  }

  memcpy(tmp->v, im->v, sizeof(im->v[0]) * h * w);

  free(pts);
}

struct i2d *__restrict noise_p2d(uint32_t octave, uint32_t freq, uint32_t scale, uint32_t h, uint32_t w) {
  return NULL;
}

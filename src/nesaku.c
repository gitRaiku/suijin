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

struct fcol f2c(float v) {
  struct fcol res;
  res.r = v;
  res.g = v;
  res.b = v;

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

float dist2(float x, float y, v2 v) {
  return P2(v.x - x) + P2(v.y - y);
}

struct fcol gnear(uint32_t x, uint32_t y, float scale, v2 *__restrict pts, uint32_t udx, uint32_t udy) {
  float sx = x / scale;
  float sy = y / scale;
  uint32_t ux = (uint32_t) sx;
  uint32_t uy = (uint32_t) sy;
  float md = 100000.0f;
  if (ux != 0) {
    if (uy != 0) {
      md = min(md, dist2(sx, sy, G(pts, ux - 1, uy - 1, udx)));
    }

      md = min(md, dist2(sx, sy, G(pts, ux - 1, uy    , udx)));

    if (uy != udy) {
      md = min(md, dist2(sx, sy, G(pts, ux - 1, uy + 1, udx)));
    }
  }
  if (uy != 0) {
      md = min(md, dist2(sx, sy, G(pts, ux    , uy - 1, udx)));
  }

      md = min(md, dist2(sx, sy, G(pts, ux    , uy    , udx)));

  if (uy != udy) {
      md = min(md, dist2(sx, sy, G(pts, ux    , uy + 1, udx)));
  }
  if (ux != udx) {
    if (uy != 0) {
      md = min(md, dist2(sx, sy, G(pts, ux + 1, uy - 1, udx)));
    }

      md = min(md, dist2(sx, sy, G(pts, ux + 1, uy    , udx)));

    if (uy != udy) {
      md = min(md, dist2(sx, sy, G(pts, ux + 1, uy + 1, udx)));
    }
  }
  return f2c(sqrtf(md));
}

#define GI(px, py) G(pts, (px), (py), udy + 2).x = 1000000.0f; G(pts, (px), (py), udy + 2).y = 1000000.0f
void noise_w2d(uint32_t h, uint32_t w, float scale, struct i2d *__restrict im) {
  static float ps = 0.0f;
  if (ps == scale) {
    fprintf(stdout, "SKIP UPDATE\r");
    return;
  }
  fprintf(stdout, "NOSK UPDATE\r");
  ps = scale;
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
  uint32_t udx = (uint32_t)dx;
  uint32_t udy = (uint32_t)dy;
  //fprintf(stdout, "Got dx and dy: %3u %3u\r", udx, udy);

  v2 *__restrict pts = calloc(sizeof(v2), udx * udy);
  
  {
    int32_t i, j;
    for (i = 0; i < udx; ++i) {
      for (j = 0; j < udy; ++j) {
        G(pts, i, j, udx).x = i * scale + rf();
        G(pts, i, j, udx).y = j * scale + rf();
      }
    }

    for (i = 0; i < h; ++i) {
      for (j = 0; j < w; ++j) {
        G(im->v, i, j, w) = gnear(i, j, scale, pts, udx, udy);
      }
    }
  }


  free(pts);
}

struct i2d *__restrict noise_p2d(uint32_t octave, uint32_t freq, uint32_t scale, uint32_t h, uint32_t w) {
  return NULL;
}

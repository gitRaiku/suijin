#include "nesaku.h"

float pscale = 0.001f;

float __inline__ __attribute((pure)) min(float o1, float o2) {
  return o1 < o2 ? o1 : o2;
}

float rf() {
  return (float)rand() / (float)RAND_MAX;
}

void dump_image_to_file(char *__restrict fname, struct i2d *__restrict im) {
   FILE *__restrict fp;
   png_structp pp;
   png_infop pi;
   
   fp = fopen(fname, "wb");
   if (fp == NULL) {
     fprintf(stderr, "Could not open %s for writing! %m\n", fname);
     exit(1);
   }

   pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (pp == NULL) {
      fclose(fp);
      fprintf(stderr, "Could not create write struct for %s!\n", fname);
      exit(1);
   }

   pi = png_create_info_struct(pp);
   if (pi == NULL) {
      fclose(fp);
      png_destroy_write_struct(&pp,  NULL);
      fprintf(stderr, "Could not create info struct for %s!\n", fname);
      exit(1);
   }

   if (setjmp(png_jmpbuf(pp))) {
      fclose(fp);
      png_destroy_write_struct(&pp, &pi);
      fprintf(stderr, "Error png write for %s!\n", fname);
      exit(1);
   }

    png_set_IHDR (pp,
                  pi,
                  im->w,
                  im->h,
                  8,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    png_byte **rp = malloc(im->h * sizeof(im->v[0]));
    {
      int32_t i, j;
      for(i = 0; i < im->h; ++i) {
        png_byte *__restrict rw = malloc(sizeof(uint8_t) * 3 * im->h);
        rp[i] = rw;
        for(j = 0; j < im->h; ++j) {
          rw[j * 3 + 0] = (uint8_t)(G(im->v, i, j, im->w).r * 255.0f);
          rw[j * 3 + 1] = (uint8_t)(G(im->v, i, j, im->w).g * 255.0f);
          rw[j * 3 + 2] = (uint8_t)(G(im->v, i, j, im->w).b * 255.0f);
        }
      }
    }


   png_init_io(pp, fp);

   png_set_rows(pp, pi, rp);
   png_write_png(pp, pi, PNG_TRANSFORM_IDENTITY, NULL);

   {
     int32_t i;
     for(i = 0; i < im->h; ++i) {
       free(rp[i]);
     }
     free(rp);
   }

   png_destroy_write_struct(&pp, &pi);

   fclose(fp);

   fprintf(stdout, "Dumped current image to %s!\n", fname);
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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void update_texture_ub(struct i2du *__restrict im, struct texture *__restrict tex) {
  tex->w = im->w;
  tex->h = im->h;

  glBindTexture(GL_TEXTURE_2D, tex->i);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->w, tex->h, 0, GL_RGB, GL_UNSIGNED_BYTE, im->v);
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

void noise_w2d(uint32_t h, uint32_t w, float scale, struct i2d *__restrict im, uint8_t reset) {
  static uint32_t lh = 0;
  static uint32_t lw = 0; 
  static float lsc = 0.0f;
  static float lps = 0.0f;
  static struct i2d *__restrict loim = NULL;
  if (lh == h && lw == w && lsc == scale && loim == im && lps == pscale && reset == 0) {
    return;
  }
  lh = h;
  lw = w;
  lsc = scale;
  loim = im;
  lps = pscale;

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

    for (i = 0; i < h; ++i) {
      for (j = 0; j < w; ++j) {
        G(im->v, i, j, w) = gnear(i, j, scale, pts, udx, udy);
      }
    }
  }

  free(pts);
}

#define PRLINP 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, \
               194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, \
               37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, \
               0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, \
               57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, \
               171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, \
               77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, \
               230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, \
               54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, \
               208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, \
               164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, \
               124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, \
               207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, \
               183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, \
               153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, \
               108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, \
               97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, \
               162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, \
               214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, \
               50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, \
               29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 
uint8_t p[512] = {PRLINP, PRLINP};
uint8_t updp = 0;

void new_perlin_perms() {
  int32_t i;
  for(i = 0; i < 255; ++i) {
    p[i] = i;
  }

  uint8_t r;
  for(i = 0; i < 255; ++i) {
    r = ((float)rand() / (float)RAND_MAX) * 255.0f;
    swap(p + i, p + r);
  }

  for(i = 0; i < 255; ++i) {
    p[i + 255] = p[i];
  }
  updp = 1;
}

float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;

    float v;

    if (h < 4) {
        v = y;
    } else if (h == 12 || h == 14) {
        v = x;
    } else {
        v = z;
    }

    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

float perlin(float x, float y, float z) {
  int32_t xi = (int32_t) x & 255;
  int32_t yi = (int32_t) y & 255;
  int32_t zi = (int32_t) z & 255;
  float xf = x - (int32_t) x;
  float yf = y - (int32_t) y;
  float zf = z - (int32_t) z;
  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  int32_t aaa, aba, aab, abb, baa, bba, bab, bbb;
  aaa = p[p[p[xi] + yi    ] + zi];
  aba = p[p[p[xi] + yi + 1] + zi];
  aab = p[p[p[xi] + yi] + zi + 1];
  abb = p[p[p[xi] + yi + 1] + zi + 1];
  baa = p[p[p[xi + 1] + yi] + zi];
  bba = p[p[p[xi + 1] + yi + 1] + zi];
  bab = p[p[p[xi + 1] + yi] + zi + 1];
  bbb = p[p[p[xi + 1] + yi + 1] + zi + 1];

  float x1, x2, y1, y2;
  x1 = lerp(grad(aaa, xf    , yf    , zf    ),
            grad(baa, xf - 1, yf    , zf    ), u);
  x2 = lerp(grad(aba, xf    , yf - 1, zf    ),
            grad(bba, xf - 1, yf - 1, zf    ), u);
  y1 = lerp(x1, x2, v);

  x1 = lerp(grad(aab, xf    , yf    , zf - 1),
            grad(bab, xf - 1, yf    , zf - 1), u);
  x2 = lerp(grad(abb, xf    , yf - 1, zf - 1),
            grad(bbb, xf - 1, yf - 1, zf - 1), u);
  y2 = lerp(x1, x2, v);

  return (lerp(y1, y2, w) + 1) / 2;
}

float octave_perlin(float x, float y, float z, int octaves, float persistence) {
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;
    for (int i = 0; i < octaves; i++) {
        total += perlin(x * frequency, y * frequency, z * frequency) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2;
    }

    return total / maxValue;
}

void noise_p2d(uint32_t h, uint32_t w, uint32_t octaves, float persistence, float scale, struct i2d *__restrict im) {
  static uint32_t lo = 0   ;
  static float    lp = 0.0f;
  static float    lh = 0.0f;
  static float    lw = 0.0f;
  static float    ls = 0.0f;
  static struct i2d *__restrict li = NULL;

  if (lh == h && lw == w && lo == octaves && lp == persistence && ls == scale && li == im && updp == 0) {
    return;
  }
  updp = 0;
  lo = octaves;
  lp = persistence;
  lh = h;
  lw = w;
  li = im;
  ls = scale;

  if (im == NULL) {
    im = malloc(sizeof(struct i2d));
  }

  im->h = h;
  im->w = w;
  if (im->v == NULL) {
    im->v = calloc(sizeof(im->v[0]), h * w);
  }

  {
    int32_t i, j;
    for (i = 0; i < w; ++i) {
      for (j = 0; j < h; ++j) {
        G(im->v, i, j, w) = f2c(octave_perlin(i / scale, j / scale, 0.0f, octaves, persistence));
      }
    }
  }

}

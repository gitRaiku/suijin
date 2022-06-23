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

struct i2d *__restrict noise_p2d(uint32_t octave, uint32_t freq, uint32_t scale, uint32_t h, uint32_t w) {
  return NULL;
}

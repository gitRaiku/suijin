#include "nesaku.h"

struct rthread threads[THREAD_COUNT];

float rf() { return (float)rand() / (float)RAND_MAX; }

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

uint32_t wpCompS, wpComp, wCompS, wComp, wbuf;
uint8_t prep_compute_shaders() {
  PR_CHECK(shader_get("shaders/worley_points_compute.glsl", GL_COMPUTE_SHADER, &wpCompS))
  PR_CHECK(program_get(1, &wpCompS, &wpComp))
  PR_CHECK(shader_get("shaders/worley_compute.glsl", GL_COMPUTE_SHADER, &wCompS))
  PR_CHECK(program_get(1, &wCompS, &wComp))
  glGenBuffers(1, &wbuf);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wbuf);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 128 * 128 * 128 * 3 * 4, NULL, GL_STATIC_READ);
  return 0;
}

void noise_w(uint32_t w, uint32_t h, uint32_t d, float scale, struct img *__restrict i) {
  if (i == NULL || (i->h != h || i->w != w || i->d != d)) {
    if (i->t) { glDeleteTextures(1, &i->t); }
    if (d == 1) { *i = create_image24(w, h   ); } 
           else { *i = create_image34(w, h, d); }
  }
  uint32_t udx = (uint32_t)(w / scale + 3);
  uint32_t udy = (uint32_t)(h / scale + 3);
  uint32_t udz = (uint32_t)(d / scale + 3);

  uint32_t dimensions = (d == 1) ? 2 : 3;

  glUseProgram(wpComp);
  program_set_uint1 (wpComp, "dimensions", dimensions);
  program_set_float1(wpComp, "seed", 1.0);
  program_set_uint1 (wpComp, "udx", udx);
  program_set_uint1 (wpComp, "udy", udy);
  program_set_float1(wpComp, "scale", scale);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wbuf);
  glDispatchCompute(udx, udy, udz);
  glMemoryBarrier(0);

  glUseProgram(wComp);
  program_set_uint1 (wComp, "dimensions", dimensions);
  program_set_uint1 (wComp, "w", w);
  program_set_uint1 (wComp, "h", h);
  program_set_uint1 (wComp, "d", d);
  program_set_float1(wComp, "scale", scale);
  if (dimensions == 2) { glBindImageTexture(0, i->t, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F); } 
                  else { glBindImageTexture(2, i->t, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F); }
  glDispatchCompute(w, h, d);
  glMemoryBarrier(0);
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

void noise_p2d(uint32_t h, uint32_t w, uint32_t octaves, float persistence, float scale, struct i2df *__restrict im) {
  if (im == NULL) {
    im = calloc(sizeof(*im), 1);
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
        G(im->v, i, j, w) = octave_perlin(i / scale, j / scale, 0.0f, octaves, persistence);
      }
    }
  }
}

void noise_p3d(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float scale, struct i3df *__restrict im) {
  if (im == NULL) {
    im = calloc(sizeof(*im), 1);
  }

  im->h = h;
  im->w = w;
  im->d = d;
  if (im->v == NULL) {
    im->v = calloc(sizeof(im->v[0]), h * w * d);
  }

  {
    int32_t i, j, k;
    for (i = 0; i < w; ++i) {
      for (j = 0; j < h; ++j) {
        for (k = 0; k < d; ++k) {
          D(im->v, i, j, d, w, h) = octave_perlin(i / scale, j / scale, k / scale, octaves, persistence);
        }
      }
    }
  }
}

float pwb(float p, float w) { /// TODO: Make nicer
  return w - w * p;
  //return w * p;
}

void *tmkpw3d(void *vp) {  /*struct rthread *v = (struct rthread*)vp; uint32_t d = v->ez; GA(h, uint32_t); GA(w, uint32_t); GA(imv, float*); GA(pscale, double); GA(octaves, uint32_t); GA(persistence, double); FFF(v->sz, d, 0, h, 0, w, D(imv, x, y, z, w, h) = pwb(octave_perlin(x / pscale, y / pscale, z / pscale, octaves, persistence), D(imv, x, y, z, w, h)););*/ return NULL; }

void noise_pw3d(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float pscale, float wscale, struct i3df *__restrict im) {
  //noise_w3d(h, w, d, wscale, im);
  //init_threads(tmkpw3d, 0, d, h, w, im->v, (double)pscale, octaves, (double)persistence);
}

void *tmkcloud3(void *vp) { 
  /*
  struct rthread *v = (struct rthread*)vp; 
  uint32_t d = v->ez; 
  GA(h, uint32_t); 
  GA(w, uint32_t); 
  GA(imv, struct rgba*); 
  GA(pw, float*); 
  GA(w1, float*); 
  GA(w2, float*); 
  GA(w3, float*); 

  FFF(v->sz, d, 0, h, 0, w, 
          D(imv, x, y, z, h, w).r = D(pw, x, y, z, h, w);
          D(imv, x, y, z, h, w).g = D(w1, x, y, z, h, w);
          D(imv, x, y, z, h, w).b = D(w2, x, y, z, h, w);
          D(imv, x, y, z, h, w).a = D(w3, x, y, z, h, w);
     );
*/
  return NULL;
}

void noise_cloud3(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float pscale, float pwscale, float wscale, struct i3da *__restrict im) {
  if (im == NULL) { im = calloc(sizeof(*im), 1); }

  im->h = h; im->w = w; im->d = d;
  if (im->v == NULL) { im->v = calloc(sizeof(im->v[0]), h * w * d); }

  struct i3df pw = {0}, w1 = {0}, w2 = {0}, w3 = {0} ;
  TIME(noise_pw3d(h, w, d, octaves, persistence, pscale, pwscale, &pw);,"pw")
    /*
  TIME(noise_w3d(h, w, d, wscale / 1.0, &w1);,"w1") 
  TIME(noise_w3d(h, w, d, wscale / 1.6, &w2);,"w2") 
  TIME(noise_w3d(h, w, d, wscale / 2.2, &w3);,"w3")
  AAAAAAAAAA = 1;
  TIME(init_threads(tmkcloud3, 0, d, h, w, im->v, pw.v, w1.v, w2.v, w3.v);,"IT")
  AAAAAAAAAA = 0;*/
  free(pw.v); free(w1.v); free(w2.v); free(w3.v);
}

void *tmkworl3(void *vp) { 
  /*
  struct rthread *v = (struct rthread*)vp; 
  uint32_t d = v->ez; 
  GA(h, uint32_t); 
  GA(w, uint32_t); 
  GA(imv, struct fcol*); 
  GA(w1, float*); 
  GA(w2, float*); 
  GA(w3, float*); 

  FFF(v->sz, d, 0, h, 0, w, 
          D(imv, x, y, z, h, w).r = D(w1, x, y, z, h, w);
          D(imv, x, y, z, h, w).g = D(w2, x, y, z, h, w);
          D(imv, x, y, z, h, w).b = D(w3, x, y, z, h, w);
     );
*/
  return NULL;
}

void noise_worl3(uint32_t h, uint32_t w, uint32_t d, float scale, struct i3d *__restrict im) {
  if (im == NULL) { im = calloc(sizeof(*im), 1); }

  im->h = h; im->w = w; im->d = d;
  if (im->v == NULL) { im->v = calloc(sizeof(im->v[0]), h * w * d); }

  struct i3df w1 = {0}, w2 = {0}, w3 = {0};
  //noise_w3d(h, w, d, scale / 1.0, &w1); noise_w3d(h, w, d, scale / 2.0, &w2); noise_w3d(h, w, d, scale / 3.0, &w3);

  //init_threads(tmkworl3, 0, d, h, w, im->v, w1.v, w2.v, w3.v);
  free(w1.v); free(w2.v); free(w3.v);
}

void noise_curl3(uint32_t h, uint32_t w, uint32_t octaves, float persistence, float scale, struct i2d *__restrict im) { /// TODO: Be more efficient
  if (im == NULL) { im = calloc(sizeof(*im), 1); }
  im->h = h; im->w = w;
  if (im->v == NULL) { im->v = calloc(sizeof(im->v[0]), h * w); }

  {
    int32_t i, j;
    float eps = 0.01;
#define CURLA 3
#define CURLB 0.5
    for (i = 0; i < h; ++i) {
      for (j = 0; j < w; ++j) {
        G(im->v, i, j, w).r = (octave_perlin(i / scale, (j + eps) / scale, 0.0f, octaves, persistence) - octave_perlin(i / scale, (j - eps) / scale, 0.0f, octaves, persistence)) / (2 * eps) * CURLA + CURLB;
        G(im->v, i, j, w).g = (octave_perlin((i + eps) / scale, j / scale, 0.0f, octaves, persistence) - octave_perlin((i - eps) / scale, j / scale, 0.0f, octaves, persistence)) / (2 * eps) * CURLA + CURLB;
        G(im->v, i, j, w).b = (octave_perlin(0.0f, i / scale, (j + eps) / scale, octaves, persistence) - octave_perlin(0.0f, i / scale, j / scale, octaves, persistence)) / (2 * eps) * CURLA + CURLB;
      }
    }
  }
}

#define CIMG(A, B, W, H, D, ...) \
  struct img ci = {0}; \
  glGenTextures(1, &ci.t); \
  glActiveTexture(GL_TEXTURE0); \
  glBindTexture(GL_TEXTURE_ ##A, ci.t); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_MAG_FILTER, GL_LINEAR); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_MIN_FILTER, GL_LINEAR); \
  glTexImage ##A(GL_TEXTURE_ ##A, 0, B ##32F, __VA_ARGS__, 0, B, GL_FLOAT, NULL); \
  glBindImageTexture(0, ci.t, 0, GL_FALSE, 0, GL_READ_WRITE, B ##32F); \
  ci.w = W; ci.h = H; ci.d = D; \
  return ci;

struct img create_image24(uint32_t w, uint32_t h)             { CIMG(2D, GL_RGBA, w, h, 1, w, h); }
struct img create_image34(uint32_t w, uint32_t h, uint32_t d) { CIMG(3D, GL_RGBA, w, h, d, w, h, d); }

//struct img create_image23(uint32_t w, uint32_t h)             { CIMG(2D, GL_RGB, w, h, 1, w, h); }
//struct img create_image33(uint32_t w, uint32_t h, uint32_t d) { CIMG(3D, GL_RGB, w, h, d, w, h, d); }

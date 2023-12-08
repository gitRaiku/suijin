#include "nesaku.h"

struct rthread threads[THREAD_COUNT];

#define thefunny4 \
  if (i == NULL || (i->h != h || i->w != w || i->d != d)) { \
    if (i->t) { glDeleteTextures(1, &i->t); } \
    if (d == 1) { *i = create_image24(w, h   ); }  \
           else { *i = create_image34(w, h, d); } \
  }

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
uint32_t pCompS, pComp, pbuf;
uint32_t pwCompS, pwComp;
uint32_t cCompS, cComp;
uint8_t prep_compute_shaders() {
  PR_CHECK(shader_get("shaders/worley_points_compute.glsl", GL_COMPUTE_SHADER, &wpCompS))
  PR_CHECK(program_get(1, &wpCompS, &wpComp))
  PR_CHECK(shader_get("shaders/worley_compute.glsl", GL_COMPUTE_SHADER, &wCompS))
  PR_CHECK(program_get(1, &wCompS, &wComp))
  glGenBuffers(1, &wbuf);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wbuf);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 128 * 128 * 128 * 3 * 4, NULL, GL_STATIC_READ);

  PR_CHECK(shader_get("shaders/perlin_compute.glsl", GL_COMPUTE_SHADER, &pCompS))
  PR_CHECK(program_get(1, &pCompS, &pComp))
  glGenBuffers(1, &pbuf);

  PR_CHECK(shader_get("shaders/perlinworley_compute.glsl", GL_COMPUTE_SHADER, &pwCompS))
  PR_CHECK(program_get(1, &pwCompS, &pwComp))

  PR_CHECK(shader_get("shaders/curl_compute.glsl", GL_COMPUTE_SHADER, &cCompS))
  PR_CHECK(program_get(1, &cCompS, &cComp))

  return 0;
}

void noise_w(uint32_t w, uint32_t h, uint32_t d, float scale, struct img *__restrict i) {
  thefunny4;
  uint32_t udx = (uint32_t)(w / scale + 3);
  uint32_t udy = (uint32_t)(h / scale + 3);
  uint32_t udz = (uint32_t)(d / scale + 3);

  glUseProgram(wpComp);
  program_set_uint1 (wpComp, "dimensions", (d == 1) ? 2 : 3);
  program_set_float1(wpComp, "seed", 1.0); /// TODO: MAKE RANDOM
  program_set_uint1 (wpComp, "udx", udx);
  program_set_uint1 (wpComp, "udy", udy);
  program_set_float1(wpComp, "scale", scale);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wbuf);
  glDispatchCompute(udx, udy, udz);
  glMemoryBarrier(0);

  glUseProgram(wComp);
  program_set_uint1 (wComp, "w", w);
  program_set_uint1 (wComp, "h", h);
  program_set_uint1 (wComp, "d", d);
  program_set_float1(wComp, "scale", scale);
  if (d == 1) { glBindImageTexture(0, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); } 
         else { glBindImageTexture(2, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); }
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
uint32_t p[512] = {PRLINP, PRLINP};

void new_perlin_perms() {
  int32_t i;
  uint8_t r;
  for(i = 0; i < 255; ++i) { p[i] = i; }
  for(i = 0; i < 255; ++i) { r = rf() * 255.0f; uswap(p + i, p + r); }
  for(i = 0; i < 255; ++i) { p[i + 255] = p[i]; }
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, pbuf);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(p), p, GL_STATIC_DRAW);
}


void noise_p(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float scale, struct img *__restrict i) {
  thefunny4;

  glUseProgram(pComp);
  program_set_uint1 (pComp, "w", w);
  program_set_uint1 (pComp, "h", h);
  program_set_float1(pComp, "scale", scale);
  program_set_float1(pComp, "persistence", persistence);
  program_set_uint1 (pComp, "octaves", octaves);
  if (d == 1) { glBindImageTexture(0, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); } 
         else { glBindImageTexture(2, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); }
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, pbuf);
  glDispatchCompute(w, h, d);
  glMemoryBarrier(0);
}

void noise_c(uint32_t h, uint32_t w, uint32_t d, uint32_t octaves, float persistence, float scale, struct img *__restrict i) {
  thefunny4;
  static struct img pi;
  noise_p(h + 2, w + 2, d + 2, octaves, persistence, scale, &pi);

  glUseProgram(cComp);
  if (d == 1) { glBindImageTexture(0, pi.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F); } 
         else { glBindImageTexture(1, pi.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F); }
  if (d == 1) { glBindImageTexture(2, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); } 
         else { glBindImageTexture(3, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); }
  glDispatchCompute(w, h, d);
  glMemoryBarrier(0);
}

void noise_pw(uint32_t w, uint32_t h, uint32_t d, uint32_t octaves, float persistence, float pscale, float wscale, struct img *__restrict i) {
  thefunny4;

  static struct img w1, w2, w3, w4, p;
  noise_w(w, h, d, wscale, &w1);
  noise_w(w, h, d, wscale / 2, &w2);
  noise_w(w, h, d, wscale / 3, &w3);
  noise_w(w, h, d, pscale, &w4);
  noise_p(w, h, d, octaves, persistence, pscale, &p);

  //i->t = p.t; return;

  glUseProgram(pwComp);
  glBindImageTexture(0, w1.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindImageTexture(1, w2.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindImageTexture(2, w3.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindImageTexture(3, w4.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindImageTexture(4, p.t, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindImageTexture(5, i->t, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glDispatchCompute(w, h, d);
  glMemoryBarrier(0);
}

#define CIMG(A, B, C, X, W, H, D, ...) \
  struct img ci = {0}; \
  glGenTextures(1, &ci.t); \
  glActiveTexture(GL_TEXTURE0); \
  glBindTexture(GL_TEXTURE_ ##A, ci.t); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_MAG_FILTER, GL_LINEAR); \
  glTexParameteri(GL_TEXTURE_ ##A, GL_TEXTURE_MIN_FILTER, GL_LINEAR); \
  glTexImage ##A(GL_TEXTURE_ ##A, 0, B, __VA_ARGS__, 0, C, GL_FLOAT, NULL); \
  glBindImageTexture(0, ci.t, 0, GL_FALSE, 0, GL_READ_WRITE, X); \
  ci.w = W; ci.h = H; ci.d = D; \
  return ci;

struct img create_image24(uint32_t w, uint32_t h)             { CIMG(2D, GL_RGBA32F, GL_RGBA, GL_RGBA32F, w, h, 1, w, h); }
struct img create_image34(uint32_t w, uint32_t h, uint32_t d) { CIMG(3D, GL_RGBA32F, GL_RGBA, GL_RGBA32F, w, h, d, w, h, d); }

//struct img create_image21(uint32_t w, uint32_t h)             { CIMG(2D, GL_RED, GL_RED, GL_R8, w, h, 1, w, h); }
//struct img create_image31(uint32_t w, uint32_t h, uint32_t d) { CIMG(3D, GL_RED, GL_RED, GL_R8, w, h, d, w, h, d); }

//struct img create_image23(uint32_t w, uint32_t h)             { CIMG(2D, GL_RGB, w, h, 1, w, h); }
//struct img create_image33(uint32_t w, uint32_t h, uint32_t d) { CIMG(3D, GL_RGB, w, h, d, w, h, d); }

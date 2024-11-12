#ifndef ENV_H
#define ENV_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <stdint.h>
#include "util.h"
#include "shaders.h"
#include "linalg.h"
#include "izanagi.h"
#include "nesaku.h"

/// Camera
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 13.0f
#define SENS 0.001f

/// Ui
#define UI_COL_BG1 0x181818FF
#define UI_COL_BG2 0x484848A0
#define UI_COL_FG1 0xBA2636A0
#define UI_COL_FG2 0xA232B4FF
#define UI_COL_FG3 0xFFFFFFFF
#define UI_CLICKABLE 0b1

/// Framerate update rate
#define FRAME_UPDATE 10

#define FRAG_PATH "shaders/cloud_frag.glsl"

/// Thermal Subsystem
#define OUTSIDE_TEMP 0.1f
#define DIFF_COEF 0.05f

enum UIT { UIT_CANNOT, UIT_CAN, UIT_NODE, UIT_SLIDERK, UIT_TEXT, UIT_CHECK };
union KMS { uint32_t u32; void *__restrict fp; };

struct uiSelection {
  enum UIT t;
  enum UIT nt;
  union KMS id;
  float dx, dy;
};

struct perlin {
  struct i2df o1;
  struct i2df o2;
  struct i2df *__restrict m;
  struct i2df *__restrict s;
  uint32_t oct;
  float h;
  float w;
  float foct;
  float per;
  float style;
  float sc;
};

struct tbox {
  uint32_t tlen;
  uint32_t tex;
  char text[64];
  uint32_t tsize;
};

struct mtcheck {
  struct tbox text;
  uint32_t slen;
  uint8_t *__restrict val;
};

struct fslider {
  float *__restrict val;
  v2 lims;
};

struct mtslider {
  struct tbox text;
  struct fslider slid;
  uint32_t slen;
  struct tbox val;
};

enum MCHID { MTEXT_BOX, MFSLIDER, MTSLIDER, MCHECKBOX };

struct mchild {
  enum MCHID id;
  void *__restrict c;
  uint8_t flags;
  void (*callback)(void*);
  void *cbp;
  uint32_t height;
  uint32_t pad;
  uint64_t fg;
  uint64_t bg;
};
VECSTRUCT(mch, struct mchild);
VECTOR_SUITE(mch, struct mchv *__restrict, struct mchild)

struct node {
  float px, py;    // Pos
  float sx, sy;    // Size
  uint32_t lp, rp; // Left, right padding
  uint32_t bp, tp; // Bottom, top padding
  mat4 aff;
  struct mchv children;
};

struct sray {
  v3 o; // Orig
  v3 d; // Dir
  v3 i; // 1 / Dir
  uint8_t s[3]; // Sign of dirs
  uint32_t mask;
};

struct skybox {
  v2 sunPos;
  float sunSize;
  struct minf m;
  v3 sunCol;
};

struct cloud {
  struct minf m;
  struct img worl;
  struct img pwor;
  struct img curl;
  v3 pos;
};

struct camera {
  vec3 pos;
  vec3 up;
  quat orientation;
  float fov;
  float ratio;
};

struct win { 
  int windowW, windowH;
  float iwinw, iwinh;
};

struct env { // God help up all
  uint32_t frame;

  GLFWwindow *__restrict window; 
  struct win w;

  FT_Library ftlib;
  FT_Face ftface;

  struct matv mats;
  struct modv mods;
  struct objv objs;

  struct camera cam;
  struct camera initCam;
  uint8_t camUpdate;

  uint32_t textprog; // Draw text


  uint32_t skyprog, skyvao, skyvbo, sbvl, sbt; // Skybox
  struct skybox sb;

  uint32_t pvao, pvbo; /// Points

#define PROGC 7 /// Shadercurslcs
  uint32_t nprog;
  uint32_t shaders[PROGC * 2];

  uint32_t cloudvao, cloudvbo, cloudprog, cbvl; // Clouds
  struct cloud cl;

  uint32_t uvao, uprog, lprog, pprog; // Draw Square

#define MC 2
  struct node nodes[MC];
};

#endif

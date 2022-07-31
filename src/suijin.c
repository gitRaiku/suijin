#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shaders.h"
#include "linalg.h"
#include "izanagi.h"
#include "nesaku.h"

#define GL_CHECK(call, message) if (!(call)) { fputs(message "! Aborting...\n", stderr); glfwTerminate(); exit(1); }
#define FT_CHECK(command, errtext) { uint32_t err = command; if (err) { fprintf(stderr, "%s: %u:[%s]!\n", errtext, err, FT_Error_String(err)); exit(1); } }
#define PR_CHECK(call) if (call) { return 1; }
#define KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

double _ctime, _ltime;
double deltaTime;

FT_Library ftlib;
FT_Face ftface;

int32_t cshading = 0;

struct camera {
  vec3 pos;
  vec3 up;
  quat orientation;
  float fov;
  float ratio;
};

double mouseX, mouseY;
double oldMouseX, oldMouseY;

struct camera initCam;
struct camera cam;

struct matv mats;
struct modv mods;

enum UIT {
  UIT_CANNOT, UIT_CAN, UIT_NODE, UIT_SLIDERK
};

union KMS {
  uint32_t u32;
  float *__restrict fp;
};

struct uiSelection {
  enum UIT t;
  union KMS id;
  int32_t dx, dy;
};

struct uiSelection selui;

uint8_t cameraUpdate = 1;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 13.0f
#define SENS 0.001f

mat4 fn;
int windowW, windowH;
float iwinw, iwinh;

void callback_error(int error, const char *desc) {
    fprintf(stderr, "GLFW error, no %i, desc %s\n", error, desc);
}

void callback_window_should_close(GLFWwindow *window) {
}

void callback_window_resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    initCam.ratio = cam.ratio = (float) width / (float) height;
    cameraUpdate = 1;
    windowW = width;
    windowH = height;
    iwinw = 1.0f / width;
    iwinh = 1.0f / height;
    // fprintf(stdout, "Window Resize To: %ix%i\n", width, height);
}

GLFWwindow *window_init() {
    GLFWwindow *window;
    glfwSetErrorCallback(callback_error);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowW = 1920;
    windowH = 1080;
    iwinw = 1.0f / windowW;
    iwinh = 1.0f / windowH;
    window = glfwCreateWindow(1080, 1920, "suijin", NULL, NULL);
    GL_CHECK(window, "Failed to create the window!");

    glfwMakeContextCurrent(window);

    GL_CHECK(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress), "Failed to initialize GLAD!");

    glViewport(0, 0, 1920, 1080);

    glfwSetFramebufferSizeCallback(window, callback_window_resize);
    glfwSetWindowCloseCallback(window, callback_window_should_close);

#ifdef SUIJIN_DEBUG
    {
        fprintf(stdout, "App compiled on GLFW version %i %i %i\n",
                GLFW_CONTEXT_VERSION_MAJOR,
                GLFW_CONTEXT_VERSION_MINOR,
                GLFW_VERSION_REVISION);
        int glfw_major, glfw_minor, glfw_revision;
        glfwGetVersion(&glfw_major, &glfw_minor, &glfw_revision);
        fprintf(stdout, "With the runtime GLFW version of %i %i %i\n",
                glfw_major,
       1        glfw_minor,
       1        glfw_revision);
    }
#endif

    return window;
}

void create_projection_matrix(mat4 m, float angle, float ratio, float near, float far) {
  memset(m, 0, sizeof(mat4));
  float tanHalfAngle = tanf(angle / 2.0f);
  m[0][0] = 1.0f / (ratio * tanHalfAngle);
  m[1][1] = 1.0f / (tanHalfAngle);
  m[2][2] = -(far + near) / (far - near);
  m[3][2] = -1.0f;
  m[2][3] = -(2.0f * far * near) / (far - near);
}

void create_lookat_matrix(mat4 m) {
  vec3 f = norm(*qv(&cam.orientation));
  vec3 s = norm(cross(cam.up, f));
  vec3 u = cross(f, s);
  //print_vec3(f, "f");
  //print_vec3(f, "f");
  //print_vec3(s, "s");
  //print_vec3(u, "u");
  //print_vec3(cam.pos, "cam.pos");

  memset(m, 0, sizeof(mat4));
  m[0][0] = s.x;
  m[0][1] = s.y;
  m[0][2] = s.z;
         
  m[1][0] = u.x;
  m[1][1] = u.y;
  m[1][2] = u.z;
         
  m[2][0] = f.x;
  m[2][1] = f.y;
  m[2][2] = f.z;
         
  m[0][3] = -dot(s, cam.pos);
  m[1][3] = -dot(u, cam.pos);
  m[2][3] = -dot(f, cam.pos);
  m[3][3] = 1.0f;
  //print_mat(m, "lookat");
}

void update_camera_matrix() {
  mat4 proj, view;
  if (cam.up.x == cam.orientation.x && cam.up.y == cam.orientation.y && cam.up.z == cam.orientation.z) {
    cam.orientation.x = 0.0000000000000001;
  }
  create_projection_matrix(proj, cam.fov, cam.ratio, 0.01f, 1000.0f);
  create_lookat_matrix(view);

  matmul44(fn, proj, view);
}

struct ckq keypresses = {0};
void kbp_callback(GLFWwindow *window, int key, int scancode, int action, int mod) {
  struct keypress kp = {0};
  kp.iskb = 1;
  kp.key = key;
  kp.scancode = scancode;
  kp.action = action;
  kp.mod = mod;
  ckpush(&keypresses, &kp);
  //print_keypress("Got keypress: ", kp);
}

void mbp_callback(GLFWwindow *window, int button, int action, int mod) {
  struct keypress kp = {0};
  kp.iskb = 0;
  kp.key = button;
  kp.action = action;
  kp.mod = mod;
  ckpush(&keypresses, &kp);
  //print_keypress("Got keypress: ", kp);
}

uint32_t CDR, CDRO; /// HANDLE INPUT
uint32_t lp[3];
enum MOVEMEMENTS { CAMERA, ITEMS, UI };
enum MOVEMEMENTS ms = CAMERA;
void reset_ft();
uint32_t curi = 0;
float *vals[10];
float scal[10];
char *nms[10] = { "A", "P2", "P3", "P4", "PERSISTANCE", "HEIGHT", "WIDTH" };
v2 lims[10];
double startY = 0;
uint8_t nwr = 0;
uint8_t dumpTex = 0;
float mxoff, myoff;
void handle_input(GLFWwindow *__restrict window) {
  { // CURSOR
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if ((mouseX != oldMouseX) || (mouseY != oldMouseY)) {
      double xoff = mouseX - oldMouseX;
      double yoff = mouseY - oldMouseY;
      oldMouseX = mouseX;
      oldMouseY = mouseY;
      switch (ms) {
        case CAMERA:
          quat qx = gen_quat(cam.up, -xoff * SENS);
          quat qy = gen_quat(cross(*qv(&cam.orientation), cam.up), yoff * SENS);

          quat qr = qmul(qy, qx);
          quat qt = qmul(qmul(qr, cam.orientation), qconj(qr));
          qt = qnorm(qt);
          //print_quat(cam.orientation, "qt");
          cam.orientation = qt;

          cameraUpdate = 1;
          break;
        case ITEMS:
          if (vals[curi] == NULL) {
            break;
          }
          *vals[curi] += -yoff * scal[curi];
          if (lims[curi].x != lims[curi].y) {
            *vals[curi] = clamp(*vals[curi], lims[curi].x, lims[curi].y);
          }
          fprintf(stdout, "%s -> % 3.3f\r", nms[curi], *vals[curi]);
          switch (curi) {
            case 3:
              reset_ft();
              break;
         }
          break;
        case UI:
          break;
      }
    }
  }

  struct keypress kp;
  while (keypresses.s != keypresses.e) {
    kp = cktop(&keypresses);
    //print_keypress("Processing keypress: ", kp);
    if (kp.iskb) {
      switch(kp.key) {
        case GLFW_KEY_R:
          if (kp.action == 1) {
            new_perlin_perms();
            nwr = 1;
          }
          break;
        case GLFW_KEY_F:
          if (kp.action == 1) {
            dumpTex = 1;
          }
          break;
        case GLFW_KEY_TAB:
          if (kp.action == 1) {
            ms = UI;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPos(window, windowW / 2.0, windowH / 2.0);
          } else if (kp.action == 0) {
            ms = CAMERA;
            selui.t = UIT_CANNOT;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          }
          break;
        default:
          break;
      }
    } else {
      if (kp.action == GLFW_PRESS) {
        switch(kp.key) {
          case GLFW_MOUSE_BUTTON_LEFT:
            if (ms == CAMERA) {
              startY = mouseY;
              ms = ITEMS;
            } else if (ms == UI) {
              selui.t = UIT_CAN;
            }
            break;
          case GLFW_MOUSE_BUTTON_RIGHT:
            ++curi;
            curi %= 10;
            fprintf(stdout, "New curi %u [%s]\n", curi, nms[curi]);
            break;
        }
      } else {
        switch(kp.key) {
          case GLFW_MOUSE_BUTTON_LEFT:
            if (ms == ITEMS) {
              ms = CAMERA;
            } else if (ms == UI) {
              selui.t = UIT_CANNOT;
            }
            break;
        }
      }
    }
  }

  { // KEY_PRESSED
    float cpanSpeed = (PAN_SPEED + KEY_PRESSED(GLFW_KEY_LEFT_SHIFT) * PAN_SPEED) * deltaTime;
    if (KEY_PRESSED(GLFW_KEY_EQUAL) && KEY_PRESSED(GLFW_KEY_LEFT_SHIFT)) {
      cam.fov += cam.fov * ZOOM_SPEED;
      cameraUpdate = 1;
    } else if (KEY_PRESSED(GLFW_KEY_EQUAL)) {
      cam.fov = initCam.fov;
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_MINUS)) {
      cam.fov -= cam.fov * ZOOM_SPEED;
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_A)) {
      cam.pos = v3a(v3m(cpanSpeed, norm(cross(*qv(&cam.orientation), cam.up))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_D)) {
      cam.pos = v3a(v3m(cpanSpeed, v3n(norm(cross(*qv(&cam.orientation), cam.up)))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_SPACE)) {
      cam.pos = v3a(v3m(cpanSpeed, cam.up), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_LEFT_CONTROL)) {
      cam.pos = v3a(v3m(cpanSpeed, v3n(cam.up)), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_W)) {
      cam.pos = v3a(v3m(cpanSpeed, v3n(*qv(&cam.orientation))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_S)) {
      cam.pos = v3a(v3m(cpanSpeed, *qv(&cam.orientation)), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_U)) {
      memcpy(&cam, &initCam, sizeof(cam));
      cameraUpdate = 1;
    }
    if (lp[0]) {
      ++cshading;
      cshading %= 3;
      lp[0] = 0;
    }
  }
}

time_t la; // check_frag_update
uint8_t check_frag_update(uint32_t *__restrict program, uint32_t vert) {
  struct stat s;
  stat("shaders/vf.glsl", &s);
  if (s.st_ctim.tv_sec != la) {
    uint32_t shaders[2];
    shaders[0] = vert;
    if (shader_get("shaders/vf.glsl", GL_FRAGMENT_SHADER, shaders + 1)) {
      return 2;
    } else {
      program_get(2, shaders, program);
      glDeleteShader(shaders[1]);
    }
    la = s.st_ctim.tv_sec;
    return 1;
  }
  return 0;
}

void update_mat(uint32_t program, struct material *__restrict mat) {
  program_set_float3v(program, "mat.ambient", mat->ambient);
  program_set_float3v(program, "mat.diffuse", mat->diffuse);
  program_set_float3v(program, "mat.spec", mat->spec);
  program_set_float3v(program, "mat.emmisive", mat->emmisive);
  program_set_float1(program, "mat.spece", mat->spece);
  program_set_float1(program, "mat.transp", mat->transp);
  program_set_float1(program, "mat.optd", mat->optd);
  program_set_int1(program, "mat.illum", mat->illum);
}

void draw_model(uint32_t program, struct matv *__restrict m, struct model *__restrict mod, mat4 aff) {
  uint32_t li = 0;
  struct material *__restrict cmat;

  glBindVertexArray(mod->vao);

  program_set_mat4(program, "affine", aff);

  cmat = &m->v[mod->m.v[0].m];
  update_mat(program, cmat);

  {
    int32_t i;
    for(i = 1; i < mod->m.l; ++i) {
      if (cmat->tamb.i != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmat->tamb.i);
        program_set_int1(program, "tex", 1);
        program_set_int1(program, "hasTexture", 1);
      } else {
        program_set_int1(program, "hasTexture", 0);
      }
      
      glDrawArrays(GL_TRIANGLES, li / 8, (mod->m.v[i].i - li) / 8);
      li = mod->m.v[i].i;

      cmat = &m->v[mod->m.v[i].m];
      update_mat(program, cmat);
    }
  }

  if (cmat->tamb.i != 0) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, cmat->tamb.i);
    program_set_int1(program, "tex", 3);
    program_set_int1(program, "hasTexture", 1);
  } else {
    program_set_int1(program, "hasTexture", 0);
  }

  glDrawArrays(GL_TRIANGLES, li / 8, (mod->v.l - li) / 8);
}

struct tbox {
  char text[64];
  float scale;
  uint32_t bp;
};

struct fslider {
  float *__restrict val;
  v2 lims;
  uint32_t bp;
};

enum MCHID {
  MTEXT_BOX, MFSLIDER
};

struct mchild {
  enum MCHID id;
  void *__restrict c;
};

VECSTRUCT(mch, struct mchild);
VECTOR_SUITE(mch, struct mchv *__restrict, struct mchild)

#define MC 2
struct node {
  float px, py;    // Pos
  float sx, sy;    // Size
  uint32_t lp, rp; // Left, right padding
  uint32_t bp, tp; // Bottom, top padding
  mat4 aff;
  struct mchv children;
};
struct node nodes[MC];

void add_title(struct node *__restrict m, char *__restrict t, float sc) {
  struct mchild mc;
  mc.id = MTEXT_BOX;
  mc.c = malloc(sizeof(struct tbox));
#define tmc ((struct tbox *)mc.c)
  strncpy(tmc->text, t, sizeof(tmc->text));
  tmc->scale = sc;
  tmc->bp = (uint32_t) 36.0f * sc;
#undef tmc

  mchvp(&m->children, mc);
}

void add_slider(struct node *__restrict m, float *__restrict v, float lb, float ub) {
  struct mchild mc;
  mc.id = MFSLIDER;
  mc.c = malloc(sizeof(struct fslider));
#define tmc ((struct fslider *)mc.c)
  tmc->val = v;
  tmc->lims = (v2) {lb, ub};
  tmc->bp = 36;
#undef tmc

  mchvp(&m->children, mc);
}

void identity_matrix(mat4 m) {
  m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f; 
  m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f; 
  m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f; 
  m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f; 
}

void create_affine_matrix(mat4 m, float xs, float ys, float zs, float x, float y, float z) {
  m[0][0] = xs * iwinw; m[0][1] =          0; m[0][2] =  0; m[0][3] = x; 
  m[1][0] =          0; m[1][1] = ys * iwinh; m[1][2] =  0; m[1][3] = y; 
  m[2][0] =          0; m[2][1] =          0; m[2][2] = zs; m[2][3] = z; 
  m[3][0] =          0; m[3][1] =          0; m[3][2] =  0; m[3][3] = 1; 
}

uint32_t uvao, uprog; // Draw Square
void draw_squaret(float px, float py, float sx, float sy, uint32_t tex) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);
  float cpx = ((px + sx / 2) * iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  if (tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    program_set_int1(uprog, "tex", 0);
    program_set_int1(uprog, "hasTexture", 1);
  } else {
    program_set_int1(uprog, "hasTexture", 0);
  }

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw_squarec(float px, float py, float sx, float sy, uint32_t col) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);
  float cpx = ((px + sx / 2) * iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  program_set_int1(uprog, "hasTexture", 0);
  program_set_float4(uprog, "col", ((col & 0xFF000000) >> 24) / 255.0f, 
                                   ((col & 0x00FF0000) >> 16) / 255.0f,
                                   ((col & 0x0000FF00) >>  8) / 255.0f, 
                                   ((col & 0x000000FF) >>  0) / 255.0f);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


struct TG {
  uint32_t index;
  FT_Vector pos;
  FT_Glyph image;
};

void update_bw_tex(uint32_t *__restrict tex, uint32_t h, uint32_t w, void *__restrict buf) {
  glBindTexture(GL_TEXTURE_2D, *tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, buf);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

uint8_t runel(char *__restrict str) {
  char *__restrict os = str;
  for (++str; (*str & 0xc0) == 0x80; ++str);
  return (uint8_t) (str - os);
}

uint32_t utf8_to_unicode(char *__restrict str, uint32_t l) {
  uint32_t res = 0;
  switch (l) {
    case 4:
      res |= *str & 0x7;
      break;
    case 3:
      res |= *str & 0xF;
      break;
    case 2:
      res |= *str & 0x1F;
      break;
    case 1:
      res |= *str & 0x7F;
      break;
  }

  --l;
  while (l) {
    ++str;
    res <<= 6;
    res |= *str & 0x3F;
    --l;
  }

  return res;
}

uint32_t draw_textbox(float x, uint32_t y, struct tbox *__restrict t) {
  uint32_t px = 0;
  FT_UInt gi;
  FT_GlyphSlot slot = ftface->glyph;

  {
    char *__restrict ct = t->text;
    uint32_t ctex;
    uint8_t cl;
    FT_ULong cchar;
    glGenTextures(1, &ctex);
    while (*ct) {
      cl = runel(ct);
      cchar = utf8_to_unicode(ct, cl);
      gi = FT_Get_Char_Index(ftface, cchar);

      FT_CHECK(FT_Load_Glyph(ftface, gi, FT_LOAD_DEFAULT), "Could not load glyph");
      FT_Render_Glyph(ftface->glyph, FT_RENDER_MODE_NORMAL);

      update_bw_tex(&ctex, slot->bitmap.rows, slot->bitmap.width, slot->bitmap.buffer);

      int32_t advance = ftface->glyph->metrics.horiAdvance >> 6;
      int32_t fw = ftface->glyph->metrics.width >> 6;
      int32_t fh = ftface->glyph->metrics.height >> 6;
      int32_t xoff = ftface->glyph->metrics.horiBearingX >> 6;
      int32_t yoff = (ftface->bbox.yMax - ftface->glyph->metrics.horiBearingY) >> 6;

      draw_squaret(x + (px + xoff) * t->scale, y + yoff * t->scale, fw * t->scale, fh * t->scale, ctex);

      px += advance;

      ct += cl;
    }
  }

  return t->bp;
}

uint32_t sS = 21;

uint32_t draw_slider(float x, uint32_t y, struct node *__restrict cm, struct fslider *__restrict t) {
  int32_t tlen = cm->sx - cm->lp - cm->rp - sS;
  float   rlen = t->lims.y - t->lims.x;
  int32_t xoff = (*t->val - t->lims.x) / rlen * tlen;
  draw_squarec(x + sS / 2, y + sS * 0.375f, cm->sx - cm->lp - cm->rp - sS, sS / 4, 0xC2C2C2FF); // Line
  draw_squarec(x + xoff, y, sS, sS, 0x323232FF);


  if (selui.t == UIT_CAN &&
      (0 <= mouseX - x - xoff && mouseX - x - xoff <= sS) &&
      (0 <= mouseY - y && mouseY - y <= sS)) { 
    selui.t = UIT_SLIDERK;
    selui.id.fp = t->val;
    selui.dx = mouseX;
    selui.dy = *t->val;
  }

  if (selui.t == UIT_SLIDERK && selui.id.fp == t->val) {
    *t->val = clamp(selui.dy + (mouseX - selui.dx) / tlen * rlen, t->lims.x, t->lims.y);
  }

  return t->bp;
}

void draw_node(uint32_t mi) {
#define cm nodes[mi]
  draw_squarec(cm.px, cm.py, cm.sx, cm.sy, 0x181818FF);

  {
    int32_t i;
    uint32_t cy = cm.tp;
    for(i = 0; i < cm.children.l; ++i) {
      switch (cm.children.v[i].id) {
        case MTEXT_BOX:
          cy += draw_textbox(cm.px + cm.lp, cm.py + cy, cm.children.v[i].c) + 11;
          break;
        case MFSLIDER:
          cy += draw_slider(cm.px + cm.lp, cm.py + cy, &cm, cm.children.v[i].c) + 11;
          break;
      }
    }
  }

  if (selui.t == UIT_CAN &&
      (0 <= mouseX - cm.px && mouseX - cm.px <= cm.sx) &&
      (0 <= mouseY - cm.py && mouseY - cm.py <= cm.sy)) { 
    selui.t = UIT_NODE;
    selui.id.u32 = mi;
    selui.dx = cm.px - mouseX;
    selui.dy = cm.py - mouseY;
  }

  if (selui.t == UIT_NODE && selui.id.u32 == mi) {
    cm.px = mouseX + selui.dx;
    cm.py = mouseY + selui.dy;
  }
#undef cm
}

uint32_t prep_ui(GLFWwindow *__restrict window) {
  uint32_t uvbo, uvao;
  float vdata[] = {
    -1.0f,  1.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 0.0f,
  };

  glGenVertexArrays(1, &uvao);
  glGenBuffers(1, &uvbo);

  glBindVertexArray(uvao);
  glBindBuffer(GL_ARRAY_BUFFER, uvbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata), vdata, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  return uvao;
}

void reset_ft() {
  if (ftlib != NULL) {
    FT_CHECK(FT_Done_FreeType(ftlib), "Could not destroy the freetype lib");
  }
  FT_CHECK(FT_Init_FreeType(&ftlib), "Could not initialize the freetype lib");

  FT_CHECK(FT_New_Face(ftlib, "Resources/Fonts/Koruri/Koruri-Regular.ttf", 0, &ftface), "Could not load the font");
  // FT_CHECK(FT_New_Face(ftlib, "Resources/Fonts/JetBrains/jetbrainsmono.ttf", 0, &ftface), "Could not load the font");
  FT_CHECK(FT_Set_Pixel_Sizes(ftface, 0, (int32_t)72), "Could not set pixel sizes");
  FT_CHECK(FT_Select_Charmap(ftface, FT_ENCODING_UNICODE), "Could not select char map");
}

uint8_t run_suijin() {
  init_random();
  setbuf(stdout, NULL);
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();

  glfwSetKeyCallback(window, kbp_callback);
  glfwSetMouseButtonCallback(window, mbp_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPointSize(10.0);
  glLineWidth(3.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  struct objv objs;

  { /// Asset loading
    modvi(&mods);
    matvi(&mats);

    parse_folder(&mods, &mats, "Resources/Items/Mountain");
    parse_folder(&mods, &mats, "Resources/Items/Dough");
    //parse_folder(&mods, &uim, "Resources/Items/Plane");
    //
    /*
        -10.109322 10.019073 - -1.889740 3.262799 - -10.138491 10.031691
         -0.849398  2.147456 -  0.000000 1.475801 -   0.000000  3.154107
     */
    
    fprintf(stdout, "%f %f - %f %f - %f %f\n", mods.v[0].exx.x, mods.v[0].exx.y, 
                                               mods.v[0].exy.x, mods.v[0].exy.y, 
                                               mods.v[0].exz.x, mods.v[0].exz.y);

    fprintf(stdout, "%f %f - %f %f - %f %f\n", mods.v[1].exx.x, mods.v[1].exx.y, 
                                               mods.v[1].exy.x, mods.v[1].exy.y, 
                                               mods.v[1].exz.x, mods.v[1].exz.y);

    struct model cm;
    init_model(&cm);
    strncpy(cm.name, "bb1", 64);

    floatvp(&cm.v, mods.v[0].exx.x);
    floatvp(&cm.v, mods.v[0].exy.x);
    floatvp(&cm.v, mods.v[0].exz.x);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 

    floatvp(&cm.v, mods.v[0].exx.y);
    floatvp(&cm.v, mods.v[0].exy.y);
    floatvp(&cm.v, mods.v[0].exz.y);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 

    floatvp(&cm.v, mods.v[0].exx.x);
    floatvp(&cm.v, mods.v[0].exy.y);
    floatvp(&cm.v, mods.v[0].exz.y);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 
    mativp(&cm.m, (struct mate) { 1, 0 });

    create_vao(cm.v.v, cm.v.l, &cm.vbo, &cm.vao);
    modvp(&mods, cm);

    init_model(&cm);
    strncpy(cm.name, "bb2", 64);

    floatvp(&cm.v, mods.v[1].exx.x);
    floatvp(&cm.v, mods.v[1].exy.x);
    floatvp(&cm.v, mods.v[1].exz.x);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 

    floatvp(&cm.v, mods.v[1].exx.y);
    floatvp(&cm.v, mods.v[1].exy.y);
    floatvp(&cm.v, mods.v[1].exz.y);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 

    floatvp(&cm.v, mods.v[1].exx.x);
    floatvp(&cm.v, mods.v[1].exy.y);
    floatvp(&cm.v, mods.v[1].exz.y);
    floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); floatvp(&cm.v, 1.0f); 
    mativp(&cm.m, (struct mate) { 1, 0 });

    create_vao(cm.v.v, cm.v.l, &cm.vbo, &cm.vao);
    modvp(&mods, cm);
    
    matvt(&mats);
    modvt(&mods);
  }

  {
    struct object cobj;
    struct minf cm;

    objvi(&objs);
    init_object(&cobj, "MIAN");
    cm.scale = 10.0f;
    cm.m = 0;
    cm.pos = (v3) {0.0f, 0.0f, 0.0f};
    maff(&cm);
    minfvp(&cobj.mins, cm);

    cm.m = 3;
    minfvp(&cobj.mins, cm);
    objvp(&objs, cobj);

    init_object(&cobj, "DOUGH");
    cm.m = 1;
    cm.scale = 10.0f;
    cm.pos = (v3) {-60.0f, 40.0f, -30.0f};
    maff(&cm);
    minfvp(&cobj.mins, cm);

    cm.m = 2;
    minfvp(&cobj.mins, cm);

    cm.m = 4;
    minfvp(&cobj.mins, cm);
    objvp(&objs, cobj);

    minfvt(&cobj.mins);
    objvt(&objs);
  }

  uint32_t nprog;

  _ctime = _ltime = deltaTime = 0;

#define SHADERC 4
  uint32_t shaders[SHADERC];
  {
    char          *paths[SHADERC] = { "shaders/vv.glsl", "shaders/vf.glsl", 
                                            "shaders/uv.glsl", "shaders/uf.glsl"};
    uint32_t shaderTypes[SHADERC] = { GL_VERTEX_SHADER   , GL_FRAGMENT_SHADER,
                                            GL_VERTEX_SHADER   , GL_FRAGMENT_SHADER  };
    uint32_t i;
    for (i = 0; i < SHADERC; ++i) {
      PR_CHECK(shader_get(paths[i], shaderTypes[i], shaders + i));
    }

    PR_CHECK(program_get(2, shaders    , &nprog));
    PR_CHECK(program_get(2, shaders + 2, &uprog));

    for (i = 0; i < SHADERC; ++i) { // TODO: REMOVE
      glDeleteShader(shaders[i]);
    }
  }

  uvao = prep_ui(window);


  { // STAT
    struct stat s;
    stat("shaders/vf.glsl", &s);
    la = s.st_ctim.tv_sec;
  }

  memset(&initCam, 0, sizeof(initCam));

  {
    int iw, ih;
    glfwGetWindowSize(window, &iw, &ih);
    initCam.ratio = (float)iw / (float)ih;
  }

  initCam.up.y = 1.0f;
  initCam.fov = toRadians(90.0f);
  /*initCam.pos = (vec3) { 0.0f, 0.0f, 0.0f };
  initCam.orientation = (quat) { 1.0f, 0.0f, 0.0f, 0.0f};*/
  initCam.pos = (vec3) { 0.0f, 60.0f, 0.0f };
  initCam.orientation = (quat) { 1.0f, 0.0f, 0.0f, 0.0f};

  memcpy(&cam, &initCam, sizeof(cam));

  glfwGetCursorPos(window, &mouseX, &mouseY);
  oldMouseX = 0.0;
  oldMouseY = 0.0;

  uint32_t frame = 0;
  reset_ft();

  /*
  mchvi(&nodes[0].children);
  add_title(&nodes[0], "Numarul de boli pe care le are steifen.", 0.20f);
  add_slider(&nodes[0], &nodes[0].sy, 0.0f, 700.0f);
  nodes[0].px = 50.0f;
  nodes[0].py = 50.0f;
  nodes[0].sx = 300.0f;
  nodes[0].sy = 500.0f;
  nodes[0].bp = 20;
  nodes[0].tp = 20;
  nodes[0].lp = 20;
  nodes[0].rp = 20;
  mchvt(&nodes[0].children);
  */

  while (!glfwWindowShouldClose(window)) {
    ++frame;
    _ctime = glfwGetTime();
    deltaTime = _ctime - _ltime;

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    handle_input(window);

    if (cameraUpdate) {
      update_camera_matrix();
      cameraUpdate = 0;
    }

    { // NPROG
      glUseProgram(nprog);

      program_set_mat4(nprog, "fn_mat", fn);
      program_set_int1(nprog, "shading", cshading);
      program_set_float3v(nprog, "camPos", cam.pos);
      int32_t i, j;
      for(i = 0; i < objs.l; ++i) {
        for(j = 0; j < objs.v[i].mins.l; ++j) {
          draw_model(nprog, &mats, mods.v + objs.v[i].mins.v[j].m, objs.v[i].mins.v[j].aff);
        }
      }
    }

    { // UPROG
      glUseProgram(uprog);
      glDisable(GL_DEPTH_TEST);

      int32_t i;
      for(i = 0; i < MC; ++i) {
        draw_node(i);
      }
    }

    if (frame % 30 == 0) {
      while (1) {
        uint8_t r = check_frag_update(&nprog, shaders[0]);
        if (r < 2) {
          break;
        } else {
          sleep(1);
        }
      }
    }

    glfwPollEvents();
    glfwSwapBuffers(window);
    _ltime = _ctime;
  }

  /*{
    int32_t i;
    for(i = 0; i < mods.l; ++i) {
      destroy_model(mods.v + i);
    }
    free(mods.v);
    free(mats.v);
  }*/

  glfwTerminate();

  return 0;
}

int main(int argc, char **argv) {
  GL_CHECK(run_suijin() == 0, "Running failed!");
  
  return 0;
}

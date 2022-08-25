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

#define FPC(v) ((float *__restrict)(v))
#define V3C(v) ((v3 *__restrict)(v))
#define V4C(v) ((v4 *__restrict)(v))

double _ctime, _ltime;
double deltaTime;

uint8_t drawObjs = 1;
uint8_t drawUi = 0;
uint8_t drawBb = 0;

mat4 identity;

FT_Library ftlib;
FT_Face ftface;

/// TODO
v3 HITP;
uint8_t HITS = 0;

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

#define FRAG_PATH "shaders/skybox_frag.glsl"

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
                glfw_minor,
                glfw_revision);
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
  vec3 f = norm(QV(cam.orientation));
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
  create_projection_matrix(proj, cam.fov, cam.ratio, 0.01f, 3000.0f);
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
uint8_t canMoveCam = 1;
float mxoff, myoff;
void handle_input(GLFWwindow *__restrict window) {
  { // CURSOR
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if ((mouseX != oldMouseX) || (mouseY != oldMouseY)) {
      double xoff = mouseX - oldMouseX;
      double yoff = mouseY - oldMouseY;
      oldMouseX = mouseX;
      oldMouseY = mouseY;

      if (canMoveCam) {
        quat qx = gen_quat(cam.up, -xoff * SENS);
        quat qy = gen_quat(cross(QV(cam.orientation), cam.up), yoff * SENS);

        quat qr = qmul(qy, qx);
        quat qt = qmul(qmul(qr, cam.orientation), qconj(qr));
        qt = qnorm(qt);
        cam.orientation = qt;

        cameraUpdate = 1;
      }

      switch (ms) {
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
        default:
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
            canMoveCam = 0;
            drawUi = 1;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPos(window, windowW / 2.0, windowH / 2.0);
          } else if (kp.action == 0) {
            ms = CAMERA;
            canMoveCam = 1;
            drawUi = 0;
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
    float cpanSpeed = (PAN_SPEED + KEY_PRESSED(GLFW_KEY_LEFT_SHIFT) * PAN_SPEED * 3) * deltaTime;
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
      cam.pos = v3a(v3m(cpanSpeed, norm(cross(QV(cam.orientation), cam.up))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_D)) {
      cam.pos = v3a(v3m(cpanSpeed, v3n(norm(cross(QV(cam.orientation), cam.up)))), cam.pos);
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
      cam.pos = v3a(v3m(cpanSpeed, v3n(QV(cam.orientation))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_S)) {
      cam.pos = v3a(v3m(cpanSpeed, QV(cam.orientation)), cam.pos);
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
  stat(FRAG_PATH, &s);
  if (s.st_ctim.tv_sec != la) {
    uint32_t shaders[2];
    shaders[0] = vert;
    if (shader_get(FRAG_PATH, GL_FRAGMENT_SHADER, shaders + 1)) {
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

  glBindVertexArray(mod->vao); /// TODO: Combine aff and fn on the cpu

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

uint32_t uvao, uprog, lprog, pprog; // Draw Square
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

v4 __attribute((pure)) h2c4(uint64_t c) { // INLINE
  return (v4) { ((c & 0xFF000000) >> 24) / 255.0f,
                ((c & 0x00FF0000) >> 16) / 255.0f, 
                ((c & 0x0000FF00) >>  8) / 255.0f, 
                ((c & 0x000000FF) >>  0) / 255.0f };
}

v3 __attribute((pure)) h2c3(uint64_t c) { // INLINE
  return (v3) { ((c & 0x00FF0000) >> 16) / 255.0f,
                ((c & 0x0000FF00) >>  8) / 255.0f, 
                ((c & 0x000000FF) >>  0) / 255.0f };
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
  v4 c = h2c4(col);
  program_set_float4(uprog, "col", c.x, c.y, c.z, c.w);

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

void prep_ui(GLFWwindow *__restrict window, uint32_t *__restrict vao) {
  uint32_t vbo;
  float vdata[] = {
    -1.0f,  1.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 0.0f,
  };

  glGenVertexArrays(1, vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata), vdata, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
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

struct sray {
  v3 o; // Orig
  v3 d; // Dir
  v3 i; // 1 / Dir
  uint8_t s[3]; // Sign of dirs
};

struct sray csray(v3 s, v3 e) {
  struct sray cs;

  cs.o = s;
  cs.d = norm(v3s(e, s));
  cs.i = v3i(cs.d);

  cs.s[0] = cs.i.x < 0; 
  cs.s[1] = cs.i.y < 0; 
  cs.s[2] = cs.i.z < 0; 

  return cs;
}

struct sray csrayd(v3 s, v3 d) {
  struct sray cs;

  cs.o = s;
  cs.d = norm(d);
  cs.i = v3i(cs.d);

  cs.s[0] = cs.i.x < 0; 
  cs.s[1] = cs.i.y < 0; 
  cs.s[2] = cs.i.z < 0; 

  return cs;
}

uint8_t rbi(struct sray *__restrict r, struct bbox *__restrict cb) {
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

     tmin = (V3C(cb)[    r->s[0]].x - r->o.x) * r->i.x; 
     tmax = (V3C(cb)[1 - r->s[0]].x - r->o.x) * r->i.x;
    tymin = (V3C(cb)[    r->s[1]].y - r->o.y) * r->i.y;
    tymax = (V3C(cb)[1 - r->s[1]].y - r->o.y) * r->i.y;
 
    if ((tmin > tymax) || (tymin > tmax)) {
        return 0;
    }
 
    if (tymin > tmin) {
        tmin = tymin;
    }

    if (tymax < tmax) {
        tmax = tymax;
    }
 
    tzmin = (V3C(cb)[    r->s[2]].z - r->o.z) * r->i.z; 
    tzmax = (V3C(cb)[1 - r->s[2]].z - r->o.z) * r->i.z; 
 
    if ((tmin > tzmax) || (tzmin > tmax)) {
        return 0; 
    }
 
    /* 
    if (tzmin > tmin) {
        tmin = tzmin;
    }

    if (tzmax < tmax) {
        tmax = tzmax;
    }
    */
 
    return 1; 
}

struct linv lins;
float *__restrict linsv;

/// ADD BB
#define CHEA(i) (FPC(&cl.e)[i] = FPC(&cb->a)[i])
#define CHSA(i) (FPC(&cl.s)[i] = FPC(&cb->a)[i])
void addbb(struct bbox *__restrict cb, struct linv *__restrict v) {
  struct line cl;
  int32_t i;
  cl.c = (v4) { 0.2f, 0.5f, 0.5f, 1.0f };

  cl.s = cb->i;
  for(i = 0; i < 3; ++i) {
    cl.e = cb->i;
    FPC(&cl.e)[i] = FPC(&cb->a)[i];
    linvp(&lins, cl);
  }

  cl.s = cb->a;
  for(i = 0; i < 3; ++i) {
    cl.e = cb->a;
    FPC(&cl.e)[i] = FPC(&cb->i)[i];
    linvp(&lins, cl);
  }

  cl.s = cl.e = cb->i;
  CHSA(0); CHEA(0); CHEA(1);
  linvp(&lins, cl);

  cl.s = cl.e = cb->i;
  CHSA(0); CHEA(0); CHEA(2);
  linvp(&lins, cl);

  cl.s = cl.e = cb->i;
  CHSA(1); CHEA(1); CHEA(0);
  linvp(&lins, cl);

  cl.s = cl.e = cb->i;
  CHSA(1); CHEA(1); CHEA(2);
  linvp(&lins, cl);

  cl.s = cl.e = cb->i;
  CHSA(2); CHEA(2); CHEA(0);
  linvp(&lins, cl);

  cl.s = cl.e = cb->i;
  CHSA(2); CHEA(2); CHEA(1);
  linvp(&lins, cl);
}

void aobj(uint32_t m, float sc, v3 pos, struct object *__restrict cb) {
  struct minf cm;
  cm.m = m;
  cm.scale = sc;
  cm.pos = pos;
  maff(&cm);
  cm.b.i = v3m4(cm.aff, mods.v[cm.m].b.i);
  cm.b.a = v3m4(cm.aff, mods.v[cm.m].b.a);
  minfvp(&cb->mins, cm);
  addbb(&cm.b, &lins);
}

void clines(uint32_t *__restrict vao, uint32_t *__restrict vbo) {
  linsv = malloc(lins.l * sizeof(float) * (3 + 4) * 2);
  {
    int32_t i, j;
    for(i = 0; i < lins.l; ++i) {
      for(j = 0; j < 3; ++j) {
        linsv[14 * i + j] = FPC(&lins.v[i].s)[j];
      }
      *V4C(linsv + 14 * i + 3) = lins.v[i].c;

      for(j = 0; j < 3; ++j) {
        linsv[14 * i + 7 + j] = FPC(&lins.v[i].e)[j];
      }
      *V4C(linsv + 14 * i + 10) = lins.v[i].c;
    }
  }

  glGenVertexArrays(1, vao);
  glGenBuffers(1, vbo);

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);
  glBufferData(GL_ARRAY_BUFFER, lins.l * sizeof(float) * (3 + 4) * 2, linsv, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) (0 * sizeof(float)));
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) (3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

struct vv3p {
  v3 *__restrict v0;
  v3 *__restrict v1;
  v3 *__restrict v2;
};

uint8_t rmif(struct sray *__restrict r, struct vv3p *__restrict p, float *__restrict t, float *__restrict u, float *__restrict v) { 
    v3 v0v1 = v3s(*p->v1, *p->v0);
    v3 v0v2 = v3s(*p->v2, *p->v0);
    v3 pvec = cross(r->d, v0v2);
    float det = dot(v0v1, pvec);

#ifdef CULLING 
    if (det < 0.00000000001) {
      return 0; 
    }
#else 
    if (fabs(det) < 0.00000000001) {
      return 0;
    }
#endif 
    float invDet = 1 / det; 
 
    v3 tvec = v3s(r->o, *p->v0);
    *u = dot(tvec, pvec) * invDet; 
    if (*u < 0 || *u > 1) {
      return 0;
    }
 
    v3 qvec = cross(tvec, v0v1); 
    *v = dot(r->d, qvec) * invDet; 
    if (*v < 0 || *u + *v > 1) {
      return 0;
    }
 
    *t = dot(v0v2, qvec) * invDet; 
 
    return 1; 
} 

#define cmm mods.v[cm->m]
void rmi(struct sray *__restrict r, struct minf *__restrict cm, float *__restrict f) {
  struct vv3p vv;
  float t = 0;
  float u = 0;
  float v = 0;

  float mf = -99999999999.0f;
  uint32_t ht = 0;

  v3 px;
  v3 py;
  v3 pz;
  {
    int32_t i;
    for(i = 0; i < cmm.v.l / 24; ++i) {
      px = v3a(cm->pos, v3m(cm->scale, *V3C(cmm.v.v + i * 24 +  0)));
      py = v3a(cm->pos, v3m(cm->scale, *V3C(cmm.v.v + i * 24 +  8)));
      pz = v3a(cm->pos, v3m(cm->scale, *V3C(cmm.v.v + i * 24 + 16)));
      vv.v0 = &px;
      vv.v1 = &py;
      vv.v2 = &pz;
      if (rmif(r, &vv, &t, &u, &v)) {
        ++ht;
        if (t > mf && t <= 0) {
          mf = t;
        }
      }
    }
  }

  *f = mf;
}
#undef cmm

void cpoints(uint32_t *__restrict vao, uint32_t *__restrict vbo) {
  glGenVertexArrays(1, vao);
  glGenBuffers(1, vbo);

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);
  glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) (0 * sizeof(float)));
  glEnableVertexAttribArray(0);
}

struct skybox {
  float sunPos;
  float sunSize;
  struct minf m;
  v3 sunCol;
};
uint32_t skyprog, skyvao, skyvbo, sbvl;

void init_skybox(struct skybox *__restrict sb) {
  memset(sb, 0, sizeof(*sb));
  sb->sunPos = 0.25f;
  sb->sunSize = 1.0f;
  sb->sunCol = h2c3(0xFFCC33);
  sb->m.scale = 400.0f;
  sb->m.scale = 100.0f;
  maff(&sb->m);

  glGenVertexArrays(1, &skyvao);
  glGenBuffers(1, &skyvbo);

  /* TODO: Use GL_TRIANGLE_STRIP
    float sbv[] = { 
      -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
       1.0f,  1.0f,  1.0f, 0.50f, 0.00f,
      -1.0f, -1.0f,  1.0f, 0.25f, 0.33f,
       1.0f, -1.0f,  1.0f, 0.25f, 0.00f,
       1.0f, -1.0f, -1.0f, 0.25f, 1.00f,
       1.0f,  1.0f,  1.0f, 0.75f, 0.33f,
       1.0f,  1.0f, -1.0f, 0.75f, 0.66f,
      -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
      -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,
      -1.0f, -1.0f,  1.0f, 0.25f, 0.33f,
      -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
       1.0f, -1.0f, -1.0f, 0.00f, 0.66f,
      -1.0f,  1.0f, -1.0f, 0.00f, 0.33f,
       1.0f,  1.0f, -1.0f, 0.50f, 1.00f
    };
  */

  float sbv[] = {
    -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
    -1.0f, -1.0f,  1.0f, 0.25f, 0.33f,
    -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,

    -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
    -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
    -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,

    -1.0f, -1.0f,  1.0f, 0.25f, 0.33f,
    -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
     1.0f, -1.0f,  1.0f, 0.25f, 0.00f,

    -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
     1.0f, -1.0f,  1.0f, 0.25f, 0.00f,
     1.0f,  1.0f,  1.0f, 0.50f, 0.00f,

    -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
    -1.0f, -1.0f,  1.0f, 0.25f, 0.33f,
     1.0f, -1.0f,  1.0f, 0.00f, 0.33f,

    -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
     1.0f, -1.0f,  1.0f, 0.00f, 0.33f,
     1.0f, -1.0f, -1.0f, 0.00f, 0.66f,

    -1.0f, -1.0f, -1.0f, 0.25f, 0.66f,
    -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,
     1.0f, -1.0f, -1.0f, 0.25f, 1.00f,

    -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,
     1.0f, -1.0f, -1.0f, 0.25f, 1.00f,
     1.0f,  1.0f, -1.0f, 0.50f, 1.00f,

    -1.0f,  1.0f,  1.0f, 0.50f, 0.33f,
    -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,
     1.0f,  1.0f,  1.0f, 0.75f, 0.33f,

    -1.0f,  1.0f, -1.0f, 0.50f, 0.66f,
     1.0f,  1.0f,  1.0f, 0.75f, 0.33f,
     1.0f,  1.0f, -1.0f, 0.75f, 0.66f,

     1.0f,  1.0f, -1.0f, 0.75f, 0.66f,
     1.0f,  1.0f,  1.0f, 0.75f, 0.33f,
     1.0f, -1.0f,  1.0f, 1.00f, 0.33f,

     1.0f,  1.0f, -1.0f, 0.75f, 0.66f,
     1.0f, -1.0f,  1.0f, 1.00f, 0.33f,
     1.0f, -1.0f, -1.0f, 1.00f, 0.66f
  };

  sbvl = sizeof(sbv) / sizeof(float) / 5;

  glBindVertexArray(skyvao);
  glBindBuffer(GL_ARRAY_BUFFER, skyvbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sbv), sbv, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

uint8_t run_suijin() {
  init_random();
  setbuf(stdout, NULL);
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();

  glfwSetKeyCallback(window, kbp_callback);
  glfwSetMouseButtonCallback(window, mbp_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  identity_matrix(identity);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPointSize(6.0);
  glLineWidth(3.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_PROGRAM_POINT_SIZE);
  //glEnable(GL_CULL_FACE);

  struct objv objs;

  { /// Asset loading
    modvi(&mods);
    matvi(&mats);
    linvi(&lins);

    parse_folder(&mods, &mats, "Resources/Items/Mountain");
    //parse_folder(&mods, &mats, "Resources/Items/Dough");
    //parse_folder(&mods, &mats, "Resources/Items/Plant");
    //parse_folder(&mods, &uim, "Resources/Items/Plane");
    
    matvt(&mats);
    modvt(&mods);
  }

  uint32_t lvao, lvbo;
  {
    struct object cb;
    objvi(&objs);

    init_object(&cb, "MIAN");
    aobj(0, 10.0f, (v3) {0.0f, 0.0f, 0.0f}, &cb);
    objvp(&objs, cb);

    /*
    init_object(&cb, "DOUGH");
    aobj(1, 10.0f, (v3) {-60.0f, 40.0f, -30.0f}, &cb);
    aobj(2, 10.0f, (v3) {-60.0f, 40.0f, -30.0f}, &cb);
    objvp(&objs, cb);

    init_object(&cb, "PLANT");
    aobj(3, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    aobj(4, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    aobj(5, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    objvp(&objs, cb);
    */

    minfvt(&cb.mins);
    objvt(&objs);

    clines(&lvao, &lvbo);
  }


  uint32_t nprog;

  _ctime = _ltime = deltaTime = 0;

#define PROGC 5
  uint32_t shaders[PROGC * 2];
  {
    char *snames[PROGC] = { "obj", "ui", "line", "point", "skybox" };
    char tmpn[128];

    uint32_t i;
    for (i = 0; i < PROGC; ++i) {
      snprintf(tmpn, sizeof(tmpn), "shaders/%s_vert.glsl", snames[i]);
      PR_CHECK(shader_get(tmpn, GL_VERTEX_SHADER  , shaders + 2 * i + 0));
      snprintf(tmpn, sizeof(tmpn), "shaders/%s_frag.glsl", snames[i]);
      PR_CHECK(shader_get(tmpn, GL_FRAGMENT_SHADER, shaders + 2 * i + 1));
    }

    PR_CHECK(program_get(2, shaders    , &nprog));
    PR_CHECK(program_get(2, shaders + 2, &uprog));
    PR_CHECK(program_get(2, shaders + 4, &lprog));
    PR_CHECK(program_get(2, shaders + 6, &pprog));
    PR_CHECK(program_get(2, shaders + 8, &skyprog));

    /*for (i = 0; i < PROGC; ++i) { // TODO: REMOVE
      glDeleteShader(shaders[i * 2]);
      glDeleteShader(shaders[i * 2 + 1]);
    }*/
  }

  uint32_t pvao, pvbo;
  {
    cpoints(&pvao, &pvbo);

    v3 ce = {0.0f, 0.0f, 0.0f};

    glBindBuffer(GL_ARRAY_BUFFER, pvbo);
    glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float), 3 * sizeof(float), &ce);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  prep_ui(window, &uvao);


  { // STAT
    struct stat s;
    stat(FRAG_PATH, &s);
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
  initCam.pos = (vec3) { 0.0f, 60.0f, 0.0f };
  initCam.orientation = (quat) { 1.0f, 0.0f, 0.0f, 0.0f};

  memcpy(&cam, &initCam, sizeof(cam));

  glfwGetCursorPos(window, &mouseX, &mouseY);
  oldMouseX = 0.0;
  oldMouseY = 0.0;

  glLineWidth(10.0f);

  uint32_t frame = 0;
  reset_ft();

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

  struct sray cs;


  struct skybox sb;

  init_skybox(&sb);

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

    {
      glUseProgram(skyprog);
      program_set_mat4(skyprog, "fn", fn);
      program_set_mat4(skyprog, "affine", sb.m.aff);

      glBindVertexArray(skyvao);
      glDrawArrays(GL_TRIANGLES, 0, sbvl);
    }

    goto end;










































    if (drawObjs) { // NPROG
      glUseProgram(nprog);

      program_set_mat4(nprog, "fn_mat", fn);
      program_set_int1(nprog, "shading", cshading);
      program_set_float3v(nprog, "camPos", cam.pos);

      int32_t i, j;
      float rd, mrd = -99999999.0f;

      if (canMoveCam && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        cs = csrayd(cam.pos, QV(cam.orientation));
      }

      for(i = 0; i < objs.l; ++i) {
        for(j = 0; j < objs.v[i].mins.l; ++j) {
          draw_model(nprog, &mats, mods.v + objs.v[i].mins.v[j].m, objs.v[i].mins.v[j].aff);
          if (canMoveCam && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (rbi(&cs, &objs.v[i].mins.v[j].b)) {
              rmi(&cs, &objs.v[i].mins.v[j], &rd);
              if (rd > mrd && rd > -9999999999.0f) {
                mrd = rd;
                HITS = 1;
                HITP = v3a(cam.pos, v3m(rd, QV(cam.orientation)));
                glBindBuffer(GL_ARRAY_BUFFER, pvbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(float), &HITP);
              }
            }
          }
        }
      }
    }

    if (drawBb) { // LPROG
      glUseProgram(lprog);
      program_set_mat4(lprog, "fn_mat", fn);

      glBindVertexArray(lvao);
      glDrawArrays(GL_LINES, 0, lins.l * 2);
    }

    { // PPROG
      glDisable(GL_DEPTH_TEST);
      glUseProgram(pprog);
      glBindVertexArray(pvao);
      program_set_float4(pprog, "col", 1.0f, 1.0f, 1.0f, 0.1f);
      program_set_float1(pprog, "ps", 7.0f);
      glDrawArrays(GL_POINTS, 1, 1);

      if (HITS) {
        program_set_mat4(pprog, "fn_mat", fn);
        program_set_float4(pprog, "col", 1.0f, 1.0f, 1.0f, 1.0f);
        program_set_float1(pprog, "ps", 0.0f);
        glDrawArrays(GL_POINTS, 0, 1);
      }
    }


    if (drawUi) { // UPROG
      glUseProgram(uprog);
      glDisable(GL_DEPTH_TEST);

      int32_t i;
      for(i = 0; i < MC; ++i) {
        draw_node(i);
      }

    }

end:;
    if (frame % 30 == 0) {
      while (!glfwWindowShouldClose(window)) {
        uint8_t r = check_frag_update(&skyprog, 9);
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

  free(linsv);

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

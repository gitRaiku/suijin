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

#define EPS 0.0000000001

float DELETE_ME;

#define OUTSIDE_TEMP 0.1f
float DIFF_COEF = 0.05f;

double _ctime, _ltime;
double deltaTime;

uint8_t drawObjs = 1;
uint8_t drawTerm = 0;
uint8_t drawClouds = 1;
uint8_t drawUi = 1;
uint8_t drawBb = 0;

struct objv objs;

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
  float dx, dy;
};
struct uiSelection selui;

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
struct perlin per;
uint8_t perlinR = 0;

uint8_t cameraUpdate = 1;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 13.0f
#define SENS 0.001f

#define UI_COL_BG1 0x181818FF
#define UI_COL_BG2 0x484848A0
#define UI_COL_FG1 0xBA2636A0
#define UI_COL_FG2 0xA232B4FF
#define UI_COL_FG3 0xFFFFFFFF

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

/// HANDLE INPUT
uint32_t CDR, CDRO;
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
        case GLFW_KEY_I:
          if (kp.action == 1) {
            new_perlin_perms();
            perlinR = 1;
          }
          break;
        case GLFW_KEY_R:
          perlinR = 1;
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
        case GLFW_KEY_O:
          if (kp.action == 1) {
            ++cshading;
            cshading %= 3;
            lp[0] = 0;
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
    if (KEY_PRESSED(GLFW_KEY_Q)) {
      glfwSetWindowShouldClose(window, 1);
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
  uint32_t tsize;
  uint32_t bp;
};

struct fslider {
  float *__restrict val;
  v2 lims;
  uint32_t bp;
  void (*callback)(void*);
  void *cbp;
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

struct node {
  float px, py;    // Pos
  float sx, sy;    // Size
  uint32_t lp, rp; // Left, right padding
  uint32_t bp, tp; // Bottom, top padding
  mat4 aff;
  struct mchv children;
};
#define MC 2
struct node nodes[MC];

void add_title(struct node *__restrict m, char *__restrict t, uint32_t tsize) { /// Should be 31
  struct mchild mc;
  mc.id = MTEXT_BOX;
  mc.c = malloc(sizeof(struct tbox));
#define tmc ((struct tbox *)mc.c)
  strncpy(tmc->text, t, sizeof(tmc->text));
  tmc->tsize = tsize * 64;
  FT_CHECK(FT_Set_Char_Size(ftface, 0, tmc->tsize, 0, 88), "");
  //tmc->bp = ((double)(ftface->bbox.yMax - ftface->bbox.yMin) / (double)ftface->units_per_EM) / 64 * tsize;
  tmc->bp = ftface->size->metrics.y_ppem;
#undef tmc

  mchvp(&m->children, mc);
}

void add_slider(struct node *__restrict m, float *__restrict v, float lb, float ub, void (*callback)(void*), void *p) {
  struct mchild mc;
  mc.id = MFSLIDER;
  mc.c = malloc(sizeof(struct fslider));
#define tmc ((struct fslider *)mc.c)
  tmc->val = v;
  tmc->lims = (v2) {lb, ub};
  tmc->bp = 36;
  tmc->callback = callback;
  tmc->cbp = p;
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
void draw_squaret(float px, float py, float sx, float sy, uint32_t tex, int32_t type) {
  glUseProgram(uprog);
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
    program_set_int1(uprog, "type", type);
  } else {
    program_set_int1(uprog, "type", 0);
  }

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

v4 gcor(v4 in, float gamma) {
  return (v4) { pow(in.x, gamma), pow(in.y, gamma), pow(in.z, gamma), in.w };
}

uint32_t textprog; // Draw text
void draw_squaretext(float px, float py, float sx, float sy, uint32_t tex, uint64_t bgcol, uint64_t fgcol) {
  mat4 aff;
  float cpx = ((px + sx / 2) * iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(textprog, "affine", aff);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, tex);
  program_set_int1(textprog, "tex", 1);
  v4 bg = gcor(h2c4(bgcol), 1 / 1.8);
  program_set_float4(textprog, "bgcol", bg.x, bg.y, bg.z, bg.w);
  v4 fg = gcor(h2c4(fgcol), 1 / 1.8);
  program_set_float4(textprog, "fgcol", fg.x, fg.y, fg.z, fg.w);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw_squaret3(float px, float py, float sx, float sy, uint32_t tex, int32_t type, float z) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);
  float cpx = ((px + sx / 2) * iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_3D, tex);
  program_set_int1(uprog, "tex3", 1);
  program_set_int1(uprog, "type", type);
  program_set_float1(uprog, "z", z);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);
  glUseProgram(0);
}

void draw_squarec(float px, float py, float sx, float sy, uint32_t col) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);
  float cpx = ((px + sx / 2) * iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  program_set_int1(uprog, "type", 0);
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

void update_rgb_tex(uint32_t *__restrict tex, uint32_t h, uint32_t w, void *__restrict buf) {
  glBindTexture(GL_TEXTURE_2D, *tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

float TEXT_SIZE = 20.0;
uint32_t draw_textbox(float x, uint32_t y, struct tbox *__restrict t) {
  uint32_t px = 0;
  FT_UInt gi;
  FT_GlyphSlot slot = ftface->glyph;

  glUseProgram(textprog);
  glBindVertexArray(uvao);
  FT_CHECK(FT_Set_Char_Size(ftface, 0, t->tsize, 0, 88),"Could not set the character size!");

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

      FT_CHECK(FT_Load_Glyph(ftface, gi, FT_LOAD_DEFAULT | FT_LOAD_COLOR | FT_LOAD_FORCE_AUTOHINT), "Could not load glyph");
      FT_Render_Glyph(ftface->glyph, FT_RENDER_MODE_NORMAL);
      update_bw_tex(&ctex, slot->bitmap.rows, slot->bitmap.width, slot->bitmap.buffer);

      int32_t advance = ftface->glyph->metrics.horiAdvance >> 6;
      int32_t fw = ftface->glyph->metrics.width >> 6;
      int32_t fh = ftface->glyph->metrics.height >> 6;
      int32_t xoff = ftface->glyph->metrics.horiBearingX >> 6;
      int32_t yoff = (-ftface->glyph->metrics.horiBearingY >> 6) + t->bp;

      //draw_squaretext(x + (px + xoff) * t->scale, y + yoff * t->scale, fw * t->scale, fh * t->scale, ctex, UI_COL_BG1, UI_COL_FG2);
      draw_squaretext(x + px + xoff, y + yoff, fw, fh, ctex, UI_COL_BG1, UI_COL_FG3);

      px += advance;

      ct += cl;
    }
    glDeleteTextures(1, &ctex);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  return t->bp;
}

uint32_t sS = 21; // Draw slider
uint32_t draw_slider(float x, uint32_t y, struct node *__restrict cm, struct fslider *__restrict t) {
  int32_t tlen = cm->sx - cm->lp - cm->rp - sS;
  float   rlen = t->lims.y - t->lims.x;
  int32_t xoff = (*t->val - t->lims.x) / rlen * tlen;
  draw_squarec(x + sS / 2, y + sS * 0.375f, cm->sx - cm->lp - cm->rp - sS, sS / 4, UI_COL_BG2); // Line
  draw_squarec(x + xoff, y, sS, sS, UI_COL_FG1);


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
    fprintf(stdout, "%f\n", *t->val);
    if (t->callback != NULL && mouseX != selui.dx) {
      t->callback(t->cbp);
    }
  }

  return t->bp;
}

void draw_node(uint32_t mi) {
#define cm nodes[mi]
  draw_squarec(cm.px, cm.py, cm.sx, cm.sy, UI_COL_BG1);

  {
    int32_t i;
    uint32_t cy = cm.tp;
    for(i = 0; i < cm.children.l; ++i) {
      switch (cm.children.v[i].id) {
        case MTEXT_BOX:
          cy += draw_textbox(cm.px + cm.lp, cm.py + cy, cm.children.v[i].c);
          break;
        case MFSLIDER:
          cy += draw_slider(cm.px + cm.lp, cm.py + cy, &cm, cm.children.v[i].c);
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
  FT_CHECK(FT_Set_Char_Size(ftface, 0, 16 * 64, 300, 300),"Could not set the character size!");
  FT_CHECK(FT_Set_Pixel_Sizes(ftface, 0, (int32_t)72), "Could not set pixel sizes");
  FT_CHECK(FT_Select_Charmap(ftface, FT_ENCODING_UNICODE), "Could not select char map");
}

struct sray {
  v3 o; // Orig
  v3 d; // Dir
  v3 i; // 1 / Dir
  uint8_t s[3]; // Sign of dirs
  uint32_t mask;
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

struct sray csrayd(v3 s, v3 d, uint32_t mask) {
  struct sray cs;

  cs.o = s;
  cs.d = norm(d);
  cs.i = v3i(cs.d);

  cs.s[0] = cs.i.x < 0; 
  cs.s[1] = cs.i.y < 0; 
  cs.s[2] = cs.i.z < 0; 

  cs.mask = mask;

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

struct linv lins; /// Bounding box
float *__restrict linsv;
void addbb(struct bbox *__restrict cb, struct linv *__restrict v) {
#define CHEA(i) (FPC(&cl.e)[i] = FPC(&cb->a)[i])
#define CHSA(i) (FPC(&cl.s)[i] = FPC(&cb->a)[i])
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
#undef CHEA
#undef CHSA
}

void aobj(uint32_t m, v3 sc, v3 pos, struct object *__restrict cb, uint32_t mask) {
  struct minf cm;
  cm.m = m;
  cm.scale = sc;
  cm.pos = pos;
  cm.rot = (vec4) {0.0f, 0.0f, 0.0f, 0.5f};
  maff(&cm);
  cm.b.i = v3m4(cm.aff, mods.v[cm.m].b.i);
  cm.b.a = v3m4(cm.aff, mods.v[cm.m].b.a);
  cm.mask = mask;
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

void rmi(struct sray *__restrict r, struct minf *__restrict cm, float *__restrict f) {
#define cmm mods.v[cm->m]
#define NEG_INF -99999999999.0f
  struct vv3p vv;
  float t = 0;
  float u = 0;
  float v = 0;

  float mf = NEG_INF;
  uint32_t ht = 0;

  v3 px;
  v3 py;
  v3 pz;
  {
    int32_t i;
    for(i = 0; i < cmm.v.l / 24; ++i) {
      px = v3a(cm->pos, v3m(cm->scale.x, *V3C(cmm.v.v + i * 24 +  0)));
      py = v3a(cm->pos, v3m(cm->scale.y, *V3C(cmm.v.v + i * 24 +  8)));
      pz = v3a(cm->pos, v3m(cm->scale.z, *V3C(cmm.v.v + i * 24 + 16)));
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
#undef cmm
}

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
  v2 sunPos;
  float sunSize;
  struct minf m;
  v3 sunCol;
};

uint32_t skyprog, skyvao, skyvbo, sbvl;
void init_skybox(struct skybox *__restrict sb) {
  memset(sb, 0, sizeof(*sb));
  sb->sunSize = 1.0f;
  sb->sunCol = h2c3(0xFFCC33);
  sb->m.scale.x = sb->m.scale.y = sb->m.scale.z = 400.0f;
  quat qr = gen_quat((vec3) { 1.0f, 0.0f, 0.0f }, M_PI / 4);
  sb->m.rot = qnorm(qmul(qmul(qr, (quat) {0.0f, 1.0f, 0.0f, 0.0f} ), qconj(qr)));
  sb->m.mask = MSKYBOX;
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

#define UP333 (1.0/3.0)
#define UP666 (2.0/3.0)

  float sbv[] = {
    -1.0f, -1.0f, -1.0f, 0.25f, UP666,
    -1.0f,  1.0f,  1.0f, 0.50f, UP333,
    -1.0f, -1.0f,  1.0f, 0.25f, UP333,

    -1.0f, -1.0f, -1.0f, 0.25f, UP666,
    -1.0f,  1.0f, -1.0f, 0.50f, UP666,
    -1.0f,  1.0f,  1.0f, 0.50f, UP333,

    -1.0f, -1.0f,  1.0f, 0.25f, UP333,
    -1.0f,  1.0f,  1.0f, 0.50f, UP333,
     1.0f, -1.0f,  1.0f, 0.25f, 0.00f,

    -1.0f,  1.0f,  1.0f, 0.50f, UP333,
     1.0f,  1.0f,  1.0f, 0.50f, 0.00f,
     1.0f, -1.0f,  1.0f, 0.25f, 0.00f,

    -1.0f, -1.0f, -1.0f, 0.25f, UP666,
    -1.0f, -1.0f,  1.0f, 0.25f, UP333,
     1.0f, -1.0f,  1.0f, 0.00f, UP333,

    -1.0f, -1.0f, -1.0f, 0.25f, UP666,
     1.0f, -1.0f,  1.0f, 0.00f, UP333,
     1.0f, -1.0f, -1.0f, 0.00f, UP666,

    -1.0f, -1.0f, -1.0f, 0.25f, UP666,
     1.0f, -1.0f, -1.0f, 0.25f, 1.00f,
    -1.0f,  1.0f, -1.0f, 0.50f, UP666,

    -1.0f,  1.0f, -1.0f, 0.50f, UP666,
     1.0f, -1.0f, -1.0f, 0.25f, 1.00f,
     1.0f,  1.0f, -1.0f, 0.50f, 1.00f,

    -1.0f,  1.0f,  1.0f, 0.50f, UP333,
    -1.0f,  1.0f, -1.0f, 0.50f, UP666,
     1.0f,  1.0f,  1.0f, 0.75f, UP333,

    -1.0f,  1.0f, -1.0f, 0.50f, UP666,
     1.0f,  1.0f, -1.0f, 0.75f, UP666,
     1.0f,  1.0f,  1.0f, 0.75f, UP333,

     1.0f,  1.0f, -1.0f, 0.75f, UP666,
     1.0f, -1.0f,  1.0f, 1.00f, UP333,
     1.0f,  1.0f,  1.0f, 0.75f, UP333,

     1.0f,  1.0f, -1.0f, 0.75f, UP666,
     1.0f, -1.0f, -1.0f, 1.00f, UP666,
     1.0f, -1.0f,  1.0f, 1.00f, UP333
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

uint32_t ttex = 0;
void init_therm(void *ignored) {
  if (ttex == 0) {
    glGenTextures(1, &ttex);
  }
  if ((uint32_t)per.h != per.o1.h || (uint32_t)per.w != per.o1.w) { /// TODO: Free and realloc doesn't work with glTexImage2D
    //if ((uint32_t)per.h * (uint32_t)per.w < per.o1.h * per.o1.w) {
      per.o1.v = realloc(per.o1.v, sizeof(per.o1.v[0]) * (uint32_t)per.h * (uint32_t)per.w);
      per.o2.v = realloc(per.o2.v, sizeof(per.o2.v[0]) * (uint32_t)per.h * (uint32_t)per.w);
    /*} else {
      free(per.o1.v);
      free(per.o2.v);
    }*/
  }
  per.m = &per.o1;
  per.s = &per.o2;
  per.oct = (uint32_t)per.foct;

  if ((uint32_t)per.style == 0) {
    noise_p2d((uint32_t)per.h, (uint32_t)per.w, per.oct, per.per, per.sc, per.m);
  } else {
    noise_w2d((uint32_t)per.h, (uint32_t)per.w, per.sc, per.m);
  }

  if (per.o2.v == NULL) {
    per.o2.v = calloc(sizeof(per.o2.v[0]), (uint32_t)per.h * (uint32_t)per.w);
  }
  per.s->h = per.m->h;
  per.s->w = per.m->w;

  glBindTexture(GL_TEXTURE_2D, ttex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (uint32_t)per.w, (uint32_t)per.h, 0, GL_RED, GL_FLOAT, per.m->v);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void upd_therm() {
  if (perlinR == 1) {
    perlinR = 0;
    init_therm(NULL);
    return;
  }
  uint32_t h, w;
  h = per.m->h;
  w = per.m->w;

  int32_t i, j;
  memset(per.s->v, 0, sizeof(float) * h * w);
  float x;
  for (i = 1; i < h - 1; ++i) {
    for (j = 1; j < w - 1; ++j) {
      x = G(per.m->v, i, j, w) * DIFF_COEF;
      G(per.s->v, i, j, w) += G(per.m->v, i, j, w) - x * 4;
      G(per.s->v, i + 1, j    , w) += x;
      G(per.s->v, i - 1, j    , w) += x;
      G(per.s->v, i    , j + 1, w) += x;
      G(per.s->v, i    , j - 1, w) += x;
    }
  }

  for (i = 1; i < w - 1; ++i) {
    x = G(per.m->v, i, 0, w) * DIFF_COEF;
    G(per.s->v, i    , 0, w) += G(per.m->v, i, 0, w) - x * 3;
    G(per.s->v, i    , 1, w) += x;
    G(per.s->v, i + 1, 0, w) += x;
    G(per.s->v, i - 1, 0, w) += x;
  }
  for (i = 1; i < w - 1; ++i) {
    x = G(per.m->v, i, h - 1, w) * DIFF_COEF;
    G(per.s->v, i    , h - 1, w) += G(per.m->v, i, h - 1, w) - x * 3;
    G(per.s->v, i    , h - 2, w) += x;
    G(per.s->v, i + 1, h - 1, w) += x;
    G(per.s->v, i - 1, h - 1, w) += x;
  }
  for (i = 1; i < h - 1; ++i) {
    x = G(per.m->v, i, 0, w) * DIFF_COEF;
    G(per.s->v, 0, i    , w) += G(per.m->v, i, 0, w) - x * 3;
    G(per.s->v, 1, i    , w) += x;
    G(per.s->v, 0, i + 1, w) += x;
    G(per.s->v, 0, i - 1, w) += x;
  }
  for (i = 1; i < h - 1; ++i) {
    x = G(per.m->v, i, w - 1, w) * DIFF_COEF;
    G(per.s->v, w - 1, i    , w) += G(per.m->v, i, w - 1, w) - x * 3;
    G(per.s->v, w - 2, i    , w) += x;
    G(per.s->v, w - 1, i + 1, w) += x;
    G(per.s->v, w - 1, i - 1, w) += x;
  }
  x = G(per.m->v, 0, 0, w) * DIFF_COEF;
  G(per.s->v, 0, 0, w) += G(per.m->v, 0, 0, w) - x * 2;
  G(per.s->v, 0, 1, w) += x;
  G(per.s->v, 1, 0, w) += x;

  x = G(per.m->v, w - 1, 0, w) * DIFF_COEF;
  G(per.s->v, w - 1, 0, w) += G(per.m->v, w - 1, 0, w) - x * 2;
  G(per.s->v, w - 1, 1, w) += x;
  G(per.s->v, w - 2, 0, w) += x;

  x = G(per.m->v, w - 1, h - 1, w) * DIFF_COEF;
  G(per.s->v, w - 1, h - 1, w) += G(per.m->v, w - 1, h - 1, w) - x * 2;
  G(per.s->v, w - 1, h - 2, w) += x;
  G(per.s->v, w - 2, h - 1, w) += x;

  x = G(per.m->v, 0, h - 1, w) * DIFF_COEF;
  G(per.s->v, 0, h - 1, w) += G(per.m->v, 0, h - 1, w) - x * 2;
  G(per.s->v, 0, h - 2, w) += x;
  G(per.s->v, 1, h - 1, w) += x;

  /*
  double mean = 0;
  double rms = 0;
  double ma = -9999999;
  double mi = 9999999;
  for (i = 0; i < w; ++i) {
    for (j = 0; j < h; ++j) {
      mean += G(per.s->v, i, j, w);
      rms += G(per.s->v, i, j, w);
      ma = max(ma, G(per.s->v, i, j, w));
      mi = min(ma, G(per.s->v, i, j, w));
    }
  }
  mean /= (double)(w * h);
  rms = sqrtf(rms / (double)(w * h));

  fprintf(stdout, "%f %f %f %f\r", mean, rms, ma, mi);
  */
  
  pswap((void **)&per.s, (void **)&per.m);
  glBindTexture(GL_TEXTURE_2D, ttex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (uint32_t)per.w, (uint32_t)per.h, GL_RED, GL_FLOAT,  per.m->v);
  glBindTexture(GL_TEXTURE_2D, 0);
}

struct cloud {
  uint32_t prog;
  uint32_t vao;
  uint32_t vbo;

  float t31colCh;

  float t31wscale;
  float t31pscale;
  float t31pwscale;
  float t31persistence;
  float t31octaves;
  float t31curslice;
  uint32_t t31; // 128^3 Perlin-worley + Worley (inc freq) * 3
  struct i3da v31;
  //struct i3df v31;
  float t32scale;
  uint32_t t32; //  32^2 Worley (inc freq) * 3
  struct i3d v32;
  float t2scale;
  float t2persistence;
  float t2octaves;
  uint32_t t2;  // 128^2 Curl noise * 3
  struct i2d v2;
};

void update_clouds(void *__restrict cp) {
  struct cloud *__restrict c = cp;
  { // Perlin-worley
    if (!c->t31) {
      glGenTextures(1, &c->t31);
    }
    glBindTexture(GL_TEXTURE_3D, c->t31);
    noise_cloud3(128, 128, 128, c->t31octaves, c->t31persistence, c->t31pscale, c->t31pwscale, c->t31wscale, &c->v31);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 128, 128, 128, 0, GL_RGBA, GL_FLOAT, c->v31.v);
    //noise_pw3d(128, 128, 128, c->t31octaves, c->t31persistence, c->t31pscale, c->t31pwscale, &c->v31);
    //glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 128, 128, 128, 0, GL_RED, GL_FLOAT, c->v31.v);
    glGenerateMipmap(GL_TEXTURE_3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_3D, 0);
  }

  { // Worley
    if (!c->t32) {
      glGenTextures(1, &c->t32);
    }
    glBindTexture(GL_TEXTURE_3D, c->t32);
    noise_worl3(32, 32, 32, c->t32scale, &c->v32);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 32, 32, 32, 0, GL_RGB, GL_FLOAT, c->v32.v);
    glGenerateMipmap(GL_TEXTURE_3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_3D, 0);
  }

  { // Curl
    if (!c->t2) {
      glGenTextures(1, &c->t2);
    }
    glBindTexture(GL_TEXTURE_2D, c->t2);
    noise_curl3(128, 128, c->t2octaves, c->t2persistence, c->t2scale, &c->v2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_FLOAT, c->v2.v);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}

uint8_t rayHit(v3 s, v3 d, float dist, uint32_t mask) {
  struct sray r = csrayd(s, d, mask);

  uint32_t i, j;
  float rd = 0;
  uint8_t a;
  for(i = 0; i < objs.l; ++i) {
    //fprintf(stdout, "%u\n", i);
    for(j = 0; j < objs.v[i].mins.l; ++j) {
      //fprintf(stdout, "\\-%u\n", j);
      a = rbi(&r, &objs.v[i].mins.v[j].b);
      //fprintf(stdout, "  \\-%u\n", a);
      if (a) {
        rmi(&r, &objs.v[i].mins.v[j], &rd);
        if (rd >= -dist) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) { 
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  fprintf(stderr, "GL CALLBACK: %s id = 0x%x, type = 0x%x,\n\tseverity = 0x%x, message = %s\n", (type==GL_DEBUG_TYPE_ERROR?"** GL ERROR **":""), id, type, severity, message); 
}

uint8_t run_suijin() {
  init_random();
  reset_ft();
  setbuf(stdout, NULL);
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();
  glfwSetKeyCallback(window, kbp_callback);
  glfwSetMouseButtonCallback(window, mbp_callback);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPointSize(6.0);
  glLineWidth(3.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_PROGRAM_POINT_SIZE);
  //glEnable(GL_CULL_FACE);
  glLineWidth(10.0f);

  identity_matrix(identity);

  struct cloud c = {0}; /// Clouds
  {
    c.t31pscale = 82.8;
    c.t31pwscale = 13.34;
    c.t31persistence = 3.46;
    c.t31octaves = 2.1;
    c.t31curslice = 0.0;
    c.t31wscale = 10.0;
    c.t32scale = 8.64;
    c.t2scale = 8.64;
    c.t2persistence = 3.46;
    c.t2octaves = 2.1;
    update_clouds(&c);
  }

  { /// Asset loading
    modvi(&mods);
    matvi(&mats);
    linvi(&lins);

    parse_folder(&mods, &mats, "Resources/Items/Mountain");
    parse_folder(&mods, &mats, "Resources/Items/Dough");
    //parse_folder(&mods, &mats, "Resources/Items/Plant");
    //parse_folder(&mods, &uim, "Resources/Items/Plane");
    
    matvt(&mats);
    modvt(&mods);
  }

  uint32_t lvao, lvbo; /// Objects
  struct objv objs;
  {
    struct object cb;
    objvi(&objs);

    init_object(&cb, "MIAN");
    aobj(0, (v3) {10.0f, 10.0f, 10.0f} , (v3) {0.0f, 0.0f, 0.0f}, &cb, MGEOMETRY);
    objvp(&objs, cb);

    
    init_object(&cb, "DOUGH");
    aobj(1, (v3) {10.0f, 10.0f, 10.0f}, (v3) {-60.0f, 40.0f, -30.0f}, &cb, MPROP);
    aobj(2, (v3) {10.0f, 10.0f, 10.0f}, (v3) {-60.0f, 40.0f, -30.0f}, &cb, MPROP);
    objvp(&objs, cb);
    /*

    init_object(&cb, "PLANT");
    aobj(3, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    aobj(4, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    aobj(5, 10.0f, (v3) {30.0f, 10.0f, -30.0f}, &cb);
    objvp(&objs, cb);
    */

    objvt(&objs);

    clines(&lvao, &lvbo);
  }

#define PROGC 6 /// Shaders
  uint32_t nprog;
  uint32_t shaders[PROGC * 2];
  {
    char *snames[PROGC] = { "obj", "ui", "line", "point", "skybox", "text"}; //, "cloud" };
    char tmpn[128];

    uint32_t i;
    for (i = 0; i < PROGC; ++i) {
      snprintf(tmpn, sizeof(tmpn), "shaders/%s_vert.glsl", snames[i]);
      PR_CHECK(shader_get(tmpn, GL_VERTEX_SHADER  , shaders + 2 * i + 0));
      snprintf(tmpn, sizeof(tmpn), "shaders/%s_frag.glsl", snames[i]);
      PR_CHECK(shader_get(tmpn, GL_FRAGMENT_SHADER, shaders + 2 * i + 1));
    }

    PR_CHECK(program_get(2, shaders      , &nprog));
    PR_CHECK(program_get(2, shaders +   2, &uprog));
    PR_CHECK(program_get(2, shaders +   4, &lprog));
    PR_CHECK(program_get(2, shaders +   6, &pprog));
    PR_CHECK(program_get(2, shaders +   8, &skyprog));
    PR_CHECK(program_get(2, shaders +  10, &textprog));
    //PR_CHECK(program_get(2, shaders + 10, &c.prog));

    for (i = 0; i < PROGC; ++i) {
      glDeleteShader(shaders[i * 2]);
      glDeleteShader(shaders[i * 2 + 1]);
    }
  }

  uint32_t pvao, pvbo; /// Points
  { 
    cpoints(&pvao, &pvbo);

    v3 ce = {0.0f, 0.0f, 0.0f};

    glBindBuffer(GL_ARRAY_BUFFER, pvbo);
    glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float), 3 * sizeof(float), &ce);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  { /// Fragment Shader Stat
    struct stat s;
    stat(FRAG_PATH, &s);
    la = s.st_ctim.tv_sec;
  }

  { /// Camera
    int iw, ih;
    memset(&initCam, 0, sizeof(initCam));
    glfwGetWindowSize(window, &iw, &ih);
    initCam.ratio = (float)iw / (float)ih;

    initCam.up.y = 1.0f;
    initCam.fov = toRadians(90.0f);
    initCam.pos = (vec3) { 0.0f, 60.0f, 0.0f };
    initCam.orientation = (quat) { 1.0f, 0.0f, 0.0f, 0.0f};
    memcpy(&cam, &initCam, sizeof(cam));
  }

  { /// Cursor
    glfwGetCursorPos(window, &mouseX, &mouseY);
    oldMouseX = 0.0;
    oldMouseY = 0.0;
  }

  { /// UI
#define TSL(node, name, namescale, var, mi, ma, fun, funp) \
      add_title(node, name, namescale); \
      add_slider(node, var, mi, ma, fun, funp)
    prep_ui(window, &uvao);
    /*{
      mchvi(&nodes[0].children);

      add_title(&nodes[0], "スプーク・プーク", 1.0f);
      TSL(&nodes[0], "H", 0.20f, &per.h, 0.0, 1000.0, init_therm, NULL);
      TSL(&nodes[0], "W", 0.20f, &per.w, 0.0, 1000.0, init_therm, NULL);
      TSL(&nodes[0], "Oct", 0.20f, &per.foct, 0.0f, 5.0f, init_therm, NULL);
      TSL(&nodes[0], "Scale", 0.20f, &per.sc, 0.0f, 200.0f, init_therm, NULL);
      TSL(&nodes[0], "Persistance", 0.20f, &per.per, 0.0f, 4.0f, init_therm, NULL);
      TSL(&nodes[0], "Diffusion", 0.20f, &DIFF_COEF, 0.0f, 0.2f, init_therm, NULL);
      TSL(&nodes[0], "Style", 0.20f, &per.style, 0.0f, 1.9f, init_therm, NULL);
      nodes[1].px = 400.0f;
      nodes[1].py = 50.0f;
      nodes[0].sx = 300.0f;
      nodes[0].sy = 500.0f;
      nodes[0].bp = 20;
      nodes[0].tp = 20;
      nodes[0].lp = 20;
      nodes[0].rp = 20;
      mchvt(&nodes[0].children);
    }*/

    {
      mchvi(&nodes[1].children);
      add_title(&nodes[1], "│#Clpouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      add_title(&nodes[1], "│#Clouds", 25);
      TSL(&nodes[1], "pscale", 15, &c.t31pscale, 0.0, 200.0f, NULL, NULL);
      TSL(&nodes[1], "pwscale", 15, &c.t31pwscale, 0.0, 100.0f, NULL, NULL);
      TSL(&nodes[1], "persistence", 15, &c.t31persistence, 0.0, 4.0f, NULL, NULL);
      TSL(&nodes[1], "octaves", 15, &c.t31octaves, 0.0, 5.0f, NULL, NULL);
      TSL(&nodes[1], "curslice", 15, &c.t31curslice, 0.0, 1.0f, NULL, NULL);
      TSL(&nodes[1], "wscale", 15, &c.t31wscale, 0.0, 200.0f, NULL, NULL);
      TSL(&nodes[1], "scale", 15, &c.t32scale, 0.0, 200.0f, NULL, NULL);
      TSL(&nodes[1], "TEXT_SIZE", 15, &TEXT_SIZE, 0.0, 60.0f, NULL, NULL);
      TSL(&nodes[1], "colch", 15, &c.t31colCh, 0.0, 3.9f, NULL, NULL);
      TSL(&nodes[1], "Update", 15, &DELETE_ME, 0.0, 3.9f, update_clouds, &c);

      nodes[1].px = 400.0f;
      nodes[1].py = 50.0f;
      nodes[1].sx = 300.0f;
      nodes[1].sy = 800.0f;
      nodes[1].bp = 0;
      nodes[1].tp = 0;
      nodes[1].lp = 0;
      nodes[1].rp = 0;
      mchvt(&nodes[1].children);
    }
  }

  struct skybox sb; /// Skybox
  uint32_t sbt;
  {
    init_skybox(&sb);
    glGenTextures(1, &sbt);
    uint32_t sbw, sbh;
    uint8_t *buf = read_png("skybox.png", "Resources/Textures", &sbw, &sbh);
    update_rgb_tex(&sbt, sbh, sbw, buf);
    free(buf);
  }

  { /// Init therm
    per.w    = per.h    = 500;
    per.o1.h = per.o1.w = 500;
    per.sc = 28.5;
    per.per = 1.0f;
    per.foct = 3.5f;
    init_therm(NULL);
  }

  uint32_t frame = 0;
  struct sray cs;
  _ctime = _ltime = deltaTime = 0;
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    { /// Skybox
      glUseProgram(skyprog);
      program_set_mat4(skyprog, "fn", fn);
      sb.m.pos = cam.pos;
      maff(&sb.m);
      program_set_mat4(skyprog, "affine", sb.m.aff);

      /*float tscale = 1 / 3.0f;
      sb.sunPos.x = (sin(_ctime * tscale) + 1) * M_PI / 2;
      if (cos(_ctime) < 0.0f) {
        sb.sunPos.y = M_PI + cos(_ctime * tscale) * M_PI;
      } else {
        sb.sunPos.y = cos(_ctime * tscale) * M_PI;
      }
      program_set_float2(skyprog, "sunPos", sb.sunPos.x, sb.sunPos.y);
      program_set_float1(skyprog, "sunSiz", sb.sunSize);
      program_set_float3(skyprog, "sunCol", 0.7f, 0.2f, 0.1f);*/

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sbt);
      program_set_int1(skyprog, "tex", 0);

      glBindVertexArray(skyvao);
      glDrawArrays(GL_TRIANGLES, 0, sbvl);
    }

    if (drawObjs) { // NPROG
      glUseProgram(nprog);

      program_set_mat4(nprog, "fn_mat", fn);
      program_set_int1(nprog, "shading", cshading);
      program_set_float3v(nprog, "camPos", cam.pos);

      int32_t i, j;
      float rd, mrd = NEG_INF;

      if (canMoveCam && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        cs = csrayd(cam.pos, QV(cam.orientation), MPROP);
      }

      for(i = 0; i < objs.l; ++i) {
        for(j = 0; j < objs.v[i].mins.l; ++j) {
          draw_model(nprog, &mats, mods.v + objs.v[i].mins.v[j].m, objs.v[i].mins.v[j].aff);
          if (canMoveCam && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (rbi(&cs, &objs.v[i].mins.v[j].b)) {
              rmi(&cs, &objs.v[i].mins.v[j], &rd);
              if (rd > mrd && rd > NEG_INF) {
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

    if (drawTerm) {
      draw_squaret((windowW - 500) / 2, (windowH - 500) / 2, 500, 500, ttex, 2);
      upd_therm();
    }

    if (drawClouds) {
      glUseProgram(uprog);
      program_set_int1(uprog, "ch", floor(c.t31colCh));
      draw_squaret3((windowW - 500) / 2 - 275, (windowH - 500) / 2 - 275, 500, 500, c.t31, 8, c.t31curslice);
      draw_squaret3((windowW - 500) / 2 + 275, (windowH - 500) / 2 - 275, 500, 500, c.t32, 8, c.t31curslice);
      draw_squaret((windowW - 500) / 2, (windowH - 500) / 2 + 275, 500, 500, c.t2, 9);
    }

    if (drawUi) { // UPROG
      glDisable(GL_DEPTH_TEST);

      int32_t i;
      for(i = 1; i < MC; ++i) {
        draw_node(i);
      }
    }

    if (frame % 30 == 0) { /// Frag update
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

  glfwTerminate(); /// This for some reason crashes but exiting without it makes the drivers crash?????????
                   /// Update: This no longer happens??????

  return 0;
}

int main(int argc, char **argv) {
  GL_CHECK(run_suijin() == 0, "Running failed!");
  
  return 0;
}

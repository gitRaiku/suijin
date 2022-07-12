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
struct objv objects;

uint8_t cameraUpdate = 1;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 0.04f
#define SENS 0.001f

mat4 fn;
int windowW, windowH;
float iwinw, iwinh;
float P1, P2, P3, P4;

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
char *nms[10] = { "P1", "P2", "P3", "P4", "PERSISTANCE", "HEIGHT", "WIDTH" };
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
            lp[1] = 0;
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
              lp[1] = 1;
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
              lp[1] = 0;
            }
            break;
        }
      }
    }
  }

  { // KEY_PRESSED
    float cpanSpeed = PAN_SPEED + KEY_PRESSED(GLFW_KEY_LEFT_SHIFT) * PAN_SPEED;
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

void draw_obj(uint32_t program, struct matv *__restrict m, struct object *__restrict obj) {
  uint32_t li = 0;
  struct material *__restrict cmat;

  glBindVertexArray(obj->vao);

  program_set_mat4(program, "affine", obj->aff);

  cmat = &m->v[obj->m.v[0].m];
  update_mat(program, cmat);

  {
    int32_t i;
    for(i = 1; i < obj->m.l; ++i) {
      if (cmat->tamb.i != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmat->tamb.i);
        program_set_int1(program, "tex", 1);
        program_set_int1(program, "hasTexture", 1);
      } else {
        program_set_int1(program, "hasTexture", 0);
      }
      
      glDrawArrays(GL_TRIANGLES, li / 8, (obj->m.v[i].i - li) / 8);
      li = obj->m.v[i].i;

      cmat = &m->v[obj->m.v[i].m];
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

  glDrawArrays(GL_TRIANGLES, li / 8, (obj->v.l - li) / 8);
}

struct tbox {
  char text[64];
  uint32_t height;
};

struct fslider {
  float *__restrict val;
  uint32_t height;
};

union MCI {
  struct tbox t;
  struct fslider s;
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

#define MC 1
struct mod {
  float px, py;
  float sx, sy;
  mat4 aff;
  struct mchv children;
};
struct mod mods[MC];
uint32_t selectedUi = 0;

void add_title(struct mchv *__restrict m, char *__restrict t) {
  struct mchild mc;
  mc.id = MTEXT_BOX;
  mc.c = malloc(sizeof(struct tbox));
#define tmc ((struct tbox *)mc.c)
  strncpy(tmc->text, t, sizeof(tmc->text));
  tmc->height = 20;
#undef tmc

  mchvp(m, mc);
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
void draw_square(float px, float py, float sx, float sy, uint32_t tex) {
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
    ++*str;
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

      //fprintf(stdout, "%c %u\n", *ct, slot->bitmap.rows);
      //draw_square(x + P3 * P1 * px * iwinw, y + py + P3 * P2 * (mh - slot->bitmap.rows) * iwinh, P3 * slot->bitmap.width, P3 * slot->bitmap.rows, ctex);

      int32_t ymax = ftface->bbox.yMax >> 6;
      int32_t advance = ftface->glyph->metrics.horiAdvance >> 6;
      int32_t fw = ftface->glyph->metrics.width >> 6;
      int32_t xoff = (advance - fw) >> 1;
      int32_t yoff = ymax - (ftface->glyph->metrics.horiBearingY >> 6);

      // fprintf(stdout, "%c - %4i %4i %4i %4i %4i\n", *ct, ymax, advance, fw, xoff, yoff);
      
      draw_square(px + x + xoff, y + yoff, fw, ymax, ctex);

      px += advance;

      ct += cl;
    }
  }

  return 0;
}

uint32_t draw_slider(float x, uint32_t y, struct tbox *__restrict t) {
  return 0;
}

void draw_ui(uint32_t mi) {
#define cm mods[mi]
  if (lp[1] == 0 &&
      (0 <= mouseX - cm.px && mouseX - cm.px <= cm.sx) &&
      (0 <= mouseY - cm.py && mouseY - cm.py <= cm.sy)) { 
    selectedUi = mi;
    mxoff = cm.px - mouseX;
    myoff = cm.py - mouseY;
  }

  draw_square(cm.px, cm.py, cm.sx, cm.sy, 0);

  {
    int32_t i;
    uint32_t cy = 0;
    for(i = 0; i < cm.children.l; ++i) {
      switch (cm.children.v[i].id) {
        case MTEXT_BOX:
          cy += draw_textbox(cm.px - cm.sx / 2, cm.py, cm.children.v[i].c);
          break;
        case MFSLIDER:
          cy += draw_slider(cm.px - cm.sx * iwinw, cm.py, cm.children.v[i].c);
          break;
      }
    }
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

  //FT_CHECK(FT_New_Face(ftlib, "Resources/Fonts/Koruri/Koruri-Regular.ttf", 0, &ftface), "Could not load the font");
  FT_CHECK(FT_New_Face(ftlib, "Resources/Fonts/JetBrains/jetbrainsmono.ttf", 0, &ftface), "Could not load the font");
  FT_CHECK(FT_Set_Pixel_Sizes(ftface, 0, (int32_t)42), "Could not set pixel sizes");
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

  matvi(&mats);
  objvi(&objects);

  parse_folder(&objects, &mats, "Resources/Items/Mountain");
  //parse_folder(&objects, &mats, "Resources/Items/Dough");
  //parse_folder(&objects, &uim, "Resources/Items/Plane");

  matvt(&mats);
  objvt(&objects);

  uint32_t nprog;

  _ctime = _ltime = deltaTime = 0;

#define SHADERC 4
  uint32_t shaders[SHADERC];
  {
    const char          *paths[SHADERC] = { "shaders/vv.glsl", "shaders/vf.glsl", 
                                            "shaders/uv.glsl", "shaders/uf.glsl"};
    const uint32_t shaderTypes[SHADERC] = { GL_VERTEX_SHADER   , GL_FRAGMENT_SHADER,
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

  mchvi(&mods[0].children);
  //add_title(&mods[0].children, "Cină de 日本");
  add_title(&mods[0].children, "am paqatufît");
  mods[0].px = 500.0f;
  mods[0].py = 500.0f;
  mods[0].sx = 25.0f;
  mods[0].sy = 25.0f;
  mchvt(&mods[0].children);

  P1 = 1.7f;
  P2 = 1.2f;
  P3 = 1.0f;
  P4 = 42.0f;

  vals[0] = &P1;
  vals[1] = &P2;
  vals[2] = &P3;
  vals[3] = &P4;

  scal[0] = scal[1] = scal[2] = 0.1f;
  scal[3] = 1.0f;
  lims[0] = lims[1] = lims[2] = (v2) {-1000.0f, 1000.0f};
  lims[3] = (v2) {1.0f, 30000.0f};

  reset_ft();

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
      int32_t i;
      for(i = 0; i < objects.l; ++i) {
        draw_obj(nprog, &mats, objects.v + i);
      }
    }

    if (lp[1] == 0) {
      selectedUi = 1001;
    }
    { // UPROG
      glUseProgram(uprog);
      glDisable(GL_DEPTH_TEST);

      int32_t i;
      for(i = 0; i < MC; ++i) {
        draw_ui(i);
      }
    }

    if (lp[1] && selectedUi != 1001) {
      mods[selectedUi].px = mouseX + mxoff;
      mods[selectedUi].py = mouseY + myoff;
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

  {
    int32_t i;
    for(i = 0; i < objects.l; ++i) {
      free(objects.v[i].m.v);
    }
    free(objects.v);
    free(mats.v);
  }

  glfwTerminate();

  return 0;
}

int main(int argc, char **argv) {
  GL_CHECK(run_suijin() == 0, "Running failed!");
  
  return 0;
}

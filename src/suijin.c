#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "env.h" 

static struct env *__restrict e;
void suijin_setenv(struct env *__restrict ee) { e = ee; }

float DELETE_ME; /// TODO: Delete

double _ctime, _ltime;
double deltaTime, dt;

uint8_t drawObjs = 1;
uint8_t drawTerm = 0;
uint8_t drawClouds = 1;
uint8_t drawUi = 0;
uint8_t drawFps = 0;
uint8_t drawBb = 0;
uint8_t drawTexture = 0;

mat4 identity;

/// TODO
v3 HITP;
uint8_t HITS = 0;

int32_t cshading = 0;

double mouseX, mouseY;
double oldMouseX, oldMouseY;

#define FRAG_PATH "shaders/cloud_frag.glsl"

enum UIT {
  UIT_CANNOT, UIT_CAN, UIT_NODE, UIT_SLIDERK, UIT_TEXT, UIT_CHECK
};

union KMS {
  uint32_t u32;
  void *__restrict fp;
};

struct uiSelection {
  enum UIT t;
  enum UIT nt;
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
  vec3 f = norm(QV(e->cam.orientation));
  vec3 s = norm(cross(e->cam.up, f));
  vec3 u = cross(f, s);

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
         
  m[0][3] = -dot(s, e->cam.pos);
  m[1][3] = -dot(u, e->cam.pos);
  m[2][3] = -dot(f, e->cam.pos);
  m[3][3] = 1.0f;
}

mat4 fn;
void update_camera_matrix() {
  mat4 proj, view;
  if (e->cam.up.x == e->cam.orientation.x && e->cam.up.y == e->cam.orientation.y && e->cam.up.z == e->cam.orientation.z) {
    e->cam.orientation.x = 0.0000000000000001;
  }
  create_projection_matrix(proj, e->cam.fov, e->cam.ratio, 0.01f, 3000.0f);
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
uint32_t curi = 0;
float *vals[10];
float scal[10];
char *nms[10] = { "A", "P2", "P3", "P4", "PERSISTANCE", "HEIGHT", "WIDTH" };
v2 lims[10];
double startY = 0;
uint8_t canMoveCam = 1;
float mxoff, myoff;


      double theta = 0.0;
      double phi = 0.0;
      int8_t kk = 1;

double sgn(double kms) {
  if (kms < 0) {
    return -1;
  } else {
    return 1;
  }
}

void handle_input() {
  { // CURSOR
    glfwGetCursorPos(e->window, &mouseX, &mouseY);

    //if ((mouseX != oldMouseX) || (mouseY != oldMouseY)) {
    if (0) {
      double xoff = mouseX - oldMouseX;
      double yoff = mouseY - oldMouseY;
      oldMouseX = mouseX;
      oldMouseY = mouseY;

      if (canMoveCam) {
        if (0) {
          quat qx = gen_quat(e->cam.up, -xoff * SENS);
          //quat qy = gen_quat(cross(QV(e->cam.orientation), e->cam.up), yoff * SENS);
          //quat qx = gen_quat(e->cam.up, 0.0);
          vec3 cr = norm(cross(QV(e->cam.orientation), e->cam.up));
          fprintf(stdout, "%.2f %.2f %.2f\n", cr.x, cr.y, cr.z);
          quat qy = gen_quat(cr, yoff * SENS);
          quat qz = gen_quat(norm(cross(cr, QV(e->cam.orientation))), kk * SENS);
          // quat qy = gen_quat((vec3){0.0, 0.0, 1.0}, yoff * SENS);

          quat qr = qmul(qz, qmul(qy, qx));
          quat qt = qmul(qmul(qr, e->cam.orientation), qconj(qr));
          print_quat(e->cam.orientation, "cam");
          print_quat(qx, "qx");
          print_quat(qx, "qy");
          print_quat(qx, "qr");
          print_quat(qx, "qt");
          qt = qnorm(qt);
          print_quat(qx, "qtn");
          e->cam.orientation = qt;
          //e->cam.orientation.y = 0.0;
        } else {
          //yoff = 0.0;
          theta += xoff * SENS;
          phi += yoff * SENS;
          fprintf(stdout, "%.2f(%.2f) %.2f(%.2f)\n", theta, sin(theta), phi, sin(phi));
        
          vec3 kms = norm((vec3) { 
              sin(theta), /// x
              cos(theta), /// y
              0.0             /// z
              });
          e->cam.orientation.x = kms.x;
          e->cam.orientation.y = kms.y;
          e->cam.orientation.z = kms.z;
          print_quat(e->cam.orientation, "cam");
        }

        e->camUpdate = 1;
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
        case GLFW_KEY_H:
          if (kp.action == 1) {
            /*
            e->cam.orientation.x *= -1;
            e->cam.orientation.y *= -1;
            e->cam.orientation.z *= -1;
            */
            kk = 1 - kk;
          }
          break;
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
            glfwSetInputMode(e->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPos(e->window, e->w.windowW / 2.0, e->w.windowH / 2.0);
          } else if (kp.action == 0) {
            ms = CAMERA;
            canMoveCam = 1;
            drawUi = 0;
            selui.t = UIT_CANNOT;
            glfwSetInputMode(e->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
      e->cam.fov += e->cam.fov * ZOOM_SPEED;
      e->camUpdate = 1;
    } else if (KEY_PRESSED(GLFW_KEY_EQUAL)) {
      e->cam.fov = e->initCam.fov;
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_MINUS)) {
      e->cam.fov -= e->cam.fov * ZOOM_SPEED;
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_A)) {
      e->cam.pos = v3a(v3m(cpanSpeed, norm(cross(QV(e->cam.orientation), e->cam.up))), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_D)) {
      e->cam.pos = v3a(v3m(cpanSpeed, v3n(norm(cross(QV(e->cam.orientation), e->cam.up)))), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_SPACE)) {
      e->cam.pos = v3a(v3m(cpanSpeed, e->cam.up), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_LEFT_CONTROL)) {
      e->cam.pos = v3a(v3m(cpanSpeed, v3n(e->cam.up)), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_W)) {
      e->cam.pos = v3a(v3m(cpanSpeed, v3n(QV(e->cam.orientation))), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_S)) {
      e->cam.pos = v3a(v3m(cpanSpeed, QV(e->cam.orientation)), e->cam.pos);
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_U)) {
      memcpy(&e->cam, &e->initCam, sizeof(e->cam));
      e->camUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_Q)) {
      glfwSetWindowShouldClose(e->window, 1);
    }
  }
}

void check_shader_update(uint32_t *__restrict program, char *__restrict path, uint32_t type, uint32_t vert, time_t *__restrict la) {
  struct stat s;
  stat(path, &s);
  if (s.st_ctim.tv_sec != *la) {
    fprintf(stdout, "UPDATED %lu\n", *la);
    *la = s.st_ctim.tv_sec;
    if (type == GL_FRAGMENT_SHADER) {
      uint32_t shaders[2];
      shaders[0] = vert;
      if (!shader_get(path, type, shaders + 1)) {
        program_get(2, shaders, program);
        glDeleteShader(shaders[1]);
      }
    } else if (type == GL_COMPUTE_SHADER) {
      uint32_t sh;
      if (shader_get(path, type, &sh)) { return; }
      if (program_get(1, &sh, program)) { return; }
    }
  }
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

enum MCHID {
  MTEXT_BOX, MFSLIDER, MTSLIDER, MCHECKBOX
};

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
#define UI_CLICKABLE 0b1

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

void add_title(struct node *__restrict m, char *__restrict t, uint32_t tsize, uint32_t pad) {
  struct mchild mc = {0};
  mc.id = MTEXT_BOX;
  mc.c = calloc(sizeof(struct tbox), 1);
#define tmc ((struct tbox *)mc.c)
  strncpy(tmc->text, t, sizeof(tmc->text));
  tmc->tsize = tsize * 64;
  FT_CHECK(FT_Set_Char_Size(e->ftface, 0, tmc->tsize, 0, 88), "");
  mc.height =e->ftface->size->metrics.y_ppem;
#undef tmc
  mc.pad = pad;

  mchvp(&m->children, mc);
}

void add_button(struct node *__restrict m, char *__restrict t, uint64_t bg, uint32_t tsize, uint32_t pad, void (*callback)(void*), void *p) {
  struct mchild mc = {0};
  mc.id = MTEXT_BOX;
  mc.c = calloc(sizeof(struct tbox), 1);
#define tmc ((struct tbox *)mc.c)
  strncpy(tmc->text, t, sizeof(tmc->text));
  tmc->tsize = tsize * 64;
  FT_CHECK(FT_Set_Char_Size(e->ftface, 0, tmc->tsize, 0, 88), "");
#undef tmc
  mc.flags = UI_CLICKABLE;
  mc.callback = callback;
  mc.cbp = p;
  mc.pad = pad;
  mc.bg = bg;
  mc.height =e->ftface->size->metrics.y_ppem;

  mchvp(&m->children, mc);
}

void add_checkbox(struct node *__restrict m, char *__restrict t, uint64_t bg, uint32_t tsize, uint32_t pad, uint8_t *__restrict val, void (*callback)(void*), void *p) {
  struct mchild mc = {0};
  mc.id = MCHECKBOX;
  mc.c = calloc(sizeof(struct mtcheck), 1);

#define tmc ((struct mtcheck *)mc.c)
  strncpy(tmc->text.text, t, sizeof(tmc->text.text));
  tmc->text.tsize = tsize * 64;
  FT_CHECK(FT_Set_Char_Size(e->ftface, 0, tmc->text.tsize, 0, 88), "");
  mc.height =e->ftface->size->metrics.y_ppem;
  tmc->val = val;
#undef tmc
  mc.flags = UI_CLICKABLE;
  mc.callback = callback;
  mc.cbp = p;
  mc.pad = pad;
  mc.bg = bg;
  mc.height =e->ftface->size->metrics.y_ppem;

  mchvp(&m->children, mc);
}

void add_slider(struct node *__restrict m, float *__restrict v, float lb, float ub, uint32_t pad, void (*callback)(void*), void *p) {
  struct mchild mc = {0};
  mc.id = MFSLIDER;
  mc.c = calloc(sizeof(struct fslider), 1);
#define tmc ((struct fslider *)mc.c)
  tmc->val = v;
  tmc->lims = (v2) {lb, ub};
#undef tmc

  mc.flags = UI_CLICKABLE;
  mc.callback = callback;
  mc.cbp = p;
  mc.pad = pad;
  mc.height = 36;

  mchvp(&m->children, mc);
}

void add_tslider(struct node *__restrict m, char *__restrict t, uint32_t tsize, float *__restrict v, float lb, float ub, uint32_t pad, void (*callback)(void*), void *p) {
  struct mchild mc = {0};
  mc.id = MTSLIDER;
  mc.c = calloc(sizeof(struct mtslider), 1);
#define tmc ((struct mtslider *)mc.c)
  strncpy(tmc->text.text, t, sizeof(tmc->text.text));
  tmc->text.tsize = tsize * 64;
  FT_CHECK(FT_Set_Char_Size(e->ftface, 0, tmc->text.tsize, 0, 88), "");
  mc.height =e->ftface->size->metrics.y_ppem;
  tmc->val.tsize = tsize * 64;
  tmc->slid.val = v;
  tmc->slen = 150;
  tmc->slid.lims = (v2) {lb, ub};
#undef tmc

  mc.callback = callback;
  mc.cbp = p;
  mc.pad = pad;

  mchvp(&m->children, mc);
}

void identity_matrix(mat4 m) {
  m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f; 
  m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f; 
  m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f; 
  m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f; 
}

void create_affine_matrix(mat4 m, float xs, float ys, float zs, float x, float y, float z) {
  m[0][0] = xs * e->w.iwinw; m[0][1] =               0; m[0][2] =  0; m[0][3] = x; 
  m[1][0] =               0; m[1][1] = ys * e->w.iwinh; m[1][2] =  0; m[1][3] = y; 
  m[2][0] =               0; m[2][1] =               0; m[2][2] = zs; m[2][3] = z; 
  m[3][0] =               0; m[3][1] =               0; m[3][2] =  0; m[3][3] = 1; 
}

uint32_t uvao, uprog, lprog, pprog; // Draw Square
void draw_squaret(float px, float py, float sx, float sy, uint32_t tex, int32_t type) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);
  float cpx = ((px + sx / 2) * e->w.iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * e->w.iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  if (tex) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex);
    program_set_int1(uprog, "tex", 2);
    program_set_int1(uprog, "type", type);
  } else {
    program_set_float4(uprog, "col", 0.0f, 0.0f, 0.0f, 1.0f);
    program_set_int1(uprog, "type", 0);
  }

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

v4 gcor(v4 in, float gamma) { return (v4) { pow(in.x, gamma), pow(in.y, gamma), pow(in.z, gamma), in.w }; }

uint32_t textprog; // Draw text
void draw_squaretext(float px, float py, float sx, float sy, uint32_t tex, uint64_t bgcol, uint64_t fgcol) {
  mat4 aff;
  float cpx = ((px + sx / 2) * e->w.iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * e->w.iwinh) * 2 - 1;
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
  float cpx = ((px + sx / 2) * e->w.iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * e->w.iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);

  program_set_int1(uprog, "tex", 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_3D, tex);
  program_set_int1(uprog, "tex3", 1);
  if (type >= 80 && type <= 83) {
    program_set_int1(uprog, "type", 8);
    program_set_int1(uprog, "ch", type - 80);
  } else {
    program_set_int1(uprog, "type", type);
  }
  program_set_float1(uprog, "z", z);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw_squarec(float px, float py, float sx, float sy, uint32_t col) {
  mat4 aff;
  glUseProgram(uprog);
  glBindVertexArray(uvao);

  float cpx = ((px + sx / 2) * e->w.iwinw) * 2 - 1;
  float cpy = ((py + sy / 2) * e->w.iwinh) * 2 - 1;
  create_affine_matrix(aff, sx, sy, 1.0f, cpx, -cpy, 0.0f);
  program_set_mat4(uprog, "affine", aff);
  program_set_int1(uprog, "type", 0);
  program_set_int1(uprog, "tex3", 0);
  program_set_int1(uprog, "tex", 1);
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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

uint8_t ui_pointer_in(float x, float y, float w, float h) {
  //fprintf(stdout, "Try pointer %f %f in %f - %f; %f - %f\n", mouseX, mouseY, x, x + w, y, y + h);
  return (0 <= mouseX - x && mouseX - x <= w) &&
         (0 <= mouseY - y && mouseY - y <= h);
}

uint64_t invc(uint64_t c) {
  return ((0xFF - ((c & 0xFF000000) >> 24)) << 24) | 
         ((0xFF - ((c & 0x00FF0000) >> 16)) << 16) | 
         ((0xFF - ((c & 0x0000FF00) >>  8)) <<  8) | 
         (c & 0x000000FF);
}

float TEXT_SIZE = 20.0;
void get_textbox_size(struct tbox *__restrict t) {
  uint32_t cl;
  FT_UInt gi;
  char *__restrict ct = t->text;
  FT_ULong cchar;
  int32_t px = 0;
  while (*ct) {
    cl = runel(ct);
    cchar = utf8_to_unicode(ct, cl);
    gi = FT_Get_Char_Index(e->ftface, cchar);

    FT_CHECK(FT_Load_Glyph(e->ftface, gi, FT_LOAD_DEFAULT | FT_LOAD_COLOR | FT_LOAD_FORCE_AUTOHINT), "Could not load glyph");
    FT_Render_Glyph(e->ftface->glyph, FT_RENDER_MODE_NORMAL);

    px +=e->ftface->glyph->metrics.horiAdvance >> 6;
    ct += cl;
  }
  t->tlen = px;
}

uint32_t draw_textbox(float x, uint32_t y, struct mchild *__restrict mc, struct tbox *__restrict t) { /// MAKE EFFICIENT
  uint32_t px = 0;
  FT_UInt gi;
  FT_GlyphSlot slot =e->ftface->glyph;

  glUseProgram(textprog);
  glBindVertexArray(uvao);
  FT_CHECK(FT_Set_Char_Size(e->ftface, 0, t->tsize, 0, 88),"Could not set the character size!");

  {
    char *__restrict ct = t->text;
    uint8_t cl;
    FT_ULong cchar;

    if (!t->tex) {
      glGenTextures(1, &t->tex);
    }

    if (!t->tlen) { get_textbox_size(t); }

    uint64_t curbg = mc->bg;
    if (mc->flags & UI_CLICKABLE) {
      if (selui.t == UIT_CAN && ui_pointer_in(x, y - mc->pad / 1.4, t->tlen, mc->height + mc->pad)) {
        curbg = invc(mc->bg);
        selui.t = UIT_TEXT;
        selui.id.fp = t;

        if (mc->callback) {
          mc->callback(mc->cbp);
        }
      }

      if (selui.t == UIT_TEXT && selui.id.fp == t) {
        curbg = invc(mc->bg);
      }
    }

    if (mc->bg) {
      draw_squarec(x, y + mc->pad / 1.4, t->tlen, mc->height + mc->pad, curbg);
    }

    glUseProgram(textprog);
    ct = t->text;
    px = 0;
    while (*ct) {
      cl = runel(ct);
      cchar = utf8_to_unicode(ct, cl);
      gi = FT_Get_Char_Index(e->ftface, cchar);

      FT_CHECK(FT_Load_Glyph(e->ftface, gi, FT_LOAD_DEFAULT | FT_LOAD_COLOR | FT_LOAD_FORCE_AUTOHINT), "Could not load glyph");
      FT_Render_Glyph(e->ftface->glyph, FT_RENDER_MODE_NORMAL);
      update_bw_tex(&t->tex, slot->bitmap.rows, slot->bitmap.width, slot->bitmap.buffer);

      int32_t fw =e->ftface->glyph->metrics.width >> 6;
      int32_t fh =e->ftface->glyph->metrics.height >> 6;
      int32_t xoff =e->ftface->glyph->metrics.horiBearingX >> 6;
      int32_t yoff = (-e->ftface->glyph->metrics.horiBearingY >> 6) + mc->height;

      draw_squaretext(x + px + xoff, y + mc->pad + yoff, fw, fh, t->tex, curbg ? (curbg & ~0xFF) : UI_COL_BG1, mc->fg ? mc->fg : UI_COL_FG3);

      px +=e->ftface->glyph->metrics.horiAdvance >> 6;

      ct += cl;
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  return mc->height + mc->pad;
}

uint32_t sS = 21; // Draw slider
uint32_t draw_slider(float x, float y, uint32_t tlen, struct node *__restrict cm, struct mchild *__restrict mc, struct fslider *__restrict t) {
  if (tlen == 0) {
    tlen = cm->sx - cm->lp - cm->rp - sS;
  }
  float   rlen = t->lims.y - t->lims.x;
  int32_t xoff = (*t->val - t->lims.x) / rlen * tlen;

  draw_squarec(x + sS / 2, y + sS * 0.375f, tlen, sS / 4, mc->bg ? mc->bg : UI_COL_BG2); // Line
  draw_squarec(x + xoff, y, sS, sS, mc->fg ? mc->fg : UI_COL_FG1);

  if (mc->flags & UI_CLICKABLE) {
    if (selui.t == UIT_CAN && ui_pointer_in(x + xoff, y, sS, sS)) {
      selui.t = UIT_SLIDERK;
      selui.id.fp = t;
      selui.dx = mouseX;
      selui.dy = *t->val;
    }

    if (selui.t == UIT_SLIDERK && selui.id.fp == t) {
      *t->val = clamp(selui.dy + (mouseX - selui.dx) / tlen * rlen, t->lims.x, t->lims.y);
      if (mc->callback != NULL && mouseX != selui.dx) {
        mc->callback(mc->cbp);
      }
    }
  }

  return mc->height + mc->pad;
}

uint32_t draw_tslider(float x, float y, struct node *__restrict cm, struct mchild *__restrict mc, struct mtslider *__restrict t) {
  uint32_t h = draw_textbox(x, y, mc, &t->text);
  mc->flags |= UI_CLICKABLE;
  draw_slider(x + t->text.tlen + sS / 2, y + h / 3, t->slen, cm, mc, &t->slid);
  mc->flags &= ~UI_CLICKABLE;
  snprintf(t->val.text, sizeof(t->val.text), "%.2f", *t->slid.val);
  draw_textbox(x + t->text.tlen + t->slen + sS + sS, y, mc, &t->val);
  return mc->height + mc->pad;
}

uint32_t draw_checkpox(float x, float y, struct node *__restrict cm, struct mchild *__restrict mc, struct mtcheck *__restrict t) {
  uint32_t cbg = mc->bg; mc->bg = 0;
  draw_textbox(x, y, mc, &t->text);
  mc->bg = cbg;
  mc->flags |= UI_CLICKABLE;
  
  int32_t xoff = t->text.tlen + 20;
  int32_t yoff = (mc->height + mc->pad) / 2;
  draw_squarec(x + xoff, y + yoff, sS, sS, *t->val ? 0xFFFFFFFF : 0xFFFF00FF);

  if (mc->flags & UI_CLICKABLE) {
    if (selui.t == UIT_CAN && ui_pointer_in(x + xoff, y + yoff, sS, sS)) {
      selui.t = UIT_CHECK;
      selui.id.fp = t;
    }

    if (selui.t == UIT_CHECK && selui.id.fp == t) {
      selui.t = UIT_CANNOT;
      *t->val = 1 - *t->val;
      if (mc->callback != NULL) { mc->callback(mc->cbp); }
    }
  }

  mc->flags &= ~UI_CLICKABLE;
  return mc->height + mc->pad;
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
          cy += draw_textbox(cm.px + cm.lp, cm.py + cy, cm.children.v + i, cm.children.v[i].c);
          break;
        case MFSLIDER:
          cy += draw_slider(cm.px + cm.lp, cm.py + cy, 0, &cm, cm.children.v + i, cm.children.v[i].c);
          break;
        case MTSLIDER:
          cy += draw_tslider(cm.px + cm.lp, cm.py + cy, &cm, cm.children.v + i, cm.children.v[i].c);
          break;
        case MCHECKBOX:
          cy += draw_checkpox(cm.px + cm.lp, cm.py + cy, &cm, cm.children.v + i, cm.children.v[i].c);
          break;
        }
    }
  }

  if (selui.t == UIT_CAN && ui_pointer_in(cm.px, cm.py, cm.sx, cm.sy)) {
    selui.t = UIT_NODE;
    selui.id.fp = &cm;
    selui.dx = cm.px - mouseX;
    selui.dy = cm.py - mouseY;
  }

  if (selui.t == UIT_NODE && selui.id.fp == &cm) {
    cm.px = mouseX + selui.dx;
    cm.py = mouseY + selui.dy;
  }
#undef cm
}

void prep_ui(uint32_t *__restrict vao) {
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
  cl.s = cb->i; for(i = 0; i < 3; ++i) { cl.e = cb->i; FPC(&cl.e)[i] = FPC(&cb->a)[i]; linvp(&lins, cl); }
  cl.s = cb->a; for(i = 0; i < 3; ++i) { cl.e = cb->a; FPC(&cl.e)[i] = FPC(&cb->i)[i]; linvp(&lins, cl); }
  cl.s = cl.e = cb->i; CHSA(0); CHEA(0); CHEA(1); linvp(&lins, cl);
  cl.s = cl.e = cb->i; CHSA(0); CHEA(0); CHEA(2); linvp(&lins, cl);
  cl.s = cl.e = cb->i; CHSA(1); CHEA(1); CHEA(0); linvp(&lins, cl);
  cl.s = cl.e = cb->i; CHSA(1); CHEA(1); CHEA(2); linvp(&lins, cl);
  cl.s = cl.e = cb->i; CHSA(2); CHEA(2); CHEA(0); linvp(&lins, cl);
  cl.s = cl.e = cb->i; CHSA(2); CHEA(2); CHEA(1); linvp(&lins, cl);
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
  cm.b.i = v3m4(cm.aff,e->mods.v[cm.m].b.i);
  cm.b.a = v3m4(cm.aff,e->mods.v[cm.m].b.a);
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
#define cmm e->mods.v[cm->m]
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
  glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

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

struct cloud {
  struct minf m;
  struct img worl;
  struct img pwor;
  struct img curl;
  v3 pos;
};
struct cloud cl;

uint32_t cloudvao, cloudvbo, cloudprog, cbvl;
void init_clouds(struct cloud *__restrict c) {
  if (!cloudvao) { glGenVertexArrays(1, &cloudvao); }
  if (!cloudvbo) { glGenBuffers(1, &cloudvbo); }

  memset(c, 0, sizeof(*c));
  c->m.scale.x = 100.0f;
  c->m.scale.z = 100.0f;
  c->m.scale.y = 10.0f;
  c->m.pos.y = 200.0f;
  c->m.pos.x = -20.0f;
  c->m.pos.z = -16.0f;
  quat qr = gen_quat((vec3) { 1.0f, 0.0f, 0.0f }, M_PI / 4);
  c->m.rot = qnorm(qmul(qmul(qr, (quat) {0.0f, 1.0f, 0.0f, 0.0f} ), qconj(qr)));
  c->m.mask = MSKYBOX;
  maff(&c->m);

  float cbv[] = {
    -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,
  };

  cbvl = sizeof(cbv) / sizeof(float) / 3;

  glBindVertexArray(cloudvao);
  glBindBuffer(GL_ARRAY_BUFFER, cloudvbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cbv), cbv, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
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
    //noise_p2d((uint32_t)per.h, (uint32_t)per.w, per.oct, per.per, per.sc, per.m);
  } else {
    //noise_w2d((uint32_t)per.h, (uint32_t)per.w, per.sc, per.m);
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
  if (perlinR == 1) { perlinR = 0; init_therm(NULL); return; }
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
  
  pswap((void **)&per.s, (void **)&per.m);
  glBindTexture(GL_TEXTURE_2D, ttex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (uint32_t)per.w, (uint32_t)per.h, GL_RED, GL_FLOAT,  per.m->v);
  glBindTexture(GL_TEXTURE_2D, 0);
}

uint8_t rayHit(v3 s, v3 d, float dist, uint32_t mask) {
  struct sray r = csrayd(s, d, mask);

  uint32_t i, j;
  float rd = 0;
  uint8_t a;
  for(i = 0; i < e->objs.l; ++i) {
    //fprintf(stdout, "%u\n", i);
    for(j = 0; j < e->objs.v[i].mins.l; ++j) {
      //fprintf(stdout, "\\-%u\n", j);
      a = rbi(&r, &e->objs.v[i].mins.v[j].b);
      //fprintf(stdout, "  \\-%u\n", a);
      if (a) {
        rmi(&r, &e->objs.v[i].mins.v[j], &rd);
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

float scale = 10.0;
float pscale = 35;
float pwscale = 40;
float curslcs = 0.5;
float kmsper = 0.53;
float kmsoct = 3.1;
float curch = 80.1;
float KMS = 2.2;

float minDensity = 0.2;
float densityMultiplier = 2.0;

void compute_shader() {
  noise_pw(128, 128, 128, (uint32_t)kmsoct, kmsper, pscale, pwscale, scale, &cl.pwor);
  noise_ww(32, 32, 32, scale, &cl.worl);
  //noise_p(128, 128, 128, (uint32_t)kmsoct, kmsper, scale, &cp.perl);
  noise_c(128, 128, 128, (uint32_t)kmsoct, kmsper, scale, &cl.curl);
}

float kkautoupda = 0.2;
void autoupda() { if (kkautoupda >= 1.0) { compute_shader(); } }

void load_assets() {
  modvi(&e->mods);
  matvi(&e->mats);
  linvi(&lins);

  parse_folder(&e->mods, &e->mats, "Resources/Items/Mountain");
  parse_folder(&e->mods, &e->mats, "Resources/Items/Dough");
  
  matvt(&e->mats);
  modvt(&e->mods);
}

struct skybox sb; /// Skybox
uint32_t sbt;
static time_t cla = 0;  // Time cloud last access
static time_t ccla = 0; // Time worley comupte last access
struct sray cs;
uint32_t pvao, pvbo; /// Points
uint32_t nprog;
#define PROGC 7 /// Shadercurslcs
uint32_t shaders[PROGC * 2];
uint32_t lvao, lvbo; /// Objects

uint8_t suijin_init() {
  if (!e) {
    fprintf(stderr, "Env not passed to suijin!\n");
    exit(1);
  }
  fprintf(stdout, "Env %p\n", e);
  glfwSetKeyCallback(e->window, kbp_callback);
  glfwSetMouseButtonCallback(e->window, mbp_callback);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPointSize(6.0);
  glLineWidth(3.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glLineWidth(10.0f);

  identity_matrix(identity);
  load_assets();

  {
    struct object cb;
    objvi(&e->objs);

    init_object(&cb, "MIAN");
    aobj(0, (v3) {10.0f, 10.0f, 10.0f} , (v3) {0.0f, 0.0f, 0.0f}, &cb, MGEOMETRY);
    objvp(&e->objs, cb);
    
    init_object(&cb, "DOUGH");
    aobj(1, (v3) {10.0f, 10.0f, 10.0f}, (v3) {-60.0f, 40.0f, -30.0f}, &cb, MPROP);
    aobj(2, (v3) {10.0f, 10.0f, 10.0f}, (v3) {-60.0f, 40.0f, -30.0f}, &cb, MPROP);
    objvp(&e->objs, cb);

    objvt(&e->objs);
    clines(&lvao, &lvbo);
  }

  {
    char *snames[PROGC] = { "obj", "ui", "line", "point", "skybox", "text", "cloud"};
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
    PR_CHECK(program_get(2, shaders +  12, &cloudprog));
    //PR_CHECK(program_get(2, shaders + 10, &c.prog));

    /*
    for (i = 0; i < PROGC; ++i) {
      glDeleteShader(shaders[i * 2]);
      glDeleteShader(shaders[i * 2 + 1]);
    }*/
    if (prep_compute_shaders()) { exit(1); }
    new_perlin_perms();
  }



  { 
    cpoints(&pvao, &pvbo);

    v3 ce = {0.0f, 0.0f, 0.0f};

    glBindBuffer(GL_ARRAY_BUFFER, pvbo);
    glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float), 3 * sizeof(float), &ce);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  { /// Camera
    int iw, ih;
    memset(&e->initCam, 0, sizeof(e->initCam));
    glfwGetWindowSize(e->window, &iw, &ih);
    e->initCam.ratio = (float)iw / (float)ih;

    e->initCam.up.y = 1.0f;
    e->initCam.fov = toRadians(90.0f);
    e->initCam.pos = (vec3) { 0.0f, 60.0f, 0.0f };
    e->initCam.orientation = (quat) { 1.0f, 0.0f, 0.0f, 0.0f};
    memcpy(&e->cam, &e->initCam, sizeof(e->cam));
  }

  { /// Cursor
    glfwGetCursorPos(e->window, &mouseX, &mouseY);
    oldMouseX = 0.0;
    oldMouseY = 0.0;
  }

  { /// UI
#define TSL(node, name, namescale, var, mi, ma, fun, funp) \
      add_title(node, name, namescale, 5); \
      add_slider(node, var, mi, ma, 5, fun, funp)
#define UI_GET_HEIGHT(res, node) { uint32_t _ch = 0; int32_t _i; for(_i = 0; _i < (node).children.l; ++_i) { _ch += (node).children.v[_i].height + (node).children.v[_i].pad; } res = _ch + 10; }
    prep_ui(&uvao);
    // add_title(&nodes[0], "スプーク・プーク", 25, 0);
      //mchvi(&nodes[1].children);

    {
      mchvi(&nodes[1].children);
      add_title(&nodes[1], "#Clouds", 25, 8);
      add_tslider(&nodes[1], "Scale", 15, &scale, 1.1, 40.0f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "PScale", 15, &pscale, 1.1, 40.0f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "PWScale", 15, &pwscale, 1.1, 40.0f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "Persist", 15, &kmsper, 0.0, 2.0f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "Octaves", 15, &kmsoct, 1.0, 20.0f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "Channel", 15, &curch, 80.1, 83.1f, 8, NULL, NULL);
      add_tslider(&nodes[1], "minDensity", 15, &minDensity, 0.0, 1.0f, 8, NULL, NULL);
      add_tslider(&nodes[1], "densityMultiplier", 15, &densityMultiplier, 0.0, 4.0f, 8, NULL, NULL);
      add_checkbox(&nodes[1], "Dracu stie", 0xFF00FFFF, 15, 10, &drawTexture, NULL, NULL);
      add_tslider(&nodes[1], "Type", 15, &KMS, 0.1, 3.1f, 8, autoupda, NULL);
      add_tslider(&nodes[1], "Curslice", 15, &curslcs, 0.0, 1.0f, 8, NULL, NULL);
      add_button(&nodes[1], "Update", 0xFF0000FF, 20, 10, compute_shader, NULL);
      add_tslider(&nodes[1], "Autoupdate", 15, &kkautoupda, 0.1, 1.9f, 8, NULL, NULL);
      nodes[1].px = 400.0f; nodes[1].py = 50.0f; nodes[1].sx = 400.0f;
      UI_GET_HEIGHT(nodes[1].sy, nodes[1]);
      nodes[1].bp = nodes[1].tp = nodes[1].rp = 0; nodes[1].lp = 8;
      mchvt(&nodes[1].children);
    }

    {
      mchvi(&nodes[2].children);
      add_title(&nodes[2], "10.0", 25, 8);
      nodes[2].children.v[0].fg = 0x10FF00FF;
      nodes[2].children.v[0].bg = 0xF0000000;
      mchvt(&nodes[2].children);
    }
  }


  {
    init_skybox(&sb);
    glGenTextures(1, &sbt);
    uint32_t sbw, sbh;
    uint8_t *buf = read_png("skybox.png", "Resources/Textures", &sbw, &sbh);
    update_rgb_tex(&sbt, sbh, sbw, buf);
    free(buf);
  }

  {
    struct stat s;
    stat("shaders/cloud_frag.glsl", &s); cla = s.st_ctim.tv_sec;
    stat("shaders/worley_compute.glsl", &s); ccla = s.st_ctim.tv_sec;
  }

  init_clouds(&cl);
  compute_shader();

  { /// Init therm
    per.w    = per.h    = 500;
    per.o1.h = per.o1.w = 500;
    per.sc = 28.5;
    per.per = 1.0f;
    per.foct = 3.5f;
    init_therm(NULL);
  }

  //selui.nt = 0xFF;
  _ctime = _ltime = deltaTime = dt = 0;
  glfwSetInputMode(e->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  return 0;
}

uint8_t suijin_run() {
  ++e->frame;
  _ctime = glfwGetTime();
  deltaTime = _ctime - _ltime;
  dt += deltaTime;
  if (e->frame % FRAME_UPDATE == 0) {
    snprintf(((struct tbox*__restrict)nodes[2].children.v[0].c)->text, 10, "%.2f", FRAME_UPDATE / dt);
    get_textbox_size(nodes[2].children.v[0].c);
    fprintf(stdout, "Frametime %f       \r", FRAME_UPDATE / dt);
    dt = 0;
  }

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  handle_input();

  if (e->camUpdate) {
    update_camera_matrix();
    e->camUpdate = 0;
  }

  { /// Skybox
    glUseProgram(skyprog);
    program_set_mat4(skyprog, "fn", fn);
    sb.m.pos = e->cam.pos;
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
    program_set_float3v(nprog, "camPos", e->cam.pos);

    int32_t i, j;
    float rd, mrd = NEG_INF;

    if (canMoveCam && glfwGetMouseButton(e->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      cs = csrayd(e->cam.pos, QV(e->cam.orientation), MPROP);
    }

    for(i = 0; i < e->objs.l; ++i) {
      for(j = 0; j < e->objs.v[i].mins.l; ++j) {
        draw_model(nprog, &e->mats, e->mods.v + e->objs.v[i].mins.v[j].m, e->objs.v[i].mins.v[j].aff);
        if (canMoveCam && glfwGetMouseButton(e->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
          if (rbi(&cs, &e->objs.v[i].mins.v[j].b)) {
            rmi(&cs, &e->objs.v[i].mins.v[j], &rd);
            if (rd > mrd && rd > NEG_INF) {
              mrd = rd;
              HITS = 1;
              HITP = v3a(e->cam.pos, v3m(rd, QV(e->cam.orientation)));
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

  if (drawTerm) { draw_squaret((e->w.windowW - 500) / 2, (e->w.windowH - 500) / 2, 500, 500, ttex, 2); upd_therm(); }

  glDisable(GL_DEPTH_TEST);
  if (drawClouds) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(cloudprog);
    program_set_mat4(cloudprog, "fn", fn);
    program_set_mat4(cloudprog, "affine", cl.m.aff);
    program_set_float3v(cloudprog, "centerScale", cl.m.scale);
    program_set_float3v(cloudprog, "centerPos", cl.m.pos);
    program_set_float3v(cloudprog, "camPos", e->cam.pos);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, cl.pwor.t);
    program_set_int1(cloudprog, "tex", 0);

    glBindVertexArray(cloudvao);
    glDrawArrays(GL_TRIANGLES, 0, cbvl);
    glDisable(GL_CULL_FACE);
  }

  if (drawUi) { // UPROG
    glUseProgram(uprog);

    int32_t i;
    for(i = 0; i < MC; ++i) { draw_node(i); }
    if (drawTexture) { draw_squaret3((e->w.windowW - 500) / 2 - 275, (e->w.windowH - 500) / 2 - 275, 500, 500, cl.pwor.t, curch, curslcs); }
    // if (selui.t == UIT_CAN) { selui.t = UIT_CANNOT; }
  }

  if (drawFps) {
    draw_textbox(e->w.windowW - ((struct tbox*__restrict)nodes[2].children.v[0].c)->tlen, 10, nodes[2].children.v + 0, nodes[2].children.v[0].c);
  }

  if (e->frame % 30 == 0) { /// Frag update
    check_shader_update(&cloudprog, "shaders/cloud_frag.glsl", GL_FRAGMENT_SHADER, shaders[12], &cla);
    check_shader_update(&wComp, "shaders/worley_compute.glsl", GL_COMPUTE_SHADER, 0, &ccla);
  }

  glfwPollEvents();
  glfwSwapBuffers(e->window);
  _ltime = _ctime;
  return 0;
}

uint8_t suijin_end() {
  free(linsv);

  /*{
    int32_t i;
    for(i = 0; i < e->mods.l; ++i) {
      destroy_model(e->mods.v + i);
    }
    free(e->mods.v);
    free(e->mats.v);
  }*/

  glfwTerminate(); /// This for some reason crashes but exiting without it makes the drivers crash?????????
                   /// Update: This no longer happens??????

  return 0;
}

void suijin_unload(void) { }

void suijin_reload(void) {
  if (!e) {
    fprintf(stderr, "Env not passed to suijin!\n");
    exit(1);
  }
}

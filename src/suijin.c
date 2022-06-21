#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

struct pos {
  float x;
  float y;
  float z;
  float w;
};

#define GL_CHECK(call, message) if (!(call)) { fputs(message "! Aborting...\n", stderr); glfwTerminate(); exit(1); }
#define PR_CHECK(call) if (call) { return 1; }
#define KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

double _ctime, _ltime;
double deltaTime;

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

uint8_t cameraUpdate = 1;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 0.04f
#define SENS 0.001f

mat4 fn;

void read_uint32_t(FILE *__restrict stream, uint32_t *__restrict nr) {
  uint8_t ch;
  *nr = 0;
  while ((ch = fgetc(stream)) && ('0' <= ch && ch <= '9')) {
    *nr *= 10;
    *nr += ch - '0';
  }
  if (ch == '\r') {
    fgetc(stream);
  }
}

uint32_t __inline__ __attribute((pure)) max(uint32_t o1, uint32_t o2) {
  return o1 > o2 ? o1 : o2;
}

void print_quat(quat q, const char *str) {
  fprintf(stdout, "%s: x = %f y = %f z = %f w = %f\n", str, q.x, q.y, q.z, q.w);
}

void print_vec3(vec3 v, const char *str) {
  fprintf(stdout, "%s: x = %f y = %f z = %f\n", str, v.x, v.y, v.z);
}

void print_mat3(mat3 m, const char *name) {
  fprintf(stdout, "Printing %s\n", name);
  int32_t i, j;
  for (i = 0; i < 3; ++i) {
    for (j = 0; j < 3; ++j) {
      fprintf(stdout, "%f ", m[i][j]);
    }
    fputc('\n', stdout);
  }
}

void print_mat(mat4 m, const char *name) {
  fprintf(stdout, "Printing %s\n", name);
  int32_t i, j;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      fprintf(stdout, "%f ", m[i][j]);
    }
    fputc('\n', stdout);
  }
}

struct keypress { /// TODO: Organise
  int32_t key;
  int32_t scancode;
  int32_t action;
  int32_t mod;
};

#define CKQL 256
struct ckq {
  struct keypress v[CKQL];
  uint32_t s;
  uint32_t e;
};

void ckpush(struct ckq *__restrict cq, struct keypress *__restrict kp) {
  cq->v[cq->e] = *kp;
  ++cq->e;
  cq->e %= CKQL;
}

struct keypress cktop(struct ckq *__restrict cq) {
  struct keypress kp = cq->v[cq->s];
  ++cq->s;
  cq->s %= CKQL;
  return kp;
}

void callback_error(int error, const char *desc) {
    fprintf(stderr, "GLFW error, no %i, desc %s\n", error, desc);
}

void callback_window_should_close(GLFWwindow *window) {
}

void callback_window_resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    initCam.ratio = cam.ratio = (float) width / (float) height;
    cameraUpdate = 1;

    fprintf(stdout, "Window Resize To: %ix%i\n", width, height);
}

void init_random() {
  uint32_t seed = 0;
  uint32_t randfd = open("/dev/urandom", O_RDONLY);
  if (randfd == -1) {
    fputs("Could not open /dev/urandom, not initalizing crng!\n", stderr);
    return;
  }
  if (read(randfd, &seed, sizeof(seed)) == -1) {
    fputs("Could not initalize crng!\n", stderr);
    return;
  }
  srand(seed);
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

float __attribute((pure)) __inline__ toRadians(float o) {
  return o * M_PI / 180.0f;
}

uint8_t __attribute((pure)) __inline__ epsilon_equals(float o1, float o2) {
  return fabsf(o1 - o2) < 0.000000001;
}

float rfloat(float lb, float ub) {
  return ((float)rand() / (float)RAND_MAX) * (ub - lb) + lb;
}

float __attribute((pure)) __inline__ lerp(float bottomLim, float topLim, float progress) {
  return (topLim - bottomLim) * progress + bottomLim;
}

float __attribute((pure)) clamp(float val, float lb, float ub) {
  if (val <= lb) {
    return lb;
  }
  if (val >= ub) {
    return ub;
  }
  return val;
}

struct pos matmul(mat3 mat,struct pos p) {
  struct pos res = {0};
  res.x = p.x * mat[0][0] + p.y * mat[0][1] + mat[0][2];
  res.y = p.x * mat[1][0] + p.y * mat[1][1] + mat[1][2];
  return res;
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

vec3 __inline__ __attribute((pure)) *__restrict qv(quat *__restrict q) {
  return (v3 *__restrict) q;
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

void print_keypress(char *__restrict ch, struct keypress kp) {
  fprintf(stdout, "%s%u %u %u %u\n", ch, kp.key, kp.scancode, kp.action, kp.mod);
}

struct ckq keypresses = {0};
void input_callback(GLFWwindow *window, int key, int scancode, int action, int mod) {
  struct keypress kp;
  kp.key = key;
  kp.scancode = scancode;
  kp.action = action;
  kp.mod = mod;
  ckpush(&keypresses, &kp);
  //print_keypress("Got keypress: ", kp);
}

uint32_t CDR, CDRO;
uint32_t lp[3];
void handle_input(GLFWwindow *__restrict window) {
  { 
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if ((mouseX != oldMouseX) || (mouseY != oldMouseY)) {
      double xoff = mouseX - oldMouseX;
      double yoff = mouseY - oldMouseY;

      oldMouseX = mouseX;
      oldMouseY = mouseY;

      quat qx = gen_quat(cam.up, -xoff * SENS);
      quat qy = gen_quat(cross(*qv(&cam.orientation), cam.up), yoff * SENS);

      quat qr = qmul(qy, qx);
      quat qt = qmul(qmul(qr, cam.orientation), qconj(qr));
      qt = qnorm(qt);
      //print_quat(cam.orientation, "qt");
      cam.orientation = qt;

      cameraUpdate = 1;
    }
  }

  struct keypress kp;
  while (keypresses.s != keypresses.e) {
    kp = cktop(&keypresses);
    //print_keypress("Processing keypress: ", kp);
    switch(kp.key) {
      case GLFW_KEY_G:
        lp[0] = kp.action == 1;
        break;
      case GLFW_KEY_T:
        if (kp.action == 1) {
          lp[1] = 1 - lp[1];
        }
        break;
      default:
        break;
    }
  }

  {
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
  stat("shaders/frag.glsl", &s);
  if (s.st_ctim.tv_sec != la) {
    uint32_t shaders[2];
    shaders[0] = vert;
    if (shader_get("shaders/frag.glsl", GL_FRAGMENT_SHADER, shaders + 1)) {
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
  //fprintf(stdout, "Using material %s\n", mat->name);
  //fprintf(stdout, "Mat tex: %u\n", mat->tamb.i);
  program_set_float3v(program, "mat.ambient", mat->ambient);
  program_set_float3v(program, "mat.diffuse", mat->diffuse);
  program_set_float3v(program, "mat.spec", mat->spec);
  program_set_float3v(program, "mat.emmisive", mat->emmisive);
  program_set_float1(program, "mat.spece", mat->spece);
  program_set_float1(program, "mat.transp", mat->transp);
  program_set_float1(program, "mat.optd", mat->optd);
  program_set_int1(program, "mat.illum", mat->illum);
}

void draw_obj(uint32_t program, struct matv *__restrict mats, struct object *__restrict obj) {
  //fprintf(stdout, "Drawing %s: %u\n", obj->name, obj->vao);
  //update_affine_matrix(obj); // TODO: AAAAA
  uint32_t li = 0;
  struct material *__restrict cmat;

  glBindVertexArray(obj->vao);

  program_set_mat4(program, "affine", obj->aff);

  cmat = &mats->v[obj->m.v[0].m];
  update_mat(program, cmat);
  
#ifdef DUMP_BUF
  double tempBuffer[40];
  memset(tempBuffer, 0, sizeof(tempBuffer));
  glGetBufferSubData(GL_ARRAY_BUFFER, 0, 40, tempBuffer);
  fprintf(stdout, "Dumping buffer of size %u: ", obj->v.l);
  {
    int32_t i;
    for(i = 0; i < 40; ++i) {
      fprintf(stdout, "%f ", tempBuffer[i]);
    }
    fputc('\n', stdout);
  }
#endif

  {
    int32_t i;
    for(i = 1; i < obj->m.l; ++i) {
      if (cmat->tamb.i != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmat->tamb.i);
        program_set_int1(program, "tex", 0);
        program_set_int1(program, "hasTexture", 1);
      } else {
        program_set_int1(program, "hasTexture", 0);
      }
      
      //fprintf(stdout, "Drawing from %u to %u\n", li / 8, (obj->m.v[i].i - li) / 8);
      glDrawArrays(GL_TRIANGLES, li / 8, (obj->m.v[i].i - li) / 8);
      li = obj->m.v[i].i;

      cmat = &mats->v[obj->m.v[i].m];
      update_mat(program, cmat);
    }
  }

  if (cmat->tamb.i != 0) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmat->tamb.i);
    program_set_int1(program, "tex", 0);
    program_set_int1(program, "hasTexture", 1);
  } else {
    program_set_int1(program, "hasTexture", 0);
  }
  //fprintf(stdout, "Drawing from %u to %u\n", li / 8, (obj->v.l - li) / 8);
  glDrawArrays(GL_TRIANGLES, li / 8, (obj->v.l - li) / 8);
}

uint8_t run_suijin() {
  init_random();
  setbuf(stdout, NULL);
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();

  glfwSetKeyCallback(window, input_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  struct matv mats;
  struct objv objects;
  matvi(&mats);
  objvi(&objects);
  parse_folder(&objects, &mats, "Items/Mountain");
  //parse_folder(&objects, &mats, "Items/Dough");
  //parse_folder(&objects, &mats, "Items/Plane");

  objvt(&objects);

  uint32_t program;

  _ctime = _ltime = deltaTime = 0;

#define SHADERC 2
  uint32_t shaders[SHADERC];
  {
    const char          *paths[SHADERC] = { "shaders/vert.glsl", "shaders/frag.glsl" };
    const uint32_t shaderTypes[SHADERC] = { GL_VERTEX_SHADER   , GL_FRAGMENT_SHADER  };
    uint32_t i;
    for (i = 0; i < SHADERC; ++i) {
      PR_CHECK(shader_get(paths[i], shaderTypes[i], shaders + i));
    }

    PR_CHECK(program_get(2, shaders, &program));

    for (i = 1; i < SHADERC; ++i) { // TODO: REMOVE
      glDeleteShader(shaders[i]);
    }
  }

  {
    struct stat s;
    stat("shaders/frag.glsl", &s);
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
  initCam.pos = (vec3) { 60.0f, 60.0f, 30.0f };
  initCam.orientation = (quat) { 0.819f, 0.353f, 0.453f, 0.0f};

  memcpy(&cam, &initCam, sizeof(cam));

  glPointSize(5.0);
  glLineWidth(3.0);

  glEnable(GL_DEPTH_TEST);

  glfwGetCursorPos(window, &mouseX, &mouseY);
  oldMouseX = 0.0;
  oldMouseY = 0.0;

  uint32_t frame = 0;

  while (!glfwWindowShouldClose(window)) {
    ++frame;
    _ctime = glfwGetTime();
    fprintf(stdout, "%2.3f - %2.3f %2.3f %2.3f - %2.3f %2.3f %2.3f %2.3f\r", cam.fov, cam.pos.x, cam.pos.y, cam.pos.z, cam.orientation.x, cam.orientation.y, cam.orientation.z, cam.orientation.w);
    deltaTime = _ctime - _ltime;

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    handle_input(window);

    if (cameraUpdate) {
      update_camera_matrix();
      cameraUpdate = 0;
    }

    glUseProgram(program);

    program_set_mat4(program, "fn_mat", fn);
    program_set_int1(program, "shading", cshading);
    program_set_float3v(program, "camPos", cam.pos);

    if (lp[1]) {
      objects.v[0].scale += 0.01f;
      update_affine_matrix(&objects.v[0]);
    }

    {
      int32_t i;
      for(i = 0; i < objects.l; ++i) {
        draw_obj(program, &mats, objects.v + i);
      }
    }

    if (frame % 30 == 0) {
      while (1) {
        uint8_t r = check_frag_update(&program, shaders[0]);
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
    //fprintf(stdout, "Fps: %3.5f\r", 1 / deltaTime);
  }

  glfwTerminate();

  return 0;
}

int main(int argc, char **argv) {
  GL_CHECK(run_suijin() == 0, "Running failed!");
  
  return 0;
}

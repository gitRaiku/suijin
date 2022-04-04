#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "shaders.h"
#include "linalg.h"

struct pos {
  float x;
  float y;
  float z;
  float w;
};

struct col {
  float r;
  float g;
  float b;
  float a;
};
#define COL_WHITE ((struct col) {1.0f, 1.0f, 1.0f, 0.2f})
#define COL_BLACK ((struct col) {0.0f, 0.0f, 0.0f, 0.2f})

struct pixel {
  struct pos p;
  struct col c;
};

#define GL_CHECK(call, message) if (!(call)) { fputs(message "! Aborting...\n", stderr); glfwTerminate(); exit(1); }
#define PR_CHECK(call) if (call) { return 1; }
#define KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

float COEF = 0.8f;
double ctime, ltime;
double deltaTime;
float mu = 1.5f;
float g = -9.8f;

//struct pixel pixels[2000] = {0};
struct pixel pixels[2000] = {0};
uint32_t pcount = sizeof(pixels) / sizeof(pixels[0]);

#define MAX_DIVISIONS 40
#define MIN_DIVISIONS 20

#define C0 0.5f
#define C1 0.2f /// TODO: FIX COLOUR INTENSITY
#define C2 0.2f

float grid_spacing = M_PI;
struct pixel lines[MAX_DIVISIONS * 4] = {0};

struct camera {
  vec3 pos;
  vec3 up;
  quat orientation;
  float fov;
  float yaw;
  float pitch;
  float roll;
  float ratio;
};

double mouseX, mouseY;
double oldMouseX, oldMouseY;

struct camera initCam;
struct camera cam;

uint8_t cameraUpdate = 1;
uint8_t showGrid = 0;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 0.1f
#define SENS 0.001f

uint32_t pvao, pvbo;
uint32_t lvao, lvbo;

mat4 fn;

void print_vec3(vec3 v, const char *str) {
  fprintf(stdout, "%s: x = %f y = %f z = %f\n", str, v.x, v.y, v.z);
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

void init_points() {
  int32_t i;
  for(i = 0; i < pcount; ++i) {
    pixels[i].p.x = rfloat(-18.0f, 18.0f);
    pixels[i].p.y = rfloat(-18.0f, 18.0f);
    pixels[i].p.z = rfloat(-18.0f, 18.0f);
    pixels[i].p.w = 1;

    pixels[i].c.r = rfloat(0.0f, 1.0f);
    pixels[i].c.g = rfloat(0.0f, 1.0f);
    pixels[i].c.b = rfloat(0.0f, 1.0f);
    pixels[i].c.a = 1.0f;

    //fprintf(stdout, "Point: %fx %fy (%f, %f, %F, %f)\n", pixels[i].p.x, pixels[i].p.y, pixels[i].c.r, pixels[i].c.g, pixels[i].c.b, pixels[i].c.a);
  }

  /*{
#define cp pixels[i * 100 + j * 10 + k]
    int32_t i, j, k;
    for (i = 0; i < 10; ++i) {
      for (j = 0; j < 10; ++j) {
        for (k = 0; k < 10; ++k) {
          cp.p.x = (float)i;
          cp.p.y = (float)j;
          cp.p.z = (float)k;
          cp.p.w = 1.0f;

          cp.c.r = i / 10.0f;
          cp.c.g = j / 10.0f;
          cp.c.b = k / 10.0f;
          cp.c.a = 1.0f;
          //fprintf(stdout, "pixels[%u] = %f %f %f %f - %f %f %f %f\n", i * 100 + j * 10 + k, cp.p.x, cp.p.y, cp.p.z, cp.p.w, cp.c.r, cp.c.g, cp.c.b, cp.c.a);
        }
      }
    }
#undef cp
  }*/
}

struct pos update_pos(struct pos p) {
  struct pos res;
  //res.x = p.x + p.y * deltaTime;
  //res.y = p.y + (-mu * p.y + g * sinf(p.x)) * deltaTime;
  
  float a = 0.95f;
  float b = 0.7f;
  float c = 0.6f;
  float d = 3.5f;
  float e = 0.25f;
  float f = 0.1f;
  float dx = (p.z - b) * p.x - d * p.y;
  float dy = d * p.x + (p.z - b) * p.y;
  float dz = c + a * p.z - (p.z * p.z * p.z) / 3.0f - (p.x * p.x) + (1.0f + e * p.z) + f * p.z * (p.x * p.x * p.x);

  res.x = p.x + dx * deltaTime;
  res.y = p.y + dy * deltaTime;
  res.z = p.z + dz * deltaTime;
  res.w = p.w;
  return res;
}

void update_points() {
  int32_t i;
  for(i = 0; i < pcount; ++i) {
    pixels[i].p = update_pos(pixels[i].p);
  }
}

struct pos matmul(mat3 mat,struct pos p) {
  struct pos res = {0};
  res.x = p.x * mat[0][0] + p.y * mat[0][1] + mat[0][2];
  res.y = p.x * mat[1][0] + p.y * mat[1][1] + mat[1][2];
  return res;
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
  return (struct vec3 *__restrict) q;
}

void create_lookat_matrix(mat4 m) {
  vec3 f = norm(*qv(&cam.orientation));
  vec3 s = cross(cam.up, f);
  vec3 u = cross(f, s);
  print_vec3(f, "f");
  print_vec3(s, "s");
  print_vec3(u, "u");
  print_vec3(cam.pos, "cam.pos");

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
  print_mat(m, "lookat");
}

void update_camera_matrix() {
  mat4 proj, view;
  if (cam.up.x == cam.orientation.x && cam.up.y == cam.orientation.y && cam.up.z == cam.orientation.z) {
    cam.orientation.x = 0.0000000000000001;
  }
  create_projection_matrix(proj, cam.fov, cam.ratio, 0.01f, 1000.0f);
  create_lookat_matrix(view);

  //matmul44(fn, view, proj);
  matmul44(fn, proj, view);
}

void handle_input(GLFWwindow *__restrict window) {
  { 
    glfwGetCursorPos(window, &mouseX, &mouseY);

    //if ((mouseX != oldMouseX) || (mouseY != oldMouseY)) {
      double xoff = mouseX - oldMouseX;
      double yoff = mouseY - oldMouseY;

      oldMouseX = mouseX;
      oldMouseY = mouseY;
      //xoff = 5;
  
      //fprintf(stdout, "xoff: %f yoff: %f\n", xoff, yoff);

      quat qx = gen_quat(norm((struct vec3) {0.0f, 1.0f, 0.0f}), -xoff * SENS);
      quat qy = gen_quat(norm((struct vec3) {1.0f, 0.0f, 0.0f}), -yoff * SENS);
      //quat qr = qmul(qx, qy);
      quat qr = qmul(qy, qx);
      quat qt = qmul(qmul(qr, cam.orientation), qconj(qr));
      /*fprintf(stdout, "Gen q: %f %f %f %f\n", q.w, q.x, q.y, q.z);
      fprintf(stdout, "Cam q: %f %f %f %f -> %f %f %f %f\n", cam.orientation.w, cam.orientation.x, cam.orientation.y, cam.orientation.z, qt.w, qt.x, qt.y, qt.z);*/
      cam.orientation = qt;

      cameraUpdate = 1;
    //}
  }

  {
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
      cam.pos = v3a(v3m(PAN_SPEED, norm(cross(*qv(&cam.orientation), cam.up))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_D)) {
      cam.pos = v3a(v3m(PAN_SPEED, v3n(norm(cross(*qv(&cam.orientation), cam.up)))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_SPACE)) {
      cam.pos = v3a(v3m(PAN_SPEED, cam.up), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_LEFT_CONTROL)) {
      cam.pos = v3a(v3m(PAN_SPEED, v3n(cam.up)), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_W)) {
      cam.pos = v3a(v3m(PAN_SPEED, v3n(*qv(&cam.orientation))), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_S)) {
      cam.pos = v3a(v3m(PAN_SPEED, *qv(&cam.orientation)), cam.pos);
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_R)) {
      init_points();
    }
    if (KEY_PRESSED(GLFW_KEY_U)) {
      memcpy(&cam, &initCam, sizeof(cam));
      cameraUpdate = 1;
    }
    if (KEY_PRESSED(GLFW_KEY_P)) {
      showGrid ^= 1;
    }
  }
}

void update_grid_lines() {
  /*while (camSize[1] / grid_spacing > MAX_DIVISIONS / 3.5f) {
    grid_spacing *= 2.0f;
  }
  while (camSize[1] / grid_spacing < MIN_DIVISIONS / 3.5f) {
    grid_spacing /= 2.0f;
  }

  float npos =  ceil((camPos[1] + camSize[1] / 2.0f) / grid_spacing) * grid_spacing;
  float spos = floor((camPos[1] - camSize[1] / 2.0f) / grid_spacing) * grid_spacing;
  float epos =  ceil((camPos[0] + camSize[0] / 2.0f) / grid_spacing) * grid_spacing;
  float wpos = floor((camPos[0] - camSize[0] / 2.0f) / grid_spacing) * grid_spacing;
  ///fprintf(stdout, "North: %f\nSouth: %f\nEast: %f\nWest: %f\n", npos, spos, epos, wpos);
  int32_t i;
  float cpos;
  float cintensity;
  for(i = 0; i < MAX_DIVISIONS; ++i) {
    cpos = wpos + i * grid_spacing;
    if (epsilon_equals(cpos, 0.0f)) {
      cintensity = C0;
    } else if ((int32_t)(cpos / grid_spacing) % 5 == 0) {
      cintensity = C1;
    } else {
      cintensity = C2;
    }
    lines[2 * i    ] = (struct pixel) {{ cpos, npos }, {1.0f, 1.0f, 1.0f, cintensity }};
    lines[2 * i + 1] = (struct pixel) {{ cpos, spos }, {1.0f, 1.0f, 1.0f, cintensity }};
  }
  for(i = 0; i < MAX_DIVISIONS; ++i) {
    cpos = spos + i * grid_spacing;
    if (epsilon_equals(cpos, 0.0f)) {
      cintensity = C0;
    } else if ((int32_t)(cpos / grid_spacing) % 5 == 0) {
      cintensity = C1;
    } else {
      cintensity = C2;
    }
    lines[MAX_DIVISIONS + 2 * i    ] = (struct pixel) {{ epos, cpos }, {1.0f, 1.0f, 1.0f, cintensity }};
    lines[MAX_DIVISIONS + 2 * i + 1] = (struct pixel) {{ wpos, cpos }, {1.0f, 1.0f, 1.0f, cintensity }};
  }

  glBindBuffer(GL_ARRAY_BUFFER, lvbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);*/
}

void print_quat(quat q, const char *str) {
  fprintf(stdout, "%s: x = %f y = %f z = %f w = %f\n", str, q.x, q.y, q.z, q.w);
}

uint8_t run_suijin() {
  init_random();
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  uint32_t program;

  ctime = ltime = deltaTime = 0;

  {
#define SHADERC 2
    const char          *paths[SHADERC] = { "shaders/vert.glsl", "shaders/frag.glsl" };
    const uint32_t shaderTypes[SHADERC] = { GL_VERTEX_SHADER   , GL_FRAGMENT_SHADER  };
    uint32_t shaders[SHADERC];
    uint32_t i;
    for (i = 0; i < SHADERC; ++i) {
      PR_CHECK(shader_get(paths[i], shaderTypes[i], shaders + i));
    }

    PR_CHECK(program_get(2, shaders, &program));

    for (i = 0; i < SHADERC; ++i) {
      glDeleteShader(shaders[i]);
    }
  }

  glGenVertexArrays(1, &pvao);
  glGenVertexArrays(1, &lvao);
  glGenBuffers(1, &pvbo);
  glGenBuffers(1, &lvbo);
  { 
    glBindVertexArray(pvao);

    glBindBuffer(GL_ARRAY_BUFFER, pvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pixels), pixels, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) sizeof(struct pos));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
  }

  { 
    glBindVertexArray(lvao);

    glBindBuffer(GL_ARRAY_BUFFER, lvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) sizeof(struct pos));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
  }

  memset(&initCam, 0, sizeof(initCam));

  {
    int iw, ih;
    glfwGetWindowSize(window, &iw, &ih);
    initCam.ratio = (float)iw / (float)ih;
  }

  initCam.up.y = 1.0f;
  initCam.fov = toRadians(90.0f);
  initCam.yaw = toRadians(0.0f);
  initCam.pitch = toRadians(0.0f);
  initCam.pos = (vec3) { 5.0f, 5.0f, 5.0f };
  initCam.orientation = (quat) { 0.707f, 0.707f, 0.0f, 0.0f};

  memcpy(&cam, &initCam, sizeof(cam));

  glPointSize(5.0);
  glLineWidth(3.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_PROGRAM_POINT_SIZE);

  init_points();

  glfwGetCursorPos(window, &mouseX, &mouseY);
  oldMouseX = 0.0;
  oldMouseY = 0.0;

  while (!glfwWindowShouldClose(window)) {
    ctime = glfwGetTime();
    deltaTime = ctime - ltime;

    handle_input(window);
    
    //fscanf(stdin, "%f%f%f", &cam.orientation.x, &cam.orientation.y, &cam.orientation.z);
    if (cam.orientation.x == 0.0) {
      //cam.orientation.x = 0.00000001;
    }
    

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    if (cameraUpdate) {
      update_camera_matrix();
      update_grid_lines();
      cameraUpdate = 0;
    }

    program_set_mat4(program, "fn_mat", fn);

    if (showGrid) {
      glBindVertexArray(lvao);
      glDrawArrays(GL_LINES, 0, sizeof(lines) / sizeof(lines[0]));
    }

    update_points();

    {
      glBindVertexArray(pvao);

      glBindBuffer(GL_ARRAY_BUFFER, pvbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pixels), pixels);

      glDrawArrays(GL_POINTS, 0, pcount);
    }

    glfwPollEvents();
    glfwSwapBuffers(window);
    ltime = ctime;
  }

  glfwTerminate();

  return 0;
}

int main(int argc, char **argv) {
  GL_CHECK(run_suijin() == 0, "Running failed!");

  return 0;
}

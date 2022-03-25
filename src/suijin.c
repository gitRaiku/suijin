#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "shaders.h"

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
struct pixel pixels[1000] = {0};
uint32_t pcount = sizeof(pixels) / sizeof(pixels[0]);

#define MAX_DIVISIONS 40
#define MIN_DIVISIONS 20
struct pixel lines[MAX_DIVISIONS * 4] = {0};

float initCamPos[2]  = {0};
float camPos[2]      = {0};
float initCamSize[2] = {0};
float camSize[2]     = {0};

uint8_t cameraUpdate = 1;
uint8_t showGrid = 0;
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 0.0003f

uint32_t pvao, pvbo;
uint32_t lvao, lvbo;
float grid_spacing = M_PI;

mat4 tl, rs, fn;
mat4 proj_mat;

#define C0 0.5f
#define C1 0.2f /// TODO: FIX COLOUR INTENSITY
#define C2 0.2f

#define INIT_RES_COEF 0.07f


void callback_error(int error, const char *desc) {
    fprintf(stderr, "GLFW error, no %i, desc %s\n", error, desc);
}

void callback_window_should_close(GLFWwindow *window) {
}

void callback_window_resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    float wc = initCamSize[0] / camSize[0];
    float hc = initCamSize[1] / camSize[1];
    initCamSize[0] = (float)width * wc * INIT_RES_COEF;
    initCamSize[1] = (float)height * hc * INIT_RES_COEF;
    memcpy(camSize, initCamSize, sizeof(camSize));
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

uint8_t __attribute((pure)) __inline__ epsilon_equals(float o1, float o2) {
  return fabsf(o1 - o2) < 0.000000001;
}

float rfloat(float lb, float ub) {
  return ((float)rand() / (float)RAND_MAX) * (ub - lb) + lb;
}

float __attribute((pure)) __inline__ lerp(float bottomLim, float topLim, float progress) {
  return (topLim - bottomLim) * progress + bottomLim;
}

void matmul44(mat4 res, mat4 m1, mat4 m2) {
  int32_t i, j, k;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      res[i][j] = 0.0f;
      for(k = 0; k < 4; ++k) {
        res[i][j] += m1[i][k] * m2[k][j];
      }
    }
  }
}

struct pos mat44x4(mat4 m, struct pos p) {
  struct pos res;
  res.x = p.x * m[0][0] + p.y * m[0][1] + p.z * m[0][2] + p.w * m[0][3];
  res.y = p.x * m[1][0] + p.y * m[1][1] + p.z * m[1][2] + p.w * m[1][3];
  res.z = p.x * m[2][0] + p.y * m[2][1] + p.z * m[2][2] + p.w * m[2][3];
  res.w = p.x * m[3][0] + p.y * m[3][1] + p.z * m[3][2] + p.w * m[3][3];

  if (!epsilon_equals(res.w, 1.0f)) {
    res.x /= res.w;
    res.y /= res.w;
    res.z /= res.w;
    res.w = 1.0f;
  }
  return res;
}

void init_points() {
  //int32_t i;
  /*for(i = 0; i < pcount; ++i) {
    pixels[i].p.x = rfloat(-18.0f, 18.0f);
    pixels[i].p.y = rfloat(-18.0f, 18.0f);
    pixels[i].p.z = rfloat(-18.0f, 18.0f);
    pixels[i].p.w = 1;

    pixels[i].c.r = rfloat(0.0f, 1.0f);
    pixels[i].c.g = rfloat(0.0f, 1.0f);
    pixels[i].c.b = rfloat(0.0f, 1.0f);
    pixels[i].c.a = 1.0f;

    //fprintf(stdout, "Point: %fx %fy (%f, %f, %F, %f)\n", pixels[i].p.x, pixels[i].p.y, pixels[i].c.r, pixels[i].c.g, pixels[i].c.b, pixels[i].c.a);
  }*/

  {
    int32_t i, j, k;
    for (i = 0; i < 10; ++i) {
      for (j = 0; j < 10; ++j) {
        for (k = 0; k < 10; ++k) {
          pixels[i * 100 + j * 10 + k].p.x = (float)i;
          pixels[i * 100 + j * 10 + k].p.y = (float)j;
          pixels[i * 100 + j * 10 + k].p.z = (float)k;
          pixels[i * 100 + j * 10 + k].p.w = 1.0f;

          pixels[i * 100 + j * 10 + k].c.r = i / 10.0f;
          pixels[i * 100 + j * 10 + k].c.g = j / 10.0f;
          pixels[i * 100 + j * 10 + k].c.b = k / 10.0f;
          pixels[i * 100 + j * 10 + k].c.a = 1.0f;
        }
      }
    }
  }
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

void print_mat(mat3 m, const char *name) {
  fprintf(stdout, "Printing %s\n", name);
  int32_t i, j;
  for (i = 0; i < 3; ++i) {
    for (j = 0; j < 3; ++j) {
      fprintf(stdout, "%f ", m[i][j]);
    }
    fputc('\n', stdout);
  }
}

void create_projection_matrix(mat4 m, float angle, float ratio, float near, float far) {
  float tanHalfAngle = tanf(angle / 2.0f);
  mat4 proj = {{ 1.0f / (ratio * tanHalfAngle), 0.0f               , 0.0f                               ,  0.0f },
               { 0.0f                         , 1.0f / tanHalfAngle, 0.0f                               ,  0.0f },
               { 0.0f                         , 0.0f               , -(far + near) / (far - near)       , -1.0f },
               { 0.0f                         , 0.0f               , -(2.0f * far * near) / (far - near),  0.0f }};
  memcpy(m, proj, 4 * 4 * sizeof(float));
}

void update_camera_matrix() {
  tl[0][0] = 1.0f; tl[0][1] = 0.0f; tl[0][2] = -camPos[0]; tl[0][3] = 0.0f;
  tl[1][0] = 0.0f; tl[1][1] = 1.0f; tl[1][2] = -camPos[1]; tl[1][3] = 0.0f;
  tl[2][0] = 0.0f; tl[2][1] = 0.0f; tl[2][2] = 1.0f;       tl[2][3] = 0.0f;
  tl[3][0] = 0.0f; tl[3][1] = 0.0f; tl[3][2] = 0.0f;       tl[3][3] = 1.0f;

  rs[0][0] = 2.0f / camSize[0]; rs[0][1] =              0.0f; rs[0][2] = 0.0f; rs[0][3] = 0.0f;
  rs[1][0] =              0.0f; rs[1][1] = 2.0f / camSize[1]; rs[1][2] = 0.0f; rs[1][3] = 0.0f;
  rs[2][0] =              0.0f; rs[2][1] =              0.0f; rs[2][2] = 1.0f; rs[2][3] = 0.0f;
  rs[3][0] =              0.0f; rs[3][1] =              0.0f; rs[3][2] = 0.0f; rs[3][3] = 1.0f;

  create_projection_matrix(proj_mat, M_PI / 2, camSize[0] / camSize[1], 0.1f, 100.0f);

  mat4 temp;
  matmul44(temp, rs, tl);
  matmul44(fn, proj_mat, temp);
}

void handle_input(GLFWwindow *__restrict window) {
  if (KEY_PRESSED(GLFW_KEY_EQUAL) && KEY_PRESSED(GLFW_KEY_LEFT_SHIFT)) {
    camSize[0] -= camSize[0] * ZOOM_SPEED;
    camSize[1] -= camSize[1] * ZOOM_SPEED;
    cameraUpdate = 1;
  } else if (KEY_PRESSED(GLFW_KEY_EQUAL)) {
    memcpy(camSize, initCamSize, sizeof(camSize));
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_MINUS)) {
    camSize[0] += camSize[0] * ZOOM_SPEED;
    camSize[1] += camSize[1] * ZOOM_SPEED;
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_R)) {
    init_points();
  }
  if (KEY_PRESSED(GLFW_KEY_W)) {
    camPos[1] += camSize[1] * PAN_SPEED;
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_S)) {
    camPos[1] -= camSize[1] * PAN_SPEED;
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_A)) {
    camPos[0] -= camSize[0] * PAN_SPEED;
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_D)) {
    camPos[0] += camSize[0] * PAN_SPEED;
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_Q)) {
    memcpy(camPos, initCamPos, sizeof(camPos));
    cameraUpdate = 1;
  }
  if (KEY_PRESSED(GLFW_KEY_P)) {
    showGrid ^= 1;
  }
}

void update_grid_lines() {
  while (camSize[1] / grid_spacing > MAX_DIVISIONS / 3.5f) {
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
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);
}

uint8_t run_suijin() {
  init_random();
  GL_CHECK(glfwInit(), "Could not initialize glfw!");

  GLFWwindow *__restrict window = window_init();

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

  {
    int iw, ih;
    glfwGetWindowSize(window, &iw, &ih);
    initCamSize[0] = (float)iw * INIT_RES_COEF;
    initCamSize[1] = (float)ih * INIT_RES_COEF;
  }

  memcpy(camPos, initCamPos, sizeof(camPos));
  memcpy(camSize, initCamSize, sizeof(camSize));

  glPointSize(2.0);
  glLineWidth(1.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  init_points();

  while (!glfwWindowShouldClose(window)) {
    ctime = glfwGetTime();
    deltaTime = ctime - ltime;

    handle_input(window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    if (cameraUpdate) {
      update_camera_matrix();
      update_grid_lines();
      cameraUpdate = 0;
      /*fprintf(stdout, "Camera pos: %f %f\n", camPos[0], camPos[1]);
      fprintf(stdout, "Camera siz: %f %f\n", camSize[0], camSize[1]);
      print_mat(fn, "final");*/
    }

    program_set_mat4(program, "fn_mat", fn);

    if (showGrid) {
      glBindVertexArray(lvao);
      glDrawArrays(GL_LINES, 0, sizeof(lines) / sizeof(lines[0]));
    }

    //update_points();

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

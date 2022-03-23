#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "shaders.h"

#define GL_CHECK(call, message) if (!(call)) { fputs(message "! Aborting...\n", stderr); glfwTerminate(); exit(1); }
#define PR_CHECK(call) if (call) { return 1; }

void callback_error(int error, const char *desc) {
    fprintf(stderr, "GLFW error, no %i, desc %s\n", error, desc);
}

void callback_window_should_close(GLFWwindow *window) {
}

void callback_window_resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
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

struct pos {
  float x;
  float y;
  float z;
};

struct col {
  float r;
  float g;
  float b;
  float a;
};

struct pixel {
  struct pos p;
  struct col c;
};

float COEF = 0.8f;

double ctime, ltime;
double deltaTime;

void init_points(struct pixel *__restrict pixels, uint32_t pcount) {
  int32_t i;
  for(i = 0; i < pcount; ++i) {
    pixels[i].p.x = (((random() % 1000) / 500.0f) - 1.0f) * COEF;
    pixels[i].p.y = (((random() % 1000) / 500.0f) - 1.0f) * COEF;
    pixels[i].p.z = 1.0f;
    pixels[i].c.r = (random() % 1000) / 1000.0f;
    pixels[i].c.g = (random() % 1000) / 1000.0f;
    pixels[i].c.b = (random() % 1000) / 1000.0f;
    pixels[i].c.a = 1.0f;

    fprintf(stdout, "Point: %fx %fy (%f, %f, %F, %f)\n", pixels[i].p.x, pixels[i].p.y, pixels[i].c.r, pixels[i].c.g, pixels[i].c.b, pixels[i].c.a);
  }
}

/*
 * X = ang vel
 * Y = current angle
 */
float mu = 1.0f;
float g = -9.8f;
struct pos update_pos(struct pos p) {
  p.y += (p.x) * deltaTime;
  p.x += (-mu * p.x + g * sinf(p.y)) * deltaTime;
  return p;
}

void update_points(struct pixel *__restrict pixels, uint32_t pcount) {
  int32_t i;
  for(i = 0; i < pcount; ++i) {
    pixels[i].p = update_pos(pixels[i].p);
  }
}

//float camPos[2] = {1920.0f / 2.0f, 1080.0f / 2.0f};
float camPos[2] = {0.0f, 0.0f};
float camSize[2] = {10.0f, 10.0f};

mat3 tl;
mat3 rs;
mat3 fn;

void matmul33(mat3 res, mat3 m1, mat3 m2) {
  int32_t i, j, k;
  for (i = 0; i < 3; ++i) {
    for (j = 0; j < 3; ++j) {
      res[i][j] = 0.0f;
      for(k = 0; k < 3; ++k) {
        res[i][j] += m1[i][k] * m2[k][j];
      }
    }
  }
}

void update_camera_matrix() {
  tl[0][0] = 1.0f; tl[0][1] = 0.0f; tl[0][2] = -camPos[0];
  tl[1][0] = 0.0f; tl[1][1] = 1.0f; tl[1][2] = -camPos[1];
  tl[2][0] = 0.0f; tl[2][1] = 0.0f; tl[2][2] = 1.0f;

  rs[0][0] = 2.0f / camSize[0]; rs[0][1] =              0.0f; rs[0][2] = 0.0f;
  rs[1][0] =              0.0f; rs[1][1] = 2.0f / camSize[1]; rs[1][2] = 0.0f;
  rs[2][0] =              0.0f; rs[2][1] =              0.0f; rs[2][2] = 1.0f;

  //matmul33(fn, rs, tl);
  matmul33(fn, tl, rs);
}

struct pos matmul(mat3 mat,struct pos p) {
  struct pos res = {0};
  res.x = p.x * mat[0][0] + p.y * mat[0][1] + p.z * mat[0][2];
  res.y = p.x * mat[1][0] + p.y * mat[1][1] + p.z * mat[1][2];
  res.z = p.x * mat[2][0] + p.y * mat[2][1] + p.z * mat[2][2];
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

  struct pixel pixels[2] = {0};
  uint32_t pcount = sizeof(pixels) / sizeof(pixels[0]);

  uint32_t vao, vbo;
  { 
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pixels), pixels, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) sizeof(struct pos));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
  }

  glPointSize(20.0);
  glLineWidth(20.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //init_points(pixels, pcount);
  pixels[0].p.x = 0.0f;
  pixels[0].p.y = 0.0f;
  pixels[0].p.z = 1.0f;
  pixels[0].c.r = 1.0f;
  pixels[0].c.g = 0.0f;
  pixels[0].c.b = 0.0f;
  pixels[0].c.a = 1.0f;
  
  pixels[1].p.x = 1.0f;
  pixels[1].p.y = 1.0f;
  pixels[1].p.z = 1.0f;
  pixels[1].c.r = 0.0f;
  pixels[1].c.g = 1.0f;
  pixels[1].c.b = 0.0f;
  pixels[1].c.a = 1.0f;

  mat3 identity_matrix = {{1, 0, 0},
                          {0, 1, 0},
                          {0, 0, 1}};

  while (!glfwWindowShouldClose(window)) {
    ctime = glfwGetTime();
    deltaTime = ctime - ltime;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);

    update_camera_matrix();
    /*print_mat(tl, "translation");
    print_mat(rs, "resize");
    print_mat(fn, "final");*/

    {
      int32_t i;
      for(i = 0; i < pcount; ++i) {
#define cp pixels[i]
        fprintf(stdout, "points[%i]: %f %f %f - %f %f %f %f\n", i, cp.p.x, cp.p.y, cp.p.z, cp.c.r, cp.c.g, cp.c.b, cp.c.a);
#undef cp
      }
    }

    update_points(pixels, pcount);
    program_set_mat3(program, "fn_mat", fn);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pixels), pixels);
    glDrawArrays(GL_POINTS, 0, pcount);

    fputs("====\n", stdout);

    /*program_set_mat3(program, "fn_mat", identity_matrix);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pixels), pixels);
    glDrawArrays(GL_LINES, 0, pcount);*/
    
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

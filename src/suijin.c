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
    pixels[i].c.r = (random() % 1000) / 1000.0f;
    pixels[i].c.g = (random() % 1000) / 1000.0f;
    pixels[i].c.b = (random() % 1000) / 1000.0f;
    pixels[i].c.a = 1.0f;

    //fprintf(stdout, "Point: %fx %fy (%f, %f, %F, %f)\n", pixels[i].p.x, pixels[i].p.y, pixels[i].c.r, pixels[i].c.g, pixels[i].c.b, pixels[i].c.a);
  }
}

struct pos update_pos(struct pos p) {
  /*p.x += sin(p.x);
  p.y += cos(p.y);*/
  return p;
}

void update_points(struct pixel *__restrict pixels, uint32_t pcount) {
  int32_t i;
  for(i = 0; i < pcount; ++i) {
    pixels[i].p = update_pos(pixels[i].p);
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
  glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

  init_points(pixels, pcount);

  mat3 affine_transform = {0};
  
  while (!glfwWindowShouldClose(window)) {
    ctime = glfwGetTime();
    deltaTime = ctime - ltime;
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    update_points(pixels, pcount);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, pcount);
    
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

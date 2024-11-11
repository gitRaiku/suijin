#include <dlfcn.h>
#include <locale.h>

#include "env.h"

#define SO_PATH "./bin/suijin.so"

static struct env e;

void *plug;
#define PLUGGABLE_FUNCTIONS /* name, return type, args */ \
    PLUG(suijin_setenv, void, struct env *__restrict ee) \
    PLUG(suijin_init, uint8_t, void) \
    PLUG(suijin_unload, void, void) \
    PLUG(suijin_reload, void, void) \
    PLUG(suijin_run, uint8_t, void) \
    PLUG(suijin_end, void, void)

#define PLUG(name, ret, ...) static ret (*name)(__VA_ARGS__);
PLUGGABLE_FUNCTIONS
#undef PLUG

uint8_t reload_so(char *objpath) {
  if (plug) { dlclose(plug); }

  plug = dlopen(objpath, RTLD_NOW);
  if (plug == NULL) {
    fprintf(stderr, "Could not open %s! [%s]\n", objpath, dlerror());
    return 1;
  }
  
#define PLUG(name, ...) \
  name = dlsym(plug, #name); \
  if (!name) { \
    fprintf(stderr, "Could not load %s! [%s]\n", objpath, dlerror()); \
    return 1; \
  }
  PLUGGABLE_FUNCTIONS
#undef PLUG

  return 0;
}

void callback_error(int error, const char *desc) { fprintf(stderr, "GLFW error, no %i, desc %s\n", error, desc); }
void callback_window_should_close(GLFWwindow *window) { }
void callback_window_resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    e.initCam.ratio = e.cam.ratio = (float) width / (float) height;
    e.camUpdate = 1;
    e.w.windowW = width;
    e.w.windowH = height;
    e.w.iwinw = 1.0f / width;
    e.w.iwinh = 1.0f / height;
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

    e.w.windowW = 1920; e.w.windowH = 1080;
    e.w.iwinw = 1.0f / e.w.windowW; e.w.iwinh = 1.0f / e.w.windowH;
    window = glfwCreateWindow(e.w.windowH, e.w.windowW, "suijin", NULL, NULL);
    GL_CHECK(window, "Failed to create the window!");

    glfwMakeContextCurrent(window);
    GL_CHECK(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress), "Failed to initialize GLAD!");

    glViewport(0, 0, 1920, 1080);
    glfwSetFramebufferSizeCallback(window, callback_window_resize);
    glfwSetWindowCloseCallback(window, callback_window_should_close);
    glfwSwapInterval(0);

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

void reset_ft() {
  if (e.ftlib != NULL) {
    FT_CHECK(FT_Done_FreeType(e.ftlib), "Could not destroy the freetype lib");
  }
  FT_CHECK(FT_Init_FreeType(&e.ftlib), "Could not initialize the freetype lib");

  //FT_CHECK(FT_New_Face(e.ftlib, "Resources/Fonts/Koruri/Koruri-Regular.ttf", 0, &e.ftface), "Could not load the font ./Resources/Fonts/JetBrains/Koruri-Regular.ttf");
  FT_CHECK(FT_New_Face(e.ftlib, "Resources/Fonts/JetBrains/jetbrainsmono.ttf", 0, &e.ftface), "Could not load the font ./Resources/Fonts/JetBrains/jetbrainsmono.ttf");
  FT_CHECK(FT_Set_Char_Size(e.ftface, 0, 16 * 64, 300, 300),"Could not set the character size!");
  FT_CHECK(FT_Set_Pixel_Sizes(e.ftface, 0, (int32_t)72), "Could not set pixel sizes");
  FT_CHECK(FT_Select_Charmap(e.ftface, FT_ENCODING_UNICODE), "Could not select char map");
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

time_t sola = 0;
void check_so_update(char *__restrict path, time_t *__restrict la) {
  if (e.frame % 2000) {
    fprintf(stdout, "%u\n", e.frame % 2000);
    struct stat s;
    stat(path, &s);
    if (s.st_ctim.tv_sec != *la) {
      fprintf(stdout, "UPDATED %lu\n", *la);
      *la = s.st_ctim.tv_sec;
      suijin_unload();
      reload_so(path);
      suijin_setenv(&e);
      suijin_reload();
    }
  }
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  setbuf(stdout, NULL);

  reset_ft();
  init_random();
  GL_CHECK(glfwInit(), "Could not initialize glfw!");
  e.window = window_init();

  {
    struct stat s;
    stat(SO_PATH, &s); sola = s.st_ctim.tv_sec;
  }
  reload_so(SO_PATH);
  suijin_setenv(&e);
  suijin_init();
  while (!glfwWindowShouldClose(e.window)) { 
    suijin_run();
    check_so_update(SO_PATH, &sola);
  }
  suijin_end();
}

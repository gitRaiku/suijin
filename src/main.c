#include <dlfcn.h>

#include "env.h"

void *plug;

#define PLUGGABLE_FUNCTIONS /* name, return type, args */ \
    PLUG(plug_init, void, void) \
    PLUG(plug_update, void, void)

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

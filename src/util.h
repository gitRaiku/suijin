#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define PR_CHECK(call) if (call) { return 1; }
#define GL_CHECK(call, message) if (!(call)) { fputs(message "! Aborting...\n", stderr); glfwTerminate(); exit(1); }
#define FT_CHECK(command, errtext) { uint32_t err = command; if (err) { fprintf(stderr, "%s: %u:[%s]!\n", errtext, err, FT_Error_String(err)); exit(1); } }
#define KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

#define FPC(v) ((float *__restrict)(v))
#define V3C(v) ((v3 *__restrict)(v))
#define V4C(v) ((v4 *__restrict)(v))

#define EPS 0.0000000001

struct fbuf {
  char fb[2048];
  uint32_t fd;
  uint32_t bufp;
  uint32_t bufl;
  uint8_t flags;
};

struct v2 {
  float x;
  float y;
};
typedef struct v2 v2;
typedef struct v2 vec2;

struct v3 {
  float x;
  float y;
  float z;
};
typedef struct v3 v3;
typedef struct v3 vec3;

struct v4 {
  float x;
  float y;
  float z;
  float w;
};
typedef struct v4 v4;
typedef struct v4 vec4;
typedef struct v4 quat;
typedef struct v4 pos;

struct uv4 {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t w;
};
typedef struct uv4 uv4;

struct uv3 {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};
typedef struct uv3 uv3;

typedef float mat3[3][3];
typedef float mat4[4][4];

struct keypress { /// TODO: Organise
  uint8_t iskb;
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

#define QV(v) (*((v3 *__restrict)(&v)))

void read_uint32_t(FILE *__restrict stream, uint32_t *__restrict nr);

float __attribute((pure)) min(float o1, float o2); // INLINE

float __attribute((pure)) max(float o1, float o2); // INLINE
                                                   
void swap(uint8_t *__restrict o1, uint8_t *__restrict o2); // INLINE
void pswap(void **o1, void **o2); // INLINE

uint32_t __attribute((pure)) umax(uint32_t o1, uint32_t o2); // INLINE

void print_quat(quat q, const char *str);

void print_vec3(vec3 v, const char *str);

void print_mat3(mat3 m, const char *name);

void print_mat(mat4 m, const char *name);

void ckpush(struct ckq *__restrict cq, struct keypress *__restrict kp);

struct keypress cktop(struct ckq *__restrict cq);

void init_random();

float __attribute((pure)) toRadians(float o); // INLINE

uint8_t __attribute((pure)) epsilon_equals(float o1, float o2); // INLINE

float rfloat(float lb, float ub);

float __attribute((pure)) lerp(float bottomLim, float topLim, float progress); // INLINE

float __attribute((pure)) clamp(float val, float lb, float ub);

pos matmul(mat3 mat, pos p);

vec3 __attribute((pure)) *__restrict qv(quat *__restrict q); // INLINE

void print_keypress(char *__restrict ch, struct keypress kp);

#endif

#include "util.h"

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

float __attribute((pure)) min(float o1, float o2) { // INLINE
  return o1 < o2 ? o1 : o2;
}

float __attribute((pure)) max(float o1, float o2) { // INLINE
  return o1 > o2 ? o1 : o2;
}

void swap(uint8_t *__restrict o1, uint8_t *__restrict o2) { // INLINE
  uint8_t t = *o1;
  *o1 = *o2;
  *o2 = t;
}

void uswap(uint32_t *__restrict o1, uint32_t *__restrict o2) { // INLINE
  uint32_t t = *o1;
  *o1 = *o2;
  *o2 = t;
}


void pswap(void **o1, void **o2) { // INLINE
  void *t = *o1;
  *o1 = *o2;
  *o2 = t;
}

uint32_t __attribute((pure)) umax(uint32_t o1, uint32_t o2) { // INLINE
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

float __attribute((pure)) toRadians(float o) { // INLINE
  return o * M_PI / 180.0f;
}

uint8_t __attribute((pure)) epsilon_equals(float o1, float o2) { // INLINE
  return fabsf(o1 - o2) < 0.000000001;
}

float rfloat(float lb, float ub) {
  return ((float)rand() / (float)RAND_MAX) * (ub - lb) + lb;
}

float __attribute((pure)) lerp(float bottomLim, float topLim, float progress) { // INLINE
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

pos matmul(mat3 mat, pos p) {
  pos res = {0};
  res.x = p.x * mat[0][0] + p.y * mat[0][1] + mat[0][2];
  res.y = p.x * mat[1][0] + p.y * mat[1][1] + mat[1][2];
  return res;
}

vec3 __attribute((pure)) *__restrict qv(quat *__restrict q) { // INLINE
  return (v3 *__restrict) q;
}

/*
vec3 __attribute((pure)) qdir(quat *__restrict q) { // INLINE
  return (v3) { q.y, y.z, q.w };
}*/

void print_keypress(char *__restrict ch, struct keypress kp) {
  fprintf(stdout, "%s%u %u %u %u\n", ch, kp.key, kp.scancode, kp.action, kp.mod);
}


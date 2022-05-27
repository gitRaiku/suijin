#ifndef IZANAMI_H
#define IZANAMI_H

#include "defs.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define VECTOR_PUSH(name, btype, stype)               \
void name ##vp(btype v, stype val) {                  \
  if (v->l == v->s) {                                 \
    v->s *= 2;                                        \
    v->v = realloc(v->v, sizeof(stype) * v->s);       \
  }                                                   \
  v->v[v->l] = val;                                   \
  ++v->l;                                             \
}                                                     

#define VECTOR_INIT(name, btype, stype)       \
void name ##vi(btype v) {                     \
  v->l = 0;                                   \
  v->s = 4;                                   \
  v->v = malloc(sizeof(stype) * v->s);        \
}                                             

#define VECTOR_TRIM(name, btype, stype)          \
void name ##vt(btype v) {                        \
  if (v->s != v->l) {                            \
    v->s = v->l;                                 \
    v->v = realloc(v->v, sizeof(stype) * v->s);  \
  }                                              \
}

#define VECTOR_SUITE(name, btype, stype) \
VECTOR_PUSH(name, btype, stype) \
VECTOR_INIT(name, btype, stype) \
VECTOR_TRIM(name, btype, stype)

struct material {
  char name[64];
  v3 ambient;
  v3 diffuse;
  v3 spec;
  v3 emmisive;
  float spece;
  float transp;
  float optd;
  uint32_t illum;
};

struct matv {
  struct material *__restrict v;
  uint32_t l;
  uint32_t s;
};

#define LINEND_MASK   0b010 /// Read a line end
#define UNREADABLE_MASK   0b100 /// There are still bytes left in the file

char _getc(struct fbuf *__restrict cb);

float read_float(struct fbuf *__restrict cb);

void next_token(char *__restrict tok, struct fbuf *__restrict cb);

void parse_material(struct matv *__restrict mats, char *__restrict fname);

#endif

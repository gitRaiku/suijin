#ifndef IZANAGI_H
#define IZANAGI_H

#include <glad/glad.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <png.h>
#include <dirent.h>

#include "util.h"
#include "linalg.h"

#define VECSTRUCT(name, type) \
struct name ##v {       \
  type *__restrict v;   \
  uint32_t l;           \
  uint32_t s;           \
}

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

#define DEF_VECTOR_PUSH(name, btype, stype) \
void name ##vp(btype v, stype val);

#define DEF_VECTOR_INIT(name, btype, stype) \
void name ##vi(btype v);

#define DEF_VECTOR_TRIM(name, btype, stype) \
void name ##vt(btype v);

#define DEF_VECTOR_SUITE(name, btype, stype) \
DEF_VECTOR_PUSH(name, btype, stype) \
DEF_VECTOR_INIT(name, btype, stype) \
DEF_VECTOR_TRIM(name, btype, stype)

struct texture {
  // char *__restrict v; // Data
  uint32_t w; // Width
  uint32_t h; // Height
  uint32_t c; // Encoding
  uint32_t i; // OpenGL Id
  /// uint8_t flags;
};

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

  /// Textures;
  struct texture tamb;   // Ambient
  struct texture tdiff;  // Diffuse
  struct texture tspec;  // Specular
  struct texture tbump;  // Bump
  struct texture tdisp;  // Displacement
  //struct texture thigh;  // Highlight
  //struct texture talpha; // Alpha
  //struct texture tdec;   // Decal
  //struct texture trefl;  // Reflection
};

VECSTRUCT(mat, struct material);

#define LINEND_MASK   0b010 /// Read a line end
#define UNREADABLE_MASK   0b100 /// There are still bytes left in the file
VECSTRUCT(v4, v4);
VECSTRUCT(v3, v3);
VECSTRUCT(v2, v2);

struct faceev {
  uv3 v;
  uv3 t;
  uv3 n;
};
VECSTRUCT(face, struct faceev);
VECSTRUCT(float, float);

struct mate {
  uint32_t m; // Material
  uint32_t i; // Start Index
};

VECSTRUCT(mati, struct mate);

struct object {
  struct floatv v;
  struct mativ m;

  uint32_t vao;
  uint32_t vbo;

  float scale;
  v3 pos;

  mat4 aff;

  char name[64];
};

VECSTRUCT(obj, struct object);

DEF_VECTOR_SUITE(mat, struct matv *__restrict, struct material)
DEF_VECTOR_SUITE(obj, struct objv *__restrict, struct object)

uint8_t *__restrict read_png(char *__restrict fname, char *__restrict dname, uint32_t *__restrict width, uint32_t *__restrict height);

void parse_folder(struct objv *__restrict objs, struct matv *__restrict mats, char *__restrict dname);

void destroy_object(struct object *__restrict obj);

void update_affine_matrix(struct object *__restrict obj);

#endif

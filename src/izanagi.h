#ifndef IZANAGI_H
#define IZANAGI_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "defs.h"
#include "linalg.h"

struct face_elem {
  uint32_t v; // Vertex Index
  uint32_t t; // Texture Index
  uint32_t n; // Normal Index
};

struct material {
  uint32_t idek; // TODO
};

struct v4v {
  v4 *__restrict v;
  uint32_t l;
  uint32_t s;
};

struct v3v {
  v3 *__restrict v;
  uint32_t l;
  uint32_t s;
};

struct v2v {
  v2 *__restrict v;
  uint32_t l;
  uint32_t s;
};

typedef struct face_elem faceev[4];
struct facev {
  faceev *v;
  uint32_t l;
  uint32_t s;
};

struct matv {
  struct material *__restrict v;
  uint32_t l;
  uint32_t s;
};

struct object {
  struct   v4v v; // Vertices
  struct   v2v t; // Texture coords
  struct   v3v n; // Normals
  struct facev f; // Faces
  struct  matv m; // Materials
  uint16_t smooth_shading;          
  char name[64];
};

void parse_object_file(char *__restrict fname, struct object *__restrict objs, uint32_t *__restrict objl);

#endif

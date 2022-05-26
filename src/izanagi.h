#ifndef IZANAGI_H
#define IZANAGI_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "defs.h"
#include "linalg.h"

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

struct faceev {
  uv4 v;
  uv4 t;
  uv4 n;
};

struct facev {
  struct faceev *v;
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

struct object *__restrict parse_object_file(char *__restrict fname, uint32_t *__restrict objl);

#endif

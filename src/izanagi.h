#ifndef IZANAGI_H
#define IZANAGI_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "defs.h"
#include "linalg.h"
#include "izanami.h"

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
  uv3 v;
  uv3 t;
  uv3 n;
};

struct facev {
  struct faceev *v;
  uint32_t l;
  uint32_t s;
};

struct floatv {
  float *v;
  uint32_t l;
  uint32_t s;
};

struct mate {
  uint32_t m; // Material
  uint32_t i; // Start Index
};

struct mativ {
  struct mate *__restrict v;
  uint32_t l;
  uint32_t s;
};

struct object {
  struct floatv v;
  struct mativ m;
  char name[64];
};

struct object *__restrict parse_object_file(char *__restrict fname, uint32_t *__restrict objl, struct matv *__restrict materials);

void destroy_object(struct object *__restrict obj);

#endif

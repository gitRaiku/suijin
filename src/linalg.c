#include "linalg.h"

vec3 norm(vec3 v) {
  float dist = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  vec3 res;
  res.x = v.x / dist;
  res.y = v.y / dist;
  res.z = v.z / dist;
  return res;
}

vec3 cross(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.y * v2.z - v1.z * v2.y;
  res.y = v1.z * v2.x - v1.x * v2.z;
  res.z = v1.x * v2.y - v1.y * v2.x;
  return res;
}

float dot(vec3 v1, vec3 v2) {
  return v1.x * v2.x + 
         v1.y * v2.y + 
         v1.z * v2.z;
}

vec3 v3m(float coef, vec3 v) {
  vec3 res;
  res.x = v.x * coef;
  res.y = v.y * coef;
  res.z = v.z * coef;
  return res;
}

vec3 v3s(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.x - v2.x;
  res.y = v1.y - v2.y;
  res.z = v1.z - v2.z;
  return res;
}

vec3 v3a(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.x + v2.x;
  res.y = v1.y + v2.y;
  res.z = v1.z + v2.z;
  return res;
}

vec3 v3n(vec3 v) {
  vec3 res;
  res.x = -v.x;
  res.y = -v.y;
  res.z = -v.z;
  return res;
}

void matmul44(mat4 res, mat4 m1, mat4 m2) {
  int32_t i, j, k;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      res[i][j] = 0.0f;
      for(k = 0; k < 4; ++k) {
        res[i][j] += m1[i][k] * m2[k][j];
      }
    }
  }
}


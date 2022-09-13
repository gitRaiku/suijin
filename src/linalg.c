#include "linalg.h"

quat __attribute((pure)) gen_quat(vec3 axis, float rot) {
  float rs = sinf(rot / 2.0f);
  float rc = cosf(rot / 2.0f);
  
  quat res;
  res.w =          rc;
  res.x = axis.x * rs;
  res.y = axis.y * rs;
  res.z = axis.z * rs;
  return res;
}

quat __attribute((pure)) qconj(quat q) {
  return (quat) { -q.x, -q.y, -q.z, q.w };
}

quat __attribute((pure)) qmul(quat q1, quat q2) {
  quat res;
  res.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
  res.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
  res.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
  res.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;

  return res;
}

void quat_to_mat(mat3 ret, quat q) {
  memset(ret, 0, sizeof(mat3));
  ret[0][0] = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  ret[0][1] =        2.0f * (q.x * q.y - q.w * q.z);
  ret[0][2] =        2.0f * (q.x * q.z + q.w * q.y);

  ret[1][0] =        2.0f * (q.x * q.y + q.w * q.z);
  ret[1][1] = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
  ret[1][2] =        2.0f * (q.y * q.z + q.w * q.x);

  ret[2][0] =        2.0f * (q.x * q.z + q.w * q.y);
  ret[2][1] =        2.0f * (q.y * q.z + q.w * q.x);
  ret[2][2] = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  
  //ret[3][3] = 1.0f;
}

vec3 __attribute((pure)) norm(vec3 v) {
  float dist = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  vec3 res;
  res.x = v.x / dist;
  res.y = v.y / dist;
  res.z = v.z / dist;
  return res;
}

quat __attribute((pure)) qnorm(quat q) {
  float dist = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  quat res;
  res.x = q.x / dist;
  res.y = q.y / dist;
  res.z = q.z / dist;
  res.w = q.w / dist;
  return res;
}

vec3 __attribute((pure)) cross(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.y * v2.z - v1.z * v2.y;
  res.y = v1.z * v2.x - v1.x * v2.z;
  res.z = v1.x * v2.y - v1.y * v2.x;
  return res;
}

float __attribute((pure)) dot(vec3 v1, vec3 v2) {
  return v1.x * v2.x + 
         v1.y * v2.y + 
         v1.z * v2.z;
}

vec3 __attribute((pure)) v3m(float coef, vec3 v) {
  vec3 res;
  res.x = v.x * coef;
  res.y = v.y * coef;
  res.z = v.z * coef;
  return res;
}

vec3 __attribute((pure)) v3s(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.x - v2.x;
  res.y = v1.y - v2.y;
  res.z = v1.z - v2.z;
  return res;
}

vec3 __attribute((pure)) v3a(vec3 v1, vec3 v2) {
  vec3 res;
  res.x = v1.x + v2.x;
  res.y = v1.y + v2.y;
  res.z = v1.z + v2.z;
  return res;
}

vec3 __attribute((pure)) v3n(vec3 v) {
  vec3 res;
  res.x = -v.x;
  res.y = -v.y;
  res.z = -v.z;
  return res;
}

vec3 __attribute((pure)) v3i(vec3 v) {
  vec3 res;
  res.x = 1 / v.x;
  res.y = 1 / v.y;
  res.z = 1 / v.z;
  return res;
}

void crotm(mat4 res, v3 r) {
  res[0][0] = cos(r.x) * cos(r.y); res[0][1] = sin(r.x) * sin(r.y) * cos(r.z) - cos(r.x) * cos(r.z); res[0][2] = cos(r.x) * sin(r.y) * cos(r.z) + sin(r.x) * sin(r.z);
  res[0][0] = cos(r.x) * sin(r.y); res[0][1] = sin(r.x) * sin(r.y) * cos(r.z) - cos(r.x) * cos(r.z); res[0][2] = cos(r.x) * sin(r.y) * cos(r.z) + sin(r.x) * sin(r.z);
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

void matmul43(mat4 res, mat4 m1, mat3 m2) {
  int32_t i, j, k;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      res[i][j] = 0.0f;
      for(k = 0; k < 4; ++k) {
        if (j == 3 || k == 3) {
          if (j == k) {
            res[i][j] += m1[i][k];
          }
          continue;
        }
        res[i][j] += m1[i][k] * m2[k][j];
      }
    }
  }
}

vec3 __attribute((pure)) vmm3(mat3 m, vec3 v) { /* Vec mat mul 3 */
  vec3 res;

  res.x = v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2]; /* v.w * m[0][3]; */
  res.y = v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2]; /* v.w * m[1][3]; */
  res.z = v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2]; /* v.w * m[2][3]; */
  //res.w = v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + v.w * m[3][3];

  return res;
}

vec3 __attribute((pure)) v3m4(mat4 m, vec3 v) {
  vec3 res;

  res.x = v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + m[0][3];
  res.y = v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + m[1][3];
  res.z = v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + m[2][3];

  return res;
}



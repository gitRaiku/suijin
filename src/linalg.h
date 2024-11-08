#ifndef LINALG_H
#define LINALG_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#include "util.h"

quat __attribute((pure)) gen_quat(vec3 axis, float rot);

quat __attribute((pure)) qmul(quat q1, quat q2);

quat __attribute((pure)) qconj(quat q);

void quat_to_mat(mat3 ret, quat q);

vec3 __attribute((pure)) norm(vec3 v);

quat __attribute((pure)) qnorm(quat q);

vec3 __attribute((pure)) cross(vec3 v1, vec3 v2);

float __attribute((pure)) dot(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3m(float coef, vec3 v);

vec3 __attribute((pure)) v3s(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3a(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3n(vec3 v);

vec3 __attribute((pure)) v3i(vec3 v);

void matmul44(mat4 res, mat4 m1, mat4 m2);

void matmul43(mat4 res, mat4 m1, mat3 m2);

void matmul33(mat3 res, mat3 m1, mat3 m2);

vec3 __attribute((pure)) vmm3(mat3 m, vec3 v); /* Vec mat mul 3 */

vec3 __attribute((pure)) v3m4(mat4 m, vec3 v);

#endif

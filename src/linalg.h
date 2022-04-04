#include <stdint.h>
#include <math.h>
#include <string.h>

struct vec3 {
  float x;
  float y;
  float z;
};
typedef struct vec3 vec3;

struct vec4 {
  float x;
  float y;
  float z;
  float w;
};
typedef struct vec4 vec4;
typedef struct vec4 quat;

typedef float mat3[3][3];
typedef float mat4[4][4];

quat __attribute((pure)) gen_quat(vec3 axis, float rot);

quat __attribute((pure)) qmul(quat q1, quat q2);

quat __attribute((pure)) qconj(quat q);

void quat_to_mat(mat3 ret, quat q);

vec3 __attribute((pure)) norm(vec3 v);

vec3 __attribute((pure)) cross(vec3 v1, vec3 v2);

float __attribute((pure)) dot(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3m(float coef, vec3 v);

vec3 __attribute((pure)) v3s(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3a(vec3 v1, vec3 v2);

vec3 __attribute((pure)) v3n(vec3 v);

void matmul44(mat4 res, mat4 m1, mat4 m2);

vec3 __attribute((pure)) vmm3(mat3 m, vec3 v); /* Vec mat mul 3 */

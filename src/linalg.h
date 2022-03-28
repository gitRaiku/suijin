#include <stdint.h>
#include <math.h>

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

typedef float mat3[3][3];
typedef float mat4[4][4];

vec3 norm(vec3 v);

vec3 cross(vec3 v1, vec3 v2);

float dot(vec3 v1, vec3 v2);

vec3 v3m(float coef, vec3 v);

vec3 v3s(vec3 v1, vec3 v2);

vec3 v3a(vec3 v1, vec3 v2);

vec3 v3n(vec3 v);

void matmul44(mat4 res, mat4 m1, mat4 m2);


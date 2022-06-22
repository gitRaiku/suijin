#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "util.h"

uint8_t shader_get(const char *__restrict const path, uint32_t shaderType, uint32_t *__restrict shader);

uint8_t program_get(uint32_t shaderc, uint32_t *__restrict shaders, uint32_t *__restrict program);

void program_set_float1(uint32_t program, const char *uniform_name, float val1);

void program_set_float2(uint32_t program, const char *uniform_name, float val1, float val2);

void program_set_float3(uint32_t program, const char *uniform_name, float val1, float val2, float val3);

void program_set_float3v(uint32_t program, const char *uniform_name, vec3 v);

void program_set_float4(uint32_t program, const char *uniform_name, float val1, float val2, float val3, float val4);

void program_set_int1(uint32_t program, const char *uniform_name, int32_t val1);

typedef float mat3[3][3];
typedef float mat4[4][4];

void program_set_mat3(uint32_t program, const char *uniform_name, mat3 val1);

void program_set_mat4(uint32_t program, const char *uniform_name, mat4 val1);

#endif

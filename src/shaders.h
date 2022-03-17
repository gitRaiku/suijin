#include <glad/glad.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

uint8_t shader_get(const char *__restrict const path, uint32_t shaderType, uint32_t *__restrict shader);

uint8_t program_get(uint32_t shaderc, uint32_t *__restrict shaders, uint32_t *__restrict program);

void program_set_float1(uint32_t program, const char *uniform_name, float val1);

void program_set_float2(uint32_t program, const char *uniform_name, float val1, float val2);

void program_set_float3(uint32_t program, const char *uniform_name, float val1, float val2, float val3);

void program_set_float4(uint32_t program, const char *uniform_name, float val1, float val2, float val3, float val4);

void program_set_int1(uint32_t program, const char *uniform_name, int32_t val1);

#include "shaders.h"

#ifdef SUIJIN_DEBUG
void shader_error(uint32_t shader, const char *__restrict path) {
  char output[512];
  glGetShaderInfoLog(shader, sizeof(output), NULL, output);
  fprintf(stderr, "Could not compile the shader at %s! [%s]\n", path, output);
}

uint8_t program_error(uint32_t program) {
  int32_t success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char output[512];
    glGetProgramInfoLog(program, sizeof(output), NULL, output);
    fprintf(stderr, "Could not link the program! [%s] \n", output);
    return 1;
  } else {
    fputs("Successfully linked the program\n", stdout);
  }
  return 0;
}
#endif

uint8_t shader_get(const char *__restrict path, uint32_t shaderType, uint32_t *__restrict shader) {
  uint32_t fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s, %m!\n", path);
    return 1;
  }

  struct stat s;
  if (stat(path, &s) < 0) {
    fprintf(stderr, "Could not stat %s, %m!\n", path);
    return 1;
  }

  char *rbuf = malloc(s.st_size + 1);
  if (read(fd, rbuf, s.st_size) != s.st_size) {
    fprintf(stderr, "Could not read %s, %m!\n", path);
    free(rbuf);
    return 1;
  }
  rbuf[s.st_size] = '\0';

  //fprintf(stdout, "SHADER: %s\n\"\"\"\n%s\n\"\"\"\n", path, rbuf);

  *shader = glCreateShader(shaderType);
  glShaderSource(*shader, 1, (const char * const*)&rbuf, NULL);
  glCompileShader(*shader);
  free(rbuf);
  close(fd);

  int32_t status;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
#ifdef SUIJIN_DEBUG
  if (status != GL_TRUE) {
    shader_error(*shader, path);
  }
#endif

  return status != GL_TRUE; // 0 for good compilation, 1 for error
}

uint8_t program_get(uint32_t shaderc, uint32_t *__restrict shaders, uint32_t *__restrict program) {
  *program = glCreateProgram();
  int32_t i;
  for(i = 0; i < shaderc; ++i) {
    glAttachShader(*program, shaders[i]);
  }
  glLinkProgram(*program);

  int32_t status;
  glGetProgramiv(*program, GL_LINK_STATUS, &status);
#ifdef SUIJIN_DEBUG
  if (status != GL_TRUE) {
    program_error(*program);
  }
#endif

  return status != GL_TRUE; // 0 for good link, 1 for error
}

void program_set_float1(uint32_t program, const char *uniform_name, float val1) {
    uint32_t uniform_loc = glGetUniformLocation(program, uniform_name);
    glUniform1f(uniform_loc, val1);
}

void program_set_float2(uint32_t program, const char *uniform_name, float val1, float val2) {
    uint32_t uniform_loc = glGetUniformLocation(program, uniform_name);
    glUniform2f(uniform_loc, val1, val2);
}

void program_set_float3(uint32_t program, const char *uniform_name, float val1, float val2, float val3) {
    uint32_t uniform_loc = glGetUniformLocation(program, uniform_name);
    glUniform3f(uniform_loc, val1, val2, val3);
}

void program_set_float4(uint32_t program, const char *uniform_name, float val1, float val2, float val3, float val4) {
    uint32_t uniform_loc = glGetUniformLocation(program, uniform_name);
    glUniform4f(uniform_loc, val1, val2, val3, val4);
}

void program_set_int1(uint32_t program, const char *uniform_name, int32_t val1) {
    uint32_t uniform_loc = glGetUniformLocation(program, uniform_name);
    glUniform1i(uniform_loc, val1);

}

#ifndef SUIJIN_FUJIN_H
#define SUIJIN_FUJIN_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <GLFW/glfw3.h>

// #include "gstate.h"

typedef int32_t axcd;

void log_message(uint32_t severity, const char *message);

void log_format(uint32_t severity, char *format, ...);

#endif //SUIJIN_FUJIN_H

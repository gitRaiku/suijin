#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include "util.h"
#include "shaders.h"
#include "linalg.h"
#include "izanagi.h"
#include "nesaku.h"

/// Camera
#define ZOOM_SPEED 0.01f
#define PAN_SPEED 13.0f
#define SENS 0.001f

/// Ui
#define UI_COL_BG1 0x181818FF
#define UI_COL_BG2 0x484848A0
#define UI_COL_FG1 0xBA2636A0
#define UI_COL_FG2 0xA232B4FF
#define UI_COL_FG3 0xFFFFFFFF

/// Framerate update rate
#define FRAME_UPDATE 10

/// Thermal Subsystem
#define OUTSIDE_TEMP 0.1f
#define DIFF_COEF 0.05f

#endif

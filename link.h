#ifndef LINK_H__
#define LINK_H__

#include "files.h"

typedef enum {
  LINK_FLAG_BIN = 1 << 0,
  LINK_FLAG_SO = 1 << 1,
  LINK_FLAG_EXE_STATE = 1 << 2,
} link_debug_flag_t;

void exe_link_obj(exe_state_t *exes, obj_t *obj);
void exe_link_boilerplate(exe_state_t *exes);
void exe_state_dump(exe_state_t *exes);

#endif  // LINK_H__

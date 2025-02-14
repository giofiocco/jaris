#ifndef LINK_H__
#define LINK_H__

#include "files.h"

typedef enum {
  LINK_FLAG_BIN = 1 << 0,
  LINK_FLAG_SO = 1 << 1,
  LINK_FLAG_EXE_STATE = 1 << 2,
} link_debug_flag_t;

void exe_link_obj(exe_state_t *state, obj_t *obj, int debug_info);
void exe_link_so(exe_state_t *state, so_t *so, char *name);
void exe_link_boilerplate(exe_state_t *state, int debug_info);
void exe_state_dump(exe_state_t *state);

#endif // LINK_H__

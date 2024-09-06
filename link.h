#ifndef LINK_H__
#define LINK_H__

#include "files.h"

typedef enum {
  LINK_FLAG_BIN = 1 << 0,
  LINK_FLAG_SO = 1 << 1,
  LINK_FLAG_EXE_STATE = 1 << 2,
} link_debug_flag_t;

exe_t link(obj_t *objs_list, int objs_count, link_debug_flag_t flag);

#endif  // LINK_H__

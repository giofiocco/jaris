#ifndef LINK_H__
#define LINK_H__

#include "files.h"

typedef enum {
  LINK_FLAG_BIN       = 0b0001,
  LINK_FLAG_EXE_STATE = 0b0010,
} link_debug_flag_t;

exe_t link(obj_t *objs_list, int objs_count, link_debug_flag_t flag);

#endif // LINK_H__

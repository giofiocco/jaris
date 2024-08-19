#include <assert.h>

#include "files.h"

#define TODO assert(0 && "TO IMPLEMENT")

void exe_link_obj(exe_t *exe, obj_t *obj) {
  (void)exe;
  (void)obj;
  TODO;
}

exe_t link(obj_t *objs_list, int objs_count) {
  assert(objs_list);

  exe_t exe = {0};

  for (int i = 0; i < objs_count; ++i) {
    exe_link_obj(&exe, &objs_list[i]);
  }

  return exe;
}

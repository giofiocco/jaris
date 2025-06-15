#ifndef LINK_H__
#define LINK_H__

#include "files.h"

typedef struct {
  exe_t exe;
  uint8_t global_count;
  uint16_t globals[GLOBAL_MAX_COUNT];
  uint8_t extern_count;
  uint16_t externs[EXTERN_MAX_COUNT];
  int so_count;
  char so_names[DYNAMIC_MAX_COUNT][FILE_NAME_MAX_LEN];
  int so_global_counts[DYNAMIC_MAX_COUNT];
  char so_gloabals_images[DYNAMIC_MAX_COUNT][GLOBAL_MAX_COUNT][LABEL_MAX_LEN];
  uint16_t so_globals_pos[DYNAMIC_MAX_COUNT][GLOBAL_MAX_COUNT];
} exe_state_t;

void exe_link_obj(exe_state_t *state, obj_t *obj, int debug_info);
void exe_link_so(exe_state_t *state, so_t *so, char *name);
void exe_link_boilerplate(exe_state_t *state, int debug_info);
void exe_state_dump(exe_state_t *state);
so_t so_from_exe_state(exe_state_t *state);

#endif // LINK_H__

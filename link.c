#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "instructions.h"
#include "link.h"

#define TODO assert(0 && "TO IMPLEMENT")

void exe_link_obj(exe_state_t *state, obj_t *obj, int debug_info) {
  assert(state);
  assert(obj);

  exe_t *exe = &state->exe;

  uint16_t offset = state->exe.code_size;
  int symbols_offset = exe->symbol_count;

  memcpy(exe->code + offset, obj->code, obj->code_size);
  exe->code_size += obj->code_size + obj->code_size % 2;

  for (int i = 0; i < obj->reloc_count; ++i) {
    assert(exe->reloc_count + 1 < RELOC_MAX_COUNT);
    exe->relocs[exe->reloc_count] = obj->relocs[i];
    exe->relocs[exe->reloc_count].where += offset;
    exe->relocs[exe->reloc_count].what += offset;
    ++exe->reloc_count;
  }

  if (debug_info) {
    for (int i = 0; i < obj->symbol_count; ++i) {
      exe_add_symbol_offset(exe, &obj->symbols[i], offset);
    }
  }

  for (int i = 0; i < obj->global_count; ++i) {
    assert(state->global_count + 1 < GLOBAL_MAX_COUNT);
    if (!debug_info) {
      exe_add_symbol_offset(exe, &obj->symbols[obj->globals[i]], offset);
    }
    state->globals[state->global_count++] = obj->globals[i] + symbols_offset;
  }

  for (int i = 0; i < obj->extern_count; ++i) {
    assert(state->extern_count + 1 < EXTERN_MAX_COUNT);
    if (!debug_info) {
      exe_add_symbol_offset(exe, &obj->symbols[obj->globals[i]], offset);
    }
    state->externs[state->extern_count++] = obj->externs[i] + symbols_offset;
  }
}

void exe_link_so(exe_state_t *state, so_t *so, char *name) {
  assert(state);
  assert(so);
  assert(name);

  assert(state->so_count + 1 < DYNAMIC_MAX_COUNT);
  int index = state->so_count;
  strncpy(state->so_names[index], name, FILE_NAME_MAX_LEN);
  state->so_global_counts[index] = so->global_count;
  for (int i = 0; i < so->global_count; ++i) {
    strncpy(state->so_gloabals_images[index][i], so->symbols[so->globals[i]].image, LABEL_MAX_LEN);
    state->so_globals_pos[index][i] = so->symbols[so->globals[i]].pos;
  }

  ++state->so_count;
}

void exe_link_boilerplate(exe_state_t *state, int debug_info) {
  assert(state);

  assert(state->exe.code_size == 0);
  assert(state->exe.symbol_count == 0);
  obj_t boilerplate = {
      .code_size = 4,
      .code = {NOP, JMP},
      .reloc_count = 0,
      .relocs = {},
      .global_count = 0,
      .globals = {},
      .extern_count = 1,
      .externs = {0},
      .symbol_count = 1,
      .symbols =
          {
              {.image = "_start",
               .pos = 0xFFFF,
               .reloc_count = 1,
               .relocs = {1},
               .relreloc_count = 0,
               .relrelocs = {}},
          },
  };

  exe_link_obj(state, &boilerplate, debug_info);
}

void exe_state_dump(exe_state_t *state) {
  assert(state);

  printf("EXE:\n");
  exe_dump(&state->exe);
  printf("GLOBALS:");
  for (int i = 0; i < state->global_count; ++i) {
    printf(" %d", state->globals[i]);
  }
  printf("\n");
  printf("EXTERNS:");
  for (int i = 0; i < state->extern_count; ++i) {
    printf(" %d", state->externs[i]);
  }
  printf("\n");
  printf("SOS: %d\n", state->so_count);
  for (int i = 0; i < state->so_count; ++i) {
    printf("\t%s %s:\n",
           state->so_names[i],
           strcmp(state->so_names[i], "\x01") == 0 ? "(STDLIB)" : "");
    for (int j = 0; j < state->so_global_counts[i]; ++j) {
      printf("\t\t%s %04X\n", state->so_gloabals_images[i][j], state->so_globals_pos[i][j]);
    }
  }
}

#include <assert.h>
#include <string.h>

#include "../../mystb/errors.h"
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

int exe_find_symbol(exe_t *exe, char *name) {
  assert(exe);
  assert(name);

  for (int i = 0; i < exe->symbol_count; ++i) {
    if (strcmp(exe->symbols[i].image, name) == 0) {
      return i;
    }
  }

  return -1;
}

void exe_link_obj(exe_state_t *state, obj_t *obj, int debug_info) {
  assert(state);
  assert(obj);

  exe_t *exe = &state->exe;

  uint16_t offset = state->exe.code_size;

  memcpy(exe->code + offset, obj->code, obj->code_size);
  exe->code_size += obj->code_size + obj->code_size % 2;

  for (int i = 0; i < obj->reloc_count; ++i) {
    assert(exe->reloc_count + 1 < RELOC_MAX_COUNT);
    exe->relocs[exe->reloc_count++] =
        (reloc_entry_t){obj->relocs[i].where + offset, obj->relocs[i].what + offset};
  }

  uint8_t copied[obj->symbol_count];
  memset(copied, 0, obj->symbol_count * sizeof(copied[0]));

  for (int i = 0; i < obj->extern_count; ++i) {
    copied[obj->externs[i]] = 1;
    symbol_t *from = &obj->symbols[obj->externs[i]];
    int index = exe_find_symbol(exe, from->image); // TODO: find global
    if (index == -1) {
      assert(state->extern_count + 1 < EXTERN_MAX_COUNT);
      state->externs[state->extern_count++] = exe->symbol_count;
      exe_add_symbol_offset(exe, from, offset);
    } else {
      assert(state->extern_count + 1 < EXTERN_MAX_COUNT);
      state->externs[state->extern_count++] = index;

      symbol_t *s = &exe->symbols[index];
      assert(s->reloc_count + from->reloc_count < INTERN_RELOC_MAX_COUNT);
      for (int j = 0; j < from->reloc_count; ++j) {
        s->relocs[s->reloc_count++] = from->relocs[j] + offset;
      }
      if (debug_info) {
        assert(s->relreloc_count + from->relreloc_count < INTERN_RELOC_MAX_COUNT);
        for (int j = 0; j < from->relreloc_count; ++j) {
          s->relrelocs[s->relreloc_count++] = from->relrelocs[j] + offset;
        }
      }
    }
  }

  for (int i = 0; i < obj->global_count; ++i) {
    copied[obj->globals[i]] = 1;
    symbol_t *from = &obj->symbols[obj->globals[i]];
    int index = exe_find_symbol(exe, from->image);
    if (index == -1) {
      assert(state->global_count + 1 < GLOBAL_MAX_COUNT);
      state->globals[state->global_count++] = exe->symbol_count;
      exe_add_symbol_offset(exe, from, offset);
    } else {
      symbol_t *s = &exe->symbols[index];
      if (s->pos != 0xFFFF) {
        eprintf("label redefinition: '%s'", s->image);
      }

      assert(state->global_count + 1 < GLOBAL_MAX_COUNT);
      state->globals[state->global_count++] = index;

      s->pos = from->pos + offset;

      assert(s->reloc_count + from->reloc_count < INTERN_RELOC_MAX_COUNT);
      for (int j = 0; j < from->reloc_count; ++j) {
        s->relocs[s->reloc_count++] = from->relocs[j] + offset;
      }
      if (debug_info) {
        assert(s->relreloc_count + from->relreloc_count < INTERN_RELOC_MAX_COUNT);
        for (int j = 0; j < from->relreloc_count; ++j) {
          s->relrelocs[s->relreloc_count++] = from->relrelocs[j] + offset;
        }
      }
    }
  }

  if (debug_info) {
    for (int i = 0; i < obj->symbol_count; ++i) {
      if (copied[i]) {
        continue;
      }
      exe_add_symbol_offset(exe, &obj->symbols[i], offset);
    }
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
      .code = {NOP, JMP, 0, 0},
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
               .relocs = {2},
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
    if (state->so_names[i][0] == 1) {
      printf("(STDLIB):\n");
    } else {
      printf("%s:\n", state->so_names[i]);
    }
    for (int j = 0; j < state->so_global_counts[i]; ++j) {
      printf("\t\t%s %04X\n", state->so_gloabals_images[i][j], state->so_globals_pos[i][j]);
    }
  }
}

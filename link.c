#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "instructions.h"
#include "link.h"

#define TODO assert(0 && "TO IMPLEMENT")

void exe_link_obj(exe_state_t *exes, obj_t *obj) {
  assert(exes);
  assert(obj);

  uint16_t offset = exes->exe.code_size;

  memcpy(exes->exe.code + offset, obj->code, obj->code_size);
  exes->exe.code_size += obj->code_size;
  exes->exe.code_size += exes->exe.code_size % 2;

  for (int i = 0; i < obj->reloc_num; ++i) {
    exe_add_reloc_offset(&exes->exe, obj->relocs[i], offset);
  }

  // TODO: redo
  for (int i = 0; i < obj->global_count; ++i) {
    symbol_t *s = &obj->symbols[obj->globals[i]];
    global_entry_t global = {0};
    strncpy(global.name, s->image, LABEL_MAX_LEN);
    global.pos = s->pos;
    exe_state_add_global(exes, global, offset);
  }

  // TODO: redo
  for (int i = 0; i < obj->extern_count; ++i) {
    symbol_t *s = &obj->symbols[obj->externs[i]];
    extern_entry_t _extern = {0};
    strncpy(_extern.name, s->image, LABEL_MAX_LEN);
    _extern.pos_num = s->reloc_num;
    memcpy(_extern.pos, s->relocs, s->reloc_num);
    exe_state_add_extern(exes, _extern, offset);
  }
}

void exe_link_boilerplate(exe_state_t *exes) {
  assert(exes);

  obj_t boilerplate = {.code_size = 4,
                       .code = {NOP, JMP},
                       .reloc_num = 0,
                       .relocs = {},
                       .global_count = 0,
                       .globals = {},
                       .extern_count = 1,
                       .externs = {0},
                       .symbol_count = 1,
                       .symbols = {(symbol_t){.image = "_start",
                                              .pos = 0xFFFF,
                                              .relocs = {2},
                                              .reloc_num = 1,
                                              .relrelocs = {},
                                              .relreloc_num = 0}}};

  exe_link_obj(exes, &boilerplate);
}

void exe_state_dump(exe_state_t *exes) {
  assert(exes);

  printf("GLOBALS:\n");
  for (int i = 0; i < exes->global_num; ++i) {
    printf("\t%s %04X\n", exes->globals[i].name, exes->globals[i].pos);
  }
  printf("EXTERNS:\n");
  for (int i = 0; i < exes->extern_num; ++i) {
    printf("\t%s [", exes->externs[i].name);
    for (int j = 0; j < exes->externs[i].pos_num; ++j) {
      if (j != 0) {
        printf(" ");
      }
      printf("%04X", exes->externs[i].pos[j]);
    }
    printf("]\n");
  }
  printf("SOS: %d\n", exes->so_num);
  for (int i = 0; i < exes->so_num; ++i) {
    printf("%s", exes->so_names[i]);
    if (strcmp(exes->so_names[0], "\x01") == 0) {
      printf(" (STDLIB)");
    }
    printf(":\n");
    so_dump(&exes->sos[0]);
    printf("ENDSO\n");
  }
  exe_dump(&exes->exe);
}

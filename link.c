#include <assert.h>
#include <stdio.h>
#include <string.h>

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
    exe_add_reloc_offset(&exes->exe, obj->reloc_table[i], offset);
  }

  for (int i = 0; i < obj->global_num; ++i) {
    exe_state_add_global(exes, obj->globals[i], offset);
  }

  for (int i = 0; i < obj->extern_num; ++i) {
    exe_state_add_extern(exes, obj->externs[i], offset);
  }
}

exe_t link(obj_t *objs_list, int objs_count, link_debug_flag_t flag) {
  assert(objs_list);

  exe_state_t exes = {0};

  if (!(flag & LINK_FLAG_BIN)) {
    obj_t boilerplate = {0};
    strcpy(boilerplate.externs[0].name, "_start");
    boilerplate.externs[0].pos[0] = 2;
    ++boilerplate.externs[0].pos_num;
    ++boilerplate.extern_num;
    boilerplate.code[0] = NOP;
    boilerplate.code[1] = JMP;
    boilerplate.code_size = 4;

    exe_link_obj(&exes, &boilerplate);
  }

  for (int i = 0; i < objs_count; ++i) {
    exe_link_obj(&exes, &objs_list[i]);
  }

  if (flag & LINK_FLAG_EXE_STATE) {
    printf("GLOBALS:\n");
    for (int i = 0; i < exes.global_num; ++i) {
      printf("\t%s %04X\n", exes.globals[i].name, exes.globals[i].pos);
    }
    printf("EXTERNS:\n");
    for (int i = 0; i < exes.extern_num; ++i) {
      printf("\t%s [", exes.externs[i].name);
      for (int j = 0; j < exes.externs[i].pos_num; ++j) {
        if (j != 0) {
          printf(" ");
        }
        printf("%04X", exes.externs[i].pos[j]);
      }
      printf("]\n");
    }
    exe_dump(&exes.exe);
  }

  exe_state_check_exe(&exes);

  return exes.exe;
}

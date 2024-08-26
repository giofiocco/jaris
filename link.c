#include <assert.h>
#include <string.h>

#include "files.h"

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

exe_t link(obj_t *objs_list, int objs_count, int bin) {
  assert(objs_list);

  exe_state_t exes = {0};

  if (!bin) {
    obj_t boilerplate = {0};
    boilerplate.externs[0].name = sv_from_cstr("_start");
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
  exe_state_check_exe(&exes);

  return exes.exe;
}

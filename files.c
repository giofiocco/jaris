#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#define ERRORS_IMPLEMENTATION
#include "errors.h"
#define SV_IMPLEMENTATION
#include "sv.h"

#define TODO assert(0 && "TO IMPLEMENT")

void obj_dump(obj_t *obj) {
  assert(obj);

  printf("GLOBALS:\n");
  for (int i = 0; i < obj->global_num; ++i) {
    printf("\t" SV_FMT " %04X\n", SV_UNPACK(obj->globals[i].name), obj->globals[i].pos);
  }
  printf("EXTERNS:\n");
  for (int i = 0; i < obj->extern_num; ++i) {
    printf("\t" SV_FMT "[", SV_UNPACK(obj->externs[i].name));
    for (int j = 0; j < obj->externs[i].pos_num; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%04X", obj->externs[i].pos[j]);
    }
    printf("]\n");
  }
  printf("CODE:\n");
  printf("\t");
  for (int i = 0; i < obj->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", obj->code[i]);
  }
}

void obj_state_add_symbol(obj_state_t *objs, sv_t sv, uint16_t pos) {
  assert(objs);

  assert(objs->symbol_num + 1 < SYMBOL_COUNT);
  objs->symbols[objs->symbol_num].image = sv;
  objs->symbols[objs->symbol_num].pos = pos;
  ++objs->symbol_num;
}

uint16_t obj_state_find_symbol(obj_state_t *objs, sv_t sv) {
  assert(objs);

  for (int i = 0; i < objs->symbol_num; ++i) {
    if (sv_eq(sv, objs->symbols[i].image)) {
      return objs->symbols[i].pos;
    }
  }
  return 0xFFFF;
}

void obj_state_add_relreloc(obj_state_t *objs, sv_t sv, uint16_t pos) {
  assert(objs);

  assert(objs->relreloc_num + 1 < INTERN_RELOC_COUNT);
  objs->relrelocs[objs->relreloc_num++] = (symbol_t){sv, pos};
}

void obj_state_add_reloc(obj_state_t *objs, sv_t sv, uint16_t pos) {
  assert(objs);

  assert(objs->reloc_num + 1 < INTERN_RELOC_COUNT);
  objs->relocs[objs->reloc_num++] = (symbol_t){sv, pos};
}

void obj_state_check_obj(obj_state_t *objs) {
  assert(objs);

  for (int i = 0; i < objs->relreloc_num; ++i) {
    uint16_t pos = obj_state_find_symbol(objs, objs->relrelocs[i].image);
    if (pos == 0xFFFF) {
      eprintf("label unset: " SV_FMT, SV_UNPACK(objs->relrelocs[i].image));
    }

    uint16_t num = pos - objs->relrelocs[i].pos;
    objs->obj.code[objs->relrelocs[i].pos] = num & 0xFF;
    objs->obj.code[objs->relrelocs[i].pos + 1] = num >> 8;
  }

  for (int i = 0; i < objs->reloc_num; ++i) {
    uint16_t pos = obj_state_find_symbol(objs, objs->relocs[i].image);
    if (pos == 0xFFFF) {
      eprintf("label unset: " SV_FMT, SV_UNPACK(objs->relocs[i].image));
    }

    obj_add_reloc(&objs->obj, objs->relocs[i].pos, pos);
  }

  for (int i = 0; i < objs->obj.global_num; ++i) {
    uint16_t pos = obj_state_find_symbol(objs, objs->obj.globals[i].name);
    if (pos == 0xFFFF) {
      eprintf("label unset: " SV_FMT, SV_UNPACK(objs->obj.globals[i].name));
    }

    objs->obj.globals[i].pos = pos;
  }
}

void obj_compile_bytecode(obj_state_t *objs, bytecode_t bc) {
  assert(objs);

  switch (bc.kind) {
    case BNONE:
      assert(0);
    case BINST:
      obj_add_instruction(&objs->obj, bc.inst);
      break;
    case BINSTHEX:
      obj_add_instruction(&objs->obj, bc.inst);
      obj_add_hex(&objs->obj, bc.arg.num);
      break;
    case BINSTHEX2:
      obj_add_instruction(&objs->obj, bc.inst);
      obj_add_hex2(&objs->obj, bc.arg.num);
      break;
    case BINSTLABEL:
      obj_add_instruction(&objs->obj, bc.inst);
      obj_state_add_reloc(objs, bc.arg.sv, objs->obj.code_size);
      objs->obj.code_size += 2;
      break;
    case BINSTRELLABEL:
      {
        obj_add_instruction(&objs->obj, bc.inst);
        uint16_t pos = obj_state_find_symbol(objs, bc.arg.sv);
        if (pos != 0xFFFF) {
          obj_add_hex2(&objs->obj, (uint16_t)(pos - objs->obj.code_size));
        } else {
          obj_state_add_relreloc(objs, bc.arg.sv, objs->obj.code_size);
          objs->obj.code_size += 2;
        }
      }
      break;
    case BHEX:
      obj_add_hex(&objs->obj, bc.arg.num);
      break;
    case BHEX2:
      obj_add_hex2(&objs->obj, bc.arg.num);
      break;
    case BSTRING:
      for (unsigned int i = 0; i < bc.arg.sv.len - 1; ++i) {
        obj_add_hex(&objs->obj, bc.arg.sv.start[1 + i]);
      }
      break;
    case BSETLABEL:
      if (obj_state_find_symbol(objs, bc.arg.sv) != 0xFFFF) {
        eprintf("label redefinition " SV_FMT, SV_UNPACK(bc.arg.sv));
      }
      obj_state_add_symbol(objs, bc.arg.sv, objs->obj.code_size);
      break;
    case BGLOBAL:
      obj_add_global(&objs->obj, bc.arg.sv);
      break;
    case BEXTERN:
      TODO;
      break;
    case BALIGN:
      if (objs->obj.code_size % 2 == 1) {
        ++objs->obj.code_size;
      }
      break;
    case BDB:
      objs->obj.code_size += bc.arg.num;
      break;
  }
}

void obj_add_reloc(obj_t *obj, uint16_t where, uint16_t what) {
  assert(obj);

  assert(obj->reloc_num + 1 < RELOC_COUNT);
  obj->reloc_table[obj->reloc_num++] = (reloc_entry_t){where, what};
}

void obj_add_global(obj_t *obj, sv_t image) {
  assert(obj);

  assert(obj->global_num + 1 < GLOBAL_COUNT);
  obj->globals[obj->global_num++] = (global_entry_t){image, 0xFFFF};
}

void obj_add_instruction(obj_t *obj, instruction_t inst) {
  assert(obj);

  obj->code[obj->code_size++] = inst;
}

void obj_add_hex(obj_t *obj, uint8_t num) {
  assert(obj);

  obj->code[obj->code_size++] = num;
}

void obj_add_hex2(obj_t *obj, uint16_t num) {
  assert(obj);

  obj->code[obj->code_size++] = num & 0xFF;
  obj->code[obj->code_size++] = num >> 8;
}

obj_t obj_decode_file(char *filename, sv_allocator_t *alloc) {
  assert(filename);
  assert(alloc);

  obj_t obj = {0};

  FILE *file = fopen(filename, "r");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  char magic_number[3] = {0};
  assert(fread(magic_number, 1, 3, file) == 3);
  if (strcmp(magic_number, "OBJ") != 0) {
    eprintf("%s magic number is not OBJ: '%s'", filename, magic_number);
  }

  int global_table_size = 0;
  assert(fread(&global_table_size, 2, 1, file) == 2);
  while (global_table_size > 0) {
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    obj.globals[obj.global_num].name = sv_alloc_len(alloc, len);
    assert(fread(&obj.globals[obj.global_num].name.start, 1, len, file) == len);
    assert(fread(&obj.globals[obj.global_num].pos, 2, 1, file) == 2);
    ++obj.global_num;
    global_table_size -= 1 + len + 2;
  }
  assert(global_table_size == 0);

  int extern_table_size = 0;
  assert(fread(&extern_table_size, 2, 1, file) == 2);
  while (extern_table_size > 0) {
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    obj.externs[obj.extern_num].name = sv_alloc_len(alloc, len);
    assert(fread(&obj.externs[obj.extern_num].name.start, 1, len, file) == len);
    assert(fread(&obj.externs[obj.extern_num].pos_num, 1, 1, file) == 1);
    for (int i = 0; i < obj.externs[obj.extern_num].pos_num; ++i) {
      assert(fread(&obj.externs[obj.extern_num].pos[i], 2, 1, file) == 2);
    }
    ++obj.extern_num;
    extern_table_size -= 1 + len + 1 + 2 * obj.externs[obj.extern_num].pos_num;
  }
  assert(extern_table_size == 0);

  assert(fread(&obj.reloc_num, 2, 1, file) == 2);
  for (int i = 0; i < obj.reloc_num; ++i) {
    assert(fread(&obj.reloc_table[i].where, 2, 1, file) == 2);
    assert(fread(&obj.reloc_table[i].what, 2, 1, file) == 2);
  }

  assert(fread(&obj.code_size, 2, 1, file) == 2);
  assert(fread(&obj.code, 1, obj.code_size, file) == obj.code_size);

  assert(fclose(file) == 0);

  return obj;
}

void obj_encode_file(obj_t *obj, char *filename) {
  assert(obj);
  assert(filename);

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  assert(fwrite("OBJ", 1, 3, file) == 3);

  int global_table_size = 0;
  assert(fwrite(&global_table_size, 2, 1, file) == 2);
  for (int i = 0; i < obj->global_num; ++i) {
    size_t len = obj->globals[i].name.len;
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(&obj->globals[i].name.start, 1, len, file) == len);
    assert(fwrite(&obj->globals[i].pos, 2, 1, file) == 2);
    global_table_size += 1 + len + 2;
  }
  assert(fseek(file, 3, SEEK_SET) == 0);
  assert(fwrite(&global_table_size, 2, 1, file) == 2);
  assert(fseek(file, 5 + global_table_size, SEEK_SET) == 0);

  int extern_table_size = 0;
  assert(fwrite(&extern_table_size, 2, 1, file) == 2);
  for (int i = 0; i < obj->extern_num; ++i) {
    uint8_t len = obj->externs[i].name.len;
    uint8_t num = obj->externs[i].pos_num;
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(&obj->externs[i].name.start, 1, len, file) == len);
    assert(fwrite(&num, 1, 1, file) == 1);
    for (int j = 0; j < num; ++j) {
      assert(fwrite(&obj->externs[i].pos[j], 2, 1, file) == 2);
    }
    extern_table_size += 1 + len + 1 + 2 * num;
  }
  assert(fseek(file, 5 + global_table_size, SEEK_SET) == 0);
  assert(fwrite(&extern_table_size, 2, 1, file) == 2);
  assert(fseek(file, 5 + global_table_size + 2 + extern_table_size, SEEK_SET) == 0);

  assert(fwrite(&obj->reloc_num, 2, 1, file) == 2);
  for (int i = 0; i < obj->reloc_num; ++i) {
    assert(fwrite(&obj->reloc_table[i].where, 2, 1, file) == 2);
    assert(fwrite(&obj->reloc_table[i].what, 2, 1, file) == 2);
  }

  assert(fwrite(&obj->code_size, 2, 1, file) == 2);
  assert(fwrite(&obj->code, 1, obj->code_size, file) == obj->code_size);

  assert(fclose(file) == 0);
}

uint16_t exe_state_find_global(exe_state_t *exes, sv_t name) {
  assert(exes);

  for (int i = 0; i < exes->global_num; ++i) {
    if (sv_eq(exes->globals[i].name, name)) {
      return exes->globals[i].pos;
    }
  }

  return 0xFFFF;
}

void exe_state_add_global(exe_state_t *exes, global_entry_t global, uint16_t offset) {
  assert(exes);

  if (exe_state_find_global(exes, global.name) != 0xFFFF) {
    eprintf("global redefinition: " SV_FMT, SV_UNPACK(global.name));
  }

  assert(exes->global_num + 1 < GLOBAL_COUNT);
  global.pos += offset;
  exes->globals[exes->global_num++] = global;
}

void exe_state_add_extern(exe_state_t *exes, extern_entry_t extern_, uint16_t offset) {
  assert(exes);

  for (int i = 0; i < extern_.pos_num; ++i) {
    extern_.pos[i] += offset;
  }

  assert(exes->extern_num + 1 < GLOBAL_COUNT);
  exes->externs[exes->extern_num++] = extern_;
}

void exe_state_check_exe(exe_state_t *exes) {
  assert(exes);

  for (int i = 0; i < exes->extern_num; ++i) {
    extern_entry_t extern_ = exes->externs[i];
    uint16_t pos = exe_state_find_global(exes, extern_.name);
    if (pos != 0xFFFF) {
      eprintf("global unset: " SV_FMT, SV_UNPACK(extern_.name));
    }

    for (int j = 0; j < extern_.pos_num; ++j) {
      exe_add_reloc(&exes->exe, (reloc_entry_t){extern_.pos[i], pos});
    }
  }
}

void exe_add_reloc_offset(exe_t *exe, reloc_entry_t reloc, uint16_t offset) {
  assert(exe);

  assert(exe->reloc_num + 1 < RELOC_COUNT);
  reloc.where += offset;
  reloc.what += offset;
  exe->reloc_table[exe->reloc_num++] = reloc;
}

void exe_add_reloc(exe_t *exe, reloc_entry_t reloc) {
  assert(exe);

  assert(exe->reloc_num + 1 < RELOC_COUNT);
  exe->reloc_table[exe->reloc_num++] = reloc;
}

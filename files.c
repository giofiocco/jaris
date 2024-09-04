#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "mystb/errors.h"

#define TODO assert(0 && "TO IMPLEMENT")

void extern_entry_add_pos(extern_entry_t *e, uint16_t pos) {
  assert(e);

  assert(e->pos_num + 1 < EXTERN_COUNT);
  e->pos[e->pos_num++] = pos;
}

void obj_dump(obj_t *obj) {
  assert(obj);

  printf("GLOBALS:\n");
  for (int i = 0; i < obj->global_num; ++i) {
    printf("\t%s %04X\n", obj->globals[i].name, obj->globals[i].pos);
  }
  printf("EXTERNS:\n");
  for (int i = 0; i < obj->extern_num; ++i) {
    printf("\t%s [", obj->externs[i].name);
    for (int j = 0; j < obj->externs[i].pos_num; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%04X", obj->externs[i].pos[j]);
    }
    printf("]\n");
  }
  printf("RELOC:\n");
  for (int i = 0; i < obj->reloc_num; ++i) {
    printf("\t%04X %04X\n", obj->reloc_table[i].where, obj->reloc_table[i].what);
  }
  printf("CODE:\n");
  printf("\t");
  for (int i = 0; i < obj->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", obj->code[i]);
  }
  printf("\n");
}

void obj_state_add_label(obj_state_t *objs, char *label, uint16_t pos) {
  assert(objs);
  assert(label);

  assert(objs->label_num + 1 < LABEL_COUNT);
  strcpy(objs->labels[objs->label_num].image, label);
  objs->labels[objs->label_num].pos = pos;
  ++objs->label_num;
}

uint16_t obj_state_find_label(obj_state_t *objs, char *label) {
  assert(objs);
  assert(label);

  for (int i = 0; i < objs->label_num; ++i) {
    if (strcmp(label, objs->labels[i].image) == 0) {
      return objs->labels[i].pos;
    }
  }
  return 0xFFFF;
}

void obj_state_add_relreloc(obj_state_t *objs, char *name, uint16_t pos) {
  assert(objs);
  assert(name);

  assert(objs->relreloc_num + 1 < INTERN_RELOC_COUNT);
  label_t label = {0};
  strcpy(label.image, name);
  label.pos = pos;
  objs->relrelocs[objs->relreloc_num++] = label;
}

void obj_state_add_reloc(obj_state_t *objs, char *name, uint16_t pos) {
  assert(objs);
  assert(name);

  assert(objs->reloc_num + 1 < INTERN_RELOC_COUNT);
  label_t label = {0};
  strcpy(label.image, name);
  label.pos = pos;
  objs->relocs[objs->reloc_num++] = label;
}

void obj_state_check_obj(obj_state_t *objs) {
  assert(objs);

  for (int i = 0; i < objs->relreloc_num; ++i) {
    uint16_t pos = obj_state_find_label(objs, objs->relrelocs[i].image);
    if (pos == 0xFFFF) {
      eprintf("label unset: %s", objs->relrelocs[i].image);
    }

    uint16_t num = pos - objs->relrelocs[i].pos;
    objs->obj.code[objs->relrelocs[i].pos] = num & 0xFF;
    objs->obj.code[objs->relrelocs[i].pos + 1] = num >> 8;
  }

  for (int i = 0; i < objs->reloc_num; ++i) {
    uint16_t pos = obj_state_find_label(objs, objs->relocs[i].image);
    if (pos == 0xFFFF) {
      extern_entry_t *e = obj_find_extern(&objs->obj, objs->relocs[i].image);

      if (e == NULL) {
        eprintf("label unset: %s", objs->relocs[i].image);
      }

      extern_entry_add_pos(e, objs->relocs[i].pos);
    } else {
      obj_add_reloc(&objs->obj, objs->relocs[i].pos, pos);
    }
  }

  for (int i = 0; i < objs->obj.global_num; ++i) {
    uint16_t pos = obj_state_find_label(objs, objs->obj.globals[i].name);
    if (pos == 0xFFFF) {
      eprintf("label unset: %s", objs->obj.globals[i].name);
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
      obj_state_add_reloc(objs, bc.arg.string, objs->obj.code_size);
      objs->obj.code_size += 2;
      break;
    case BINSTRELLABEL:
      {
        obj_add_instruction(&objs->obj, bc.inst);
        uint16_t pos = obj_state_find_label(objs, bc.arg.string);
        if (pos != 0xFFFF) {
          obj_add_hex2(&objs->obj, (uint16_t)(pos - objs->obj.code_size));
        } else {
          obj_state_add_relreloc(objs, bc.arg.string, objs->obj.code_size);
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
      for (char *c = bc.arg.string; *c; ++c) {
        obj_add_hex(&objs->obj, *c);
      }
      break;
    case BSETLABEL:
      if (obj_state_find_label(objs, bc.arg.string) != 0xFFFF) {
        eprintf("label redefinition %s", bc.arg.string);
      }
      obj_state_add_label(objs, bc.arg.string, objs->obj.code_size);
      break;
    case BGLOBAL:
      obj_add_global(&objs->obj, bc.arg.string);
      break;
    case BEXTERN:
      obj_add_extern(&objs->obj, bc.arg.string);
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

void obj_add_global(obj_t *obj, char *image) {
  assert(obj);
  assert(image);

  assert(obj->global_num + 1 < GLOBAL_COUNT);
  global_entry_t entry = {0};
  strcpy(entry.name, image);
  entry.pos = 0xFFFF;
  obj->globals[obj->global_num++] = entry;
}

void obj_add_extern(obj_t *obj, char *image) {
  assert(obj);
  assert(image);

  assert(obj->extern_num + 1 < EXTERN_COUNT);
  extern_entry_t entry = {0};
  strcpy(entry.name, image);
  obj->externs[obj->extern_num++] = entry;
}

extern_entry_t *obj_find_extern(obj_t *obj, char *image) {
  assert(obj);
  assert(image);

  for (int i = 0; i < obj->extern_num; ++i) {
    if (strcmp(image, obj->externs[i].name) == 0) {
      return &obj->externs[i];
    }
  }

  return NULL;
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

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  char magic_number[3] = {0};
  assert(fread(magic_number, 1, 3, file) == 3);
  if (strcmp(magic_number, "OBJ") != 0) {
    eprintf("%s: expected magic number to be 'OBJ': found '%s'", filename, magic_number);
  }

  int global_table_size = 0;
  assert(fread(&global_table_size, 2, 1, file) == 1);
  while (global_table_size > 0) {
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    assert(fread(obj.globals[obj.global_num].name, 1, len, file) == len);
    assert(fread(&obj.globals[obj.global_num].pos, 2, 1, file) == 1);
    ++obj.global_num;
    global_table_size -= 1 + len + 2;
  }
  assert(global_table_size == 0);

  int extern_table_size = 0;
  assert(fread(&extern_table_size, 2, 1, file) == 1);
  while (extern_table_size > 0) {
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    assert(fread(obj.externs[obj.extern_num].name, 1, len, file) == len);
    assert(fread(&obj.externs[obj.extern_num].pos_num, 1, 1, file) == 1);
    for (int i = 0; i < obj.externs[obj.extern_num].pos_num; ++i) {
      assert(fread(&obj.externs[obj.extern_num].pos[i], 2, 1, file) == 1);
    }
    extern_table_size -= 1 + len + 1 + 2 * obj.externs[obj.extern_num].pos_num;
    ++obj.extern_num;
  }
  assert(extern_table_size == 0);

  assert(fread(&obj.reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < obj.reloc_num; ++i) {
    assert(fread(&obj.reloc_table[i].where, 2, 1, file) == 1);
    assert(fread(&obj.reloc_table[i].what, 2, 1, file) == 1);
  }

  assert(fread(&obj.code_size, 2, 1, file) == 1);
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
  assert(fwrite(&global_table_size, 2, 1, file) == 1);
  for (int i = 0; i < obj->global_num; ++i) {
    size_t len = strlen(obj->globals[i].name);
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(obj->globals[i].name, 1, len, file) == len);
    assert(fwrite(&obj->globals[i].pos, 2, 1, file) == 1);
    global_table_size += 1 + len + 2;
  }
  assert(fseek(file, 3, SEEK_SET) == 0);
  assert(fwrite(&global_table_size, 2, 1, file) == 1);
  assert(fseek(file, 5 + global_table_size, SEEK_SET) == 0);

  int extern_table_size = 0;
  assert(fwrite(&extern_table_size, 2, 1, file) == 1);
  for (int i = 0; i < obj->extern_num; ++i) {
    uint8_t len = strlen(obj->externs[i].name);
    uint8_t num = obj->externs[i].pos_num;
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(obj->externs[i].name, 1, len, file) == len);
    assert(fwrite(&num, 1, 1, file) == 1);
    for (int j = 0; j < num; ++j) {
      assert(fwrite(&obj->externs[i].pos[j], 2, 1, file) == 1);
    }
    extern_table_size += 1 + len + 1 + 2 * num;
  }
  assert(fseek(file, 5 + global_table_size, SEEK_SET) == 0);
  assert(fwrite(&extern_table_size, 2, 1, file) == 1);
  assert(fseek(file, 5 + global_table_size + 2 + extern_table_size, SEEK_SET) == 0);

  assert(fwrite(&obj->reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < obj->reloc_num; ++i) {
    assert(fwrite(&obj->reloc_table[i].where, 2, 1, file) == 1);
    assert(fwrite(&obj->reloc_table[i].what, 2, 1, file) == 1);
  }

  assert(fwrite(&obj->code_size, 2, 1, file) == 1);
  assert(fwrite(&obj->code, 1, obj->code_size, file) == obj->code_size);

  assert(fclose(file) == 0);
}

void exe_dump(exe_t *exe) {
  assert(exe);

  printf("CODE:\n");
  printf("\t");
  for (int i = 0; i < exe->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", exe->code[i]);
  }
  printf("\nRELOC:\n");
  for (int i = 0; i < exe->reloc_num; ++i) {
    printf("\t%04X %04X\n", exe->reloc_table[i].where, exe->reloc_table[i].what);
  }
}

uint16_t exe_state_find_global(exe_state_t *exes, char *name) {
  assert(exes);
  assert(name);

  for (int i = 0; i < exes->global_num; ++i) {
    if (strcmp(exes->globals[i].name, name) == 0) {
      return exes->globals[i].pos;
    }
  }

  return 0xFFFF;
}

void exe_state_add_global(exe_state_t *exes, global_entry_t global, uint16_t offset) {
  assert(exes);

  if (exe_state_find_global(exes, global.name) != 0xFFFF) {
    eprintf("global redefinition: %s", global.name);
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
    if (pos == 0xFFFF) {
      eprintf("global unset: %s", extern_.name);
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

exe_t exe_decode_file(char *filename) {
  assert(filename);

  exe_t exe = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  char magic_number[3] = {0};
  assert(fread(magic_number, 1, 3, file) == 3);
  if (strcmp(magic_number, "EXE") != 0) {
    eprintf("%s: expected magic number to be 'EXE': found '%s'", filename, magic_number);
  }

  assert(fread(&exe.code_size, 2, 1, file) == 1);
  assert(fread(exe.code, 1, exe.code_size, file) == exe.code_size);

  assert(fread(&exe.reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < exe.reloc_num; ++i) {
    assert(fread(&exe.reloc_table[i].where, 2, 1, file) == 1);
    assert(fread(&exe.reloc_table[i].what, 2, 1, file) == 1);
  }

  assert(fclose(file) == 0);

  return exe;
}

void exe_encode_file(exe_t *exe, char *filename) {
  assert(exe);
  assert(filename);

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  assert(fwrite("EXE", 1, 3, file) == 3);

  assert(fwrite(&exe->code_size, 2, 1, file) == 1);
  assert(fwrite(&exe->code, 1, exe->code_size, file) == exe->code_size);

  assert(fwrite(&exe->reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < exe->reloc_num; ++i) {
    assert(fwrite(&exe->reloc_table[i].where, 2, 1, file) == 1);
    assert(fwrite(&exe->reloc_table[i].what, 2, 1, file) == 1);
  }

  assert(fclose(file) == 0);
}

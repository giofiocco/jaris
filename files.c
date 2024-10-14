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

  printf("GLOBALS: %d\n", obj->global_num);
  for (int i = 0; i < obj->global_num; ++i) {
    printf("\t%s %04X\n", obj->globals[i].name, obj->globals[i].pos);
  }
  printf("EXTERNS: %d\n", obj->extern_num);
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
  printf("RELOC: %d\n", obj->reloc_num);
  for (int i = 0; i < obj->reloc_num; ++i) {
    printf("\t%04X %04X\n", obj->reloc_table[i].where, obj->reloc_table[i].what);
  }
  printf("CODE: %d\n", obj->code_size);
  printf("\t");
  for (int i = 0; i < obj->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", obj->code[i]);
  }
  printf("\n");
}

void obj_state_add_symbol(obj_state_t *objs, char *name, uint16_t pos) {
  assert(objs);
  assert(name);

  symbol_t *s = obj_state_find_symbol(objs, name);
  if (s) {
    if (s->pos != 0xFFFF) {
      eprintf("label redifinition: %s", name);
    }
    s->pos = pos;
    return;
  }

  assert(objs->symbol_num + 1 < SYMBOL_COUNT);
  strcpy(objs->symbols[objs->symbol_num].image, name);
  objs->symbols[objs->symbol_num].pos = pos;
  ++objs->symbol_num;
}

symbol_t *obj_state_find_symbol(obj_state_t *objs, char *name) {
  assert(objs);
  assert(name);

  for (int i = 0; i < objs->symbol_num; ++i) {
    if (strcmp(name, objs->symbols[i].image) == 0) {
      return &objs->symbols[i];
    }
  }
  return NULL;
}

uint16_t obj_state_find_symbol_pos(obj_state_t *objs, char *name) {
  assert(objs);
  assert(name);

  symbol_t *s = obj_state_find_symbol(objs, name);
  return s ? s->pos : 0xFFFF;
}

void obj_state_add_relreloc(obj_state_t *objs, char *name, uint16_t pos) {
  assert(objs);
  assert(name);

  symbol_t *s = obj_state_find_symbol(objs, name);
  if (s == NULL) {
    obj_state_add_symbol(objs, name, 0xFFFF);
    s = &objs->symbols[objs->symbol_num - 1];
  }
  assert(s->relreloc_num + 1 < RELOC_COUNT);
  s->relrelocs[s->relreloc_num++] = pos;
}

void obj_state_add_reloc(obj_state_t *objs, char *name, uint16_t pos) {
  assert(objs);
  assert(name);

  symbol_t *s = obj_state_find_symbol(objs, name);
  if (s == NULL) {
    obj_state_add_symbol(objs, name, 0xFFFF);
    s = &objs->symbols[objs->symbol_num - 1];
  }
  assert(s->reloc_num + 1 < RELOC_COUNT);
  s->relocs[s->reloc_num++] = pos;
}

void obj_state_check_obj(obj_state_t *objs) {
  assert(objs);

  symbol_t *s = NULL;
  extern_entry_t *e = NULL;
  for (int i = 0; i < objs->symbol_num; ++i) {
    s = &objs->symbols[i];
    e = NULL;

    if (s->pos == 0xFFFF) {
      e = obj_find_extern(&objs->obj, s->image);
      if (e == NULL) {
        eprintf("label undef: %s", s->image);
      }
      assert(s->relreloc_num == 0);
    }

    for (int j = 0; j < s->relreloc_num; ++j) {
      uint16_t num = s->pos - s->relrelocs[j];
      objs->obj.code[s->relrelocs[j]] = num & 0xFF;
      objs->obj.code[s->relrelocs[j] + 1] = num >> 8;
    }

    for (int j = 0; j < s->reloc_num; ++j) {
      if (e) {
        extern_entry_add_pos(e, s->relocs[j]);
      } else {
        obj_add_reloc(&objs->obj, s->relocs[j], s->pos);
      }
    }
  }

  for (int i = 0; i < objs->obj.global_num; ++i) {
    uint16_t pos = obj_state_find_symbol_pos(objs, objs->obj.globals[i].name);
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
      if (objs->obj.code_size % 2 == 0) {
        obj_add_instruction(&objs->obj, NOP);
      }
      obj_add_instruction(&objs->obj, bc.inst);
      obj_add_hex2(&objs->obj, bc.arg.num);
      break;
    case BINSTLABEL:
      if (objs->obj.code_size % 2 == 0) {
        obj_add_instruction(&objs->obj, NOP);
      }
      obj_add_instruction(&objs->obj, bc.inst);
      obj_state_add_reloc(objs, bc.arg.string, objs->obj.code_size);
      objs->obj.code_size += 2;
      break;
    case BINSTRELLABEL:
      {
        if (objs->obj.code_size % 2 == 0) {
          obj_add_instruction(&objs->obj, NOP);
        }
        obj_add_instruction(&objs->obj, bc.inst);
        uint16_t pos = obj_state_find_symbol_pos(objs, bc.arg.string);
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
      if (obj_state_find_symbol_pos(objs, bc.arg.string) != 0xFFFF) {
        eprintf("label redefinition %s", bc.arg.string);
      }
      obj_state_add_symbol(objs, bc.arg.string, objs->obj.code_size);
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

obj_t obj_decode_file(char *filename) {
  assert(filename);

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

  printf("CODE: %d\n", exe->code_size);
  printf("\t");
  for (int i = 0; i < exe->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", exe->code[i]);
  }
  printf("\nRELOC: %d\n", exe->reloc_num);
  for (int i = 0; i < exe->reloc_num; ++i) {
    printf("\t%04X %04X\n", exe->reloc_table[i].where, exe->reloc_table[i].what);
  }
  printf("DYNAMIC LINKING: %d\n", exe->dynamic_num);
  for (int i = 0; i < exe->dynamic_num; ++i) {
    if (exe->dynamics_table[i].file_name[0] == 1) {
      printf("\tSTD LIB:");
    } else {
      printf("\t%s:", exe->dynamics_table[i].file_name);
    }
    printf(" %d\n", exe->dynamics_table[i].reloc_num);
    for (int j = 0; j < exe->dynamics_table[i].reloc_num; ++j) {
      printf("\t\t%04X %04X\n",
             exe->dynamics_table[i].reloc_table[i].where,
             exe->dynamics_table[i].reloc_table[i].what);
    }
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

  int done[exes->extern_num];
  memset(done, 0, exes->extern_num * sizeof(int));
  int fileused = 0;

  for (int i = 0; i < exes->so_num; ++i) {
    fileused = 0;
    dynamic_entry_t *dt = &exes->exe.dynamics_table[exes->exe.dynamic_num];

    for (int j = 0; j < exes->extern_num; ++j) {
      extern_entry_t extern_ = exes->externs[j];

      uint16_t pos = so_find_global(&exes->sos[i], extern_.name);
      if (pos != 0xFFFF) {
        fileused = 1;
        done[j] = 1;
        for (int l = 0; l < extern_.pos_num; ++l) {
          dt->reloc_table[dt->reloc_num++] = (reloc_entry_t){extern_.pos[l], pos};
        }
      }
    }

    if (fileused) {
      ++exes->exe.dynamic_num;
      strcpy(dt->file_name, exes->so_names[i]);
    }
  }

  for (int i = 0; i < exes->extern_num; ++i) {
    if (!done[i]) {
      extern_entry_t extern_ = exes->externs[i];
      uint16_t pos = exe_state_find_global(exes, extern_.name);
      if (pos == 0xFFFF) {
        eprintf("global unset: %s", extern_.name);
      }

      for (int j = 0; j < extern_.pos_num; ++j) {
        exe_add_reloc(&exes->exe, (reloc_entry_t){extern_.pos[j], pos});
      }
    }
  }

  for (int i = 0; i < exes->exe.reloc_num; ++i) {
    reloc_entry_t *reloc = &exes->exe.reloc_table[i];
    exes->exe.code[reloc->where] = reloc->what & 0xFF;
    exes->exe.code[reloc->where + 1] = (reloc->what >> 8) & 0xFF;
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

#include <stdlib.h>
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

  while (1) {
    dynamic_entry_t *dt = &exe.dynamics_table[exe.dynamic_num];

    int i = 0;
    do {
      assert(fread(&dt->file_name[i], 1, 1, file) == 1);
    } while (dt->file_name[i++] != 0);

    if (dt->file_name[0] == 0) {
      break;
    }

    assert(fread(&dt->reloc_num, 2, 1, file) == 1);
    for (int j = 0; j < dt->reloc_num; ++j) {
      assert(fread(&dt->reloc_table[j].where, 2, 1, file) == 1);
      assert(fread(&dt->reloc_table[j].what, 2, 1, file) == 1);
    }

    ++exe.dynamic_num;
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

  for (int i = 0; i < exe->dynamic_num; ++i) {
    dynamic_entry_t *dt = &exe->dynamics_table[i];
    uint8_t len = strlen(dt->file_name) + 1;
    assert(fwrite(dt->file_name, 1, len, file) == len);
    assert(fwrite(&dt->reloc_num, 2, 1, file) == 1);
    for (int j = 0; j < dt->reloc_num; ++j) {
      assert(fwrite(&dt->reloc_table[j].where, 2, 1, file) == 1);
      assert(fwrite(&dt->reloc_table[j].what, 2, 1, file) == 1);
    }
  }
  uint8_t finaldltentry[] = {0};
  assert(fwrite(&finaldltentry, 1, sizeof(finaldltentry), file) == sizeof(finaldltentry));

  assert(fclose(file) == 0);
}

void bin_encode_file(exe_t *exe, char *filename) {
  assert(exe);
  assert(filename);

  assert(exe->reloc_num == 0);

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  assert(fwrite(&exe->code, 1, exe->code_size, file) == exe->code_size);

  assert(fclose(file) == 0);
}

void so_dump(so_t *so) {
  assert(so);

  printf("GLOBALS: %d\n", so->global_num);
  for (int i = 0; i < so->global_num; ++i) {
    printf("\t%s %04X\n", so->globals[i].name, so->globals[i].pos);
  }
  printf("CODE: %d\n", so->code_size);
  printf("\t");
  for (int i = 0; i < so->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", so->code[i]);
  }
  printf("\n");
  printf("RELOC: %d\n", so->reloc_num);
  for (int i = 0; i < so->reloc_num; ++i) {
    printf("\t%04X %04X\n", so->reloc_table[i].where, so->reloc_table[i].what);
  }
}

so_t so_decode_file(char *filename) {
  assert(filename);

  so_t so = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  char magic_number[2] = {0};
  assert(fread(magic_number, 1, 2, file) == 2);
  if (strcmp(magic_number, "SO") != 0) {
    eprintf("%s: expected magic number to be 'SO': found '%s'", filename, magic_number);
  }

  assert(fread(&so.code_size, 2, 1, file) == 1);
  assert(fread(so.code, 1, so.code_size, file) == so.code_size);

  assert(fread(&so.reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < so.reloc_num; ++i) {
    assert(fread(&so.reloc_table[i].where, 2, 1, file) == 1);
    assert(fread(&so.reloc_table[i].what, 2, 1, file) == 1);
  }

  int global_table_size = 0;
  assert(fread(&global_table_size, 1, 1, file) == 1);
  while (global_table_size > 0) {
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    assert(fread(so.globals[so.global_num].name, 1, len, file) == len);
    assert(fread(&so.globals[so.global_num].pos, 2, 1, file) == 1);
    ++so.global_num;
    global_table_size -= 1 + len + 2;
  }
  assert(global_table_size == 0);

  assert(fclose(file) == 0);

  return so;
}

void so_encode_file(exe_state_t *exes, char *filename) {
  assert(exes);
  assert(filename);

  exe_t *exe = &exes->exe;

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  assert(fwrite("SO", 1, 2, file) == 2);

  assert(fwrite(&exe->code_size, 2, 1, file) == 1);
  assert(fwrite(&exe->code, 1, exe->code_size, file) == exe->code_size);

  assert(fwrite(&exe->reloc_num, 2, 1, file) == 1);
  for (int i = 0; i < exe->reloc_num; ++i) {
    assert(fwrite(&exe->reloc_table[i].where, 2, 1, file) == 1);
    assert(fwrite(&exe->reloc_table[i].what, 2, 1, file) == 1);
  }

  int global_table_size_index = ftell(file);
  int global_table_size = 0;
  assert(fwrite(&global_table_size, 1, 1, file) == 1);
  for (int i = 0; i < exes->global_num; ++i) {
    size_t len = strlen(exes->globals[i].name);
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(exes->globals[i].name, 1, len, file) == len);
    assert(fwrite(&exes->globals[i].pos, 2, 1, file) == 1);
    global_table_size += 1 + len + 2;
  }
  assert(fseek(file, global_table_size_index, SEEK_SET) == 0);
  assert(fwrite(&global_table_size, 1, 1, file) == 1);

  assert(fclose(file) == 0);
}

uint16_t so_find_global(so_t *so, char *name) {
  assert(so);
  assert(name);

  for (int i = 0; i < so->global_num; ++i) {
    if (strcmp(so->globals[i].name, name) == 0) {
      return so->globals[i].pos;
    }
  }

  return 0xFFFF;
}

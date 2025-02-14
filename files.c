#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "instructions.h"
#include "mystb/errors.h"

#define TODO assert(0 && "TO IMPLEMENT")

void symbol_list_dump(symbol_t *symbols, uint16_t count) {
  assert(symbols);

  for (int i = 0; i < count; ++i) {
    symbol_t *s = &symbols[i];
    printf("%s: %04X\n", s->image, s->pos);
    printf("\tRELOCS:");
    for (int j = 0; j < s->reloc_count; ++j) {
      printf(" %04X\n", s->relocs[j]);
    }
    printf("\n");
    printf("\tRELRELOCS:");
    for (int j = 0; j < s->relreloc_count; ++j) {
      printf(" %04X\n", s->relrelocs[j]);
    }
    printf("\n");
  }
}

void symbols_list_decode(symbol_t *symbols, uint16_t count, FILE *file) {
  assert(symbols);
  assert(file);

  assert(fread(&count, 2, 1, file) == 1);
  for (int i = 0; i < count; ++i) {
    symbol_t *s = &symbols[i];
    size_t len = 0;
    assert(fread(&len, 1, 1, file) == 1);
    assert(len > 0);
    assert(fread(s->image, 1, len, file) == len);
    assert(fread(&s->pos, 2, 1, file) == 1);
    assert(fread(&s->reloc_count, 1, 1, file) == 1);
    assert(fread(s->relocs, 2, s->reloc_count, file) == s->reloc_count);
    assert(fread(&s->relreloc_count, 1, 1, file) == 1);
    assert(fread(s->relrelocs, 2, s->relreloc_count, file) == s->relreloc_count);
  }
}

void symbols_list_encode(symbol_t *symbols, uint16_t count, FILE *file) {
  assert(symbols);
  assert(file);

  assert(fwrite(&count, 2, 1, file) == 1);
  for (int i = 0; i < count; ++i) {
    symbol_t *s = &symbols[i];
    uint8_t len = strlen(s->image);
    assert(fwrite(&len, 1, 1, file) == 1);
    assert(fwrite(s->image, 1, len, file) == len);
    assert(fwrite(&s->pos, 2, 1, file) == 1);
    assert(fwrite(&s->reloc_count, 1, 1, file) == 1);
    assert(fwrite(s->relocs, 2, s->reloc_count, file) == s->reloc_count);
    assert(fwrite(&s->relreloc_count, 1, 1, file) == 1);
    assert(fwrite(s->relrelocs, 2, s->relreloc_count, file) == s->relreloc_count);
  }
}

void obj_dump(obj_t *obj) {
  assert(obj);

  printf("CODE: %d\n", obj->code_size);
  printf("\t");
  for (int i = 0; i < obj->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", obj->code[i]);
  }
  printf("\n");
  printf("GLOBALS:");
  for (int i = 0; i < obj->global_count; ++i) {
    printf(" %d", obj->globals[i]);
  }
  printf("\n");
  printf("EXTERNS:");
  for (int i = 0; i < obj->extern_count; ++i) {
    printf(" %d", obj->externs[i]);
  }
  printf("\n");
  printf("SYMBOLS: %d\n", obj->symbol_count);
  symbol_list_dump(obj->symbols, obj->symbol_count);
}

uint16_t obj_add_symbol(obj_t *obj, char *name, uint16_t pos) {
  assert(obj);
  assert(name);

  for (int i = 0; i < obj->symbol_count; ++i) {
    if (strcmp(obj->symbols[i].image, name) != 0) {
      continue;
    }
    if (obj->symbols[i].pos != 0xFFFF) {
      eprintf("label redifinition: %s", name);
    }
    obj->symbols[i].pos = pos;
    return i;
  }

  assert(obj->symbol_count + 1 < SYMBOL_MAX_COUNT);
  strncpy(obj->symbols[obj->symbol_count].image, name, LABEL_MAX_LEN);
  obj->symbols[obj->symbol_count].pos = pos;
  ++obj->symbol_count;
  return obj->symbol_count - 1;
}

symbol_t *obj_find_symbol(obj_t *obj, char *name) {
  assert(obj);
  assert(name);

  for (int i = 0; i < obj->symbol_count; ++i) {
    if (strcmp(name, obj->symbols[i].image) == 0) {
      return &obj->symbols[i];
    }
  }
  return NULL;
}

uint16_t obj_find_symbol_pos(obj_t *obj, char *name) {
  assert(obj);
  assert(name);

  symbol_t *s = obj_find_symbol(obj, name);
  return s ? s->pos : 0xFFFF;
}

void obj_add_symbol_reloc(obj_t *obj, char *name, uint16_t pos) {
  assert(obj);
  assert(name);

  symbol_t *s = obj_find_symbol(obj, name);
  if (s == NULL) {
    s = &obj->symbols[obj_add_symbol(obj, name, 0xFFFF)];
  }
  assert(s->reloc_count + 1 < RELOC_MAX_COUNT);
  s->relocs[s->reloc_count++] = pos;
}

void obj_add_symbol_relreloc(obj_t *obj, char *name, uint16_t pos) {
  assert(obj);
  assert(name);

  symbol_t *s = obj_find_symbol(obj, name);
  if (s == NULL) {
    s = &obj->symbols[obj_add_symbol(obj, name, 0xFFFF)];
  }
  assert(s->relreloc_count + 1 < RELOC_MAX_COUNT);
  s->relrelocs[s->relreloc_count++] = pos;
}

void obj_check(obj_t *obj, int debug_info) {
  assert(obj);

  for (int i = 0; i < obj->extern_count; ++i) {
    assert(obj->symbols[obj->externs[i]].pos == 0xFFFF);
    assert(obj->symbols[obj->externs[i]].relreloc_count == 0);
  }
  for (int i = 0; i < obj->global_count; ++i) {
    if (obj->symbols[obj->globals[i]].pos == 0xFFFF) {
      eprintf("label unset: %s", obj->symbols[obj->globals[i]].image);
    }
  }
  for (int i = 0; i < obj->symbol_count; ++i) {
    symbol_t *s = &obj->symbols[i];
    for (int j = 0; j < s->relreloc_count; ++j) {
      uint16_t num = s->pos - s->relrelocs[j];
      obj->code[s->relrelocs[j]] = num & 0xFF;
      obj->code[s->relrelocs[j] + 1] = (num >> 8) & 0xFF;
    }
    for (int j = 0; j < s->reloc_count; ++j) {
      obj->relocs[obj->reloc_count++] = (reloc_entry_t){s->relocs[j], s->pos};
      obj->code[s->relocs[j]] = s->pos & 0xFF;
      obj->code[s->relocs[j] + 1] = (s->pos >> 8) & 0xFF;
    }
  }

  if (!debug_info) {
    obj_t new = {0};
    new.code_size = obj->code_size;
    memcpy(new.code, obj->code, obj->code_size);
    new.reloc_count = obj->reloc_count;
    memcpy(new.relocs, obj->relocs, obj->reloc_count * sizeof(reloc_entry_t));
    new.global_count = obj->global_count;
    new.extern_count = obj->extern_count;

    for (int i = 0; i < obj->global_count; ++i) {
      new.globals[i] = new.symbol_count;
      symbol_t *s = &new.symbols[new.symbol_count++];
      memcpy(s, &obj->symbols[obj->globals[i]], sizeof(symbol_t));
      memset(s->relrelocs, 0, s->relreloc_count * sizeof(uint16_t));
      s->relreloc_count = 0;
    }
    for (int i = 0; i < obj->extern_count; ++i) {
      new.externs[i] = new.symbol_count;
      memcpy(&new.symbols[new.symbol_count++], &obj->symbols[obj->externs[i]], sizeof(symbol_t));
    }
    *obj = new;
  }
}

void obj_compile_bytecode(obj_t *obj, bytecode_t bc) {
  assert(obj);

  switch (bc.kind) {
    case BNONE:
      assert(0);
    case BINST:
      obj_add_instruction(obj, bc.inst);
      break;
    case BINSTHEX:
      obj_add_instruction(obj, bc.inst);
      obj_add_hex(obj, bc.arg.num);
      break;
    case BINSTHEX2:
      if (obj->code_size % 2 == 0) {
        obj_add_instruction(obj, NOP);
      }
      obj_add_instruction(obj, bc.inst);
      obj_add_hex2(obj, bc.arg.num);
      break;
    case BINSTLABEL:
      if (obj->code_size % 2 == 0) {
        obj_add_instruction(obj, NOP);
      }
      obj_add_instruction(obj, bc.inst);
      obj_add_symbol_reloc(obj, bc.arg.string, obj->code_size);
      obj->code_size += 2;
      break;
    case BINSTRELLABEL:
      if (obj->code_size % 2 == 0) {
        obj_add_instruction(obj, NOP);
      }
      obj_add_instruction(obj, bc.inst);
      obj_add_symbol_relreloc(obj, bc.arg.string, obj->code_size);
      obj->code_size += 2;
      break;
    case BHEX:
      obj_add_hex(obj, bc.arg.num);
      break;
    case BHEX2:
      obj_add_hex2(obj, bc.arg.num);
      break;
    case BSTRING:
      for (char *c = bc.arg.string; *c; ++c) {
        obj_add_hex(obj, *c);
      }
      break;
    case BSETLABEL:
      if (obj_find_symbol_pos(obj, bc.arg.string) != 0xFFFF) {
        eprintf("label redefinition %s", bc.arg.string);
      }
      obj_add_symbol(obj, bc.arg.string, obj->code_size);
      break;
    case BGLOBAL:
    {
      uint16_t i = obj_add_symbol(obj, bc.arg.string, 0xFFFF);
      assert(obj->global_count + 1 < GLOBAL_MAX_COUNT);
      obj->globals[obj->global_count++] = i;
    } break;
    case BEXTERN:
    {
      uint16_t i = obj_add_symbol(obj, bc.arg.string, 0xFFFF);
      assert(obj->extern_count + 1 < EXTERN_MAX_COUNT);
      obj->externs[obj->extern_count++] = i;
    } break;
    case BALIGN:
      if (obj->code_size % 2 == 1) {
        ++obj->code_size;
      }
      break;
    case BDB:
      obj->code_size += bc.arg.num;
      break;
  }
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

  char magic_number[4] = {0};
  assert(fread(magic_number, 1, 3, file) == 3);
  if (strcmp(magic_number, "OBJ") != 0) {
    eprintf("%s: expected magic number to be 'OBJ': found '%s'", filename, magic_number);
  }

  assert(fread(&obj.code_size, 2, 1, file) == 1);
  assert(fread(obj.code, 1, obj.code_size, file) == obj.code_size);
  assert(fread(&obj.global_count, 1, 1, file) == 1);
  assert(fread(obj.globals, 2, obj.global_count, file) == obj.global_count);
  assert(fread(&obj.extern_count, 1, 1, file) == 1);
  assert(fread(obj.externs, 2, obj.extern_count, file) == obj.extern_count);
  symbols_list_decode(obj.symbols, obj.symbol_count, file);

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
  assert(fwrite(&obj->code_size, 2, 1, file) == 1);
  assert(fwrite(&obj->code, 1, obj->code_size, file) == obj->code_size);
  assert(fwrite(&obj->global_count, 1, 1, file) == 1);
  assert(fwrite(obj->globals, 2, obj->global_count, file) == obj->global_count);
  assert(fwrite(&obj->extern_count, 1, 1, file) == 1);
  assert(fwrite(obj->externs, 2, obj->extern_count, file) == obj->extern_count);
  symbols_list_encode(obj->symbols, obj->symbol_count, file);

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
  printf("\nRELOC: %d\n", exe->reloc_count);
  for (int i = 0; i < exe->reloc_count; ++i) {
    printf("\t%04X %04X\n", exe->relocs[i].where, exe->relocs[i].what);
  }
  printf("DYNAMIC LINKING: %d\n", exe->dynamic_count);
  for (int i = 0; i < exe->dynamic_count; ++i) {
    if (exe->dynamics[i].file_name[0] == 1) {
      printf("\tSTD LIB:");
    } else {
      printf("\t%s:", exe->dynamics[i].file_name);
    }
    printf(" %d\n", exe->dynamics[i].reloc_count);
    for (int j = 0; j < exe->dynamics[i].reloc_count; ++j) {
      printf("\t\t%04X %04X\n", exe->dynamics[i].relocs[j].where, exe->dynamics[i].relocs[j].what);
    }
  }
}

void exe_add_symbol_offset(exe_t *exe, symbol_t *from, uint16_t offset) {
  assert(exe);
  assert(from);

  assert(exe->symbol_count + 1 < SYMBOL_MAX_COUNT);
  symbol_t *s = &exe->symbols[exe->symbol_count++];
  memcpy(s, from, sizeof(symbol_t));
  s->pos += offset;
  for (int j = 0; j < s->reloc_count; ++j) {
    s->relocs[j] += offset;
  }
  for (int j = 0; j < s->relreloc_count; ++j) {
    s->relrelocs[j] += offset;
  }
}

uint16_t exe_state_find_global(exe_state_t *state, char *name) {
  assert(state);
  assert(name);

  for (int i = 0; i < state->global_count; ++i) {
    if (strcmp(state->exe.symbols[state->globals[i]].image, name) == 0) {
      return state->exe.symbols[state->globals[i]].pos;
    }
  }

  return 0xFFFF;
}

uint16_t exe_state_find_so_global(exe_state_t *state, int so_index, char *name) {
  assert(state);
  assert(name);

  for (int k = 0; k < state->so_global_counts[so_index]; ++k) {
    if (strcmp(state->so_gloabals_images[so_index][k], name) == 0) {
      return state->so_globals_pos[so_index][k];
    }
  }
  return 0xFFFF;
}

void exe_state_check_exe(exe_state_t *state) {
  assert(state);

  int done[state->extern_count];
  memset(done, 0, state->extern_count * sizeof(int));

  for (int i = 0; i < state->so_count; ++i) {
    int file_used = 0;
    dynamic_entry_t *de = NULL;

    for (int j = 0; j < state->extern_count; ++j) {
      if (done[j]) {
        continue;
      }
      symbol_t *s = &state->exe.symbols[state->externs[j]];

      uint16_t pos = exe_state_find_so_global(state, i, s->image);
      if (pos == 0xFFFF) {
        continue;
      }
      done[j] = 1;

      if (file_used == 0) {
        file_used = 1;
        de = &state->exe.dynamics[state->exe.dynamic_count++];
        strncpy(de->file_name, state->so_names[i], FILE_NAME_MAX_LEN);
      }
      assert(de != NULL);

      for (int k = 0; k < s->reloc_count; ++k) {
        assert(de->reloc_count + 1 < RELOC_MAX_COUNT);
        de->relocs[de->reloc_count] = (reloc_entry_t){s->relocs[i], pos};
      }
    }
  }

  for (int i = 0; i < state->extern_count; ++i) {
    if (done[i]) {
      continue;
    }
    symbol_t *s = &state->exe.symbols[i];

    uint16_t pos = exe_state_find_global(state, s->image);
    if (pos == 0xFFFF) {
      eprintf("global unset: %s", s->image);
    }

    for (int j = 0; j < s->reloc_count; ++j) {
      assert(state->exe.reloc_count + 1 < RELOC_MAX_COUNT);
      state->exe.relocs[state->exe.reloc_count++] = (reloc_entry_t){s->relocs[j], pos};
    }
  }

  for (int i = 0; i < state->exe.reloc_count; ++i) {
    reloc_entry_t *reloc = &state->exe.relocs[i];
    state->exe.code[reloc->where] = reloc->what & 0xFF;
    state->exe.code[reloc->where + 1] = (reloc->what >> 8) & 0xFF;
  }
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

  assert(fread(&exe.reloc_count, 2, 1, file) == 1);
  for (int i = 0; i < exe.reloc_count; ++i) {
    assert(fread(&exe.relocs[i].where, 2, 1, file) == 1);
    assert(fread(&exe.relocs[i].what, 2, 1, file) == 1);
  }

  while (1) {
    dynamic_entry_t *dt = &exe.dynamics[exe.dynamic_count];

    int i = 0;
    do {
      assert(fread(&dt->file_name[i], 1, 1, file) == 1);
    } while (dt->file_name[i++] != 0);

    if (dt->file_name[0] == 0) {
      break;
    }

    assert(fread(&dt->reloc_count, 2, 1, file) == 1);
    for (int j = 0; j < dt->reloc_count; ++j) {
      assert(fread(&dt->relocs[j].where, 2, 1, file) == 1);
      assert(fread(&dt->relocs[j].what, 2, 1, file) == 1);
    }

    ++exe.dynamic_count;
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

  assert(fwrite(&exe->reloc_count, 2, 1, file) == 1);
  for (int i = 0; i < exe->reloc_count; ++i) {
    assert(fwrite(&exe->relocs[i].where, 2, 1, file) == 1);
    assert(fwrite(&exe->relocs[i].what, 2, 1, file) == 1);
  }

  for (int i = 0; i < exe->dynamic_count; ++i) {
    dynamic_entry_t *dt = &exe->dynamics[i];
    uint8_t len = strlen(dt->file_name) + 1;
    assert(fwrite(dt->file_name, 1, len, file) == len);
    assert(fwrite(&dt->reloc_count, 2, 1, file) == 1);
    for (int j = 0; j < dt->reloc_count; ++j) {
      assert(fwrite(&dt->relocs[j].where, 2, 1, file) == 1);
      assert(fwrite(&dt->relocs[j].what, 2, 1, file) == 1);
    }
  }
  int ending = 0;
  assert(fwrite(&ending, 1, 1, file) == 1);

  assert(fclose(file) == 0);
}

void bin_encode_file(exe_t *exe, char *filename) {
  assert(exe);
  assert(filename);

  if (exe->reloc_count != 0) {
    eprintf("cannot encode bin file with relocations\n");
  }

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  assert(fwrite(&exe->code, 1, exe->code_size, file) == exe->code_size);

  assert(fclose(file) == 0);
}

void so_dump(so_t *so) {
  assert(so);

  printf("CODE: %d\n", so->code_size);
  printf("\t");
  for (int i = 0; i < so->code_size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", so->code[i]);
  }
  printf("\n");
  printf("GLOBALS:");
  for (int i = 0; i < so->global_count; ++i) {
    printf(" %d", so->globals[i]);
  }
  printf("\n");
  printf("\n");
  printf("SYMBOLS: %d\n", so->symbol_count);
  for (int i = 0; i < so->symbol_count; ++i) {
    symbol_t *s = &so->symbols[i];
    printf("\t%s: %04X\n", s->image, s->pos);
    if (s->reloc_count > 0) {
      printf("\t\tRELOC: [");
      for (int j = 0; j < s->reloc_count; ++j) {
        if (j != 0) {
          printf(", ");
        }
        printf("%04X", s->relocs[j]);
      }
      printf("]\n");
    }
    if (s->relreloc_count > 0) {
      printf("\t\tRELRELOC: [");
      for (int j = 0; j < s->relreloc_count; ++j) {
        if (j != 0) {
          printf(", ");
        }
        printf("%04X", s->relrelocs[j]);
      }
      printf("]\n");
    }
  }
}

so_t so_decode_file(char *filename) {
  assert(filename);

  so_t so = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }

  char magic_number[3] = {0};
  assert(fread(magic_number, 1, 2, file) == 2);
  if (strcmp(magic_number, "SO") != 0) {
    eprintf("%s: expected magic number to be 'SO': found '%s'", filename, magic_number);
  }
  assert(fread(&so.code_size, 2, 1, file) == 1);
  assert(fread(so.code, 1, so.code_size, file) == so.code_size);
  assert(fread(&so.global_count, 1, 1, file) == 1);
  assert(fread(so.globals, 2, so.global_count, file) == so.global_count);
  symbols_list_decode(so.symbols, so.symbol_count, file);

  assert(fclose(file) == 0);

  return so;
}

void so_encode_file(so_t *so, char *filename) {
  assert(so);
  assert(filename);

  FILE *file = fopen(filename, "wb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fwrite("SO", 1, 2, file) == 2);
  assert(fwrite(&so->code_size, 2, 1, file) == 1);
  assert(fwrite(&so->code, 1, so->code_size, file) == so->code_size);
  assert(fwrite(&so->global_count, 1, 1, file) == 1);
  assert(fwrite(so->globals, 2, so->global_count, file) == so->global_count);
  symbols_list_encode(so->symbols, so->symbol_count, file);

  assert(fclose(file) == 0);
}

so_t so_from_exe_state(exe_state_t *state) {
  assert(state);

  exe_t *exe = &state->exe;

  so_t so = {0};

  if (exe->dynamic_count != 0 || state->so_count != 0) {
    eprintf("cannot encode so file with dynamic relocs\n");
  }

  so.code_size = exe->code_size;
  memcpy(so.code, exe->code, exe->code_size);
  so.reloc_count = exe->reloc_count;
  memcpy(so.relocs, exe->relocs, exe->reloc_count * sizeof(reloc_entry_t));
  so.symbol_count = exe->symbol_count;
  memcpy(so.symbols, exe->symbols, exe->symbol_count * sizeof(symbol_t));
  so.global_count = state->global_count;
  memcpy(so.globals, state->globals, state->global_count * sizeof(uint16_t));

  return so;
}

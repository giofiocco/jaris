#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "../instructions.h"
#include "../mystb/errors.h"
#include "files.h"

#define TODO assert(0 && "TO IMPLEMENT")

void symbol_list_dump(symbol_t *symbols, uint16_t count) {
  assert(symbols);

  printf("SYMBOLS: %d\n", count);
  for (int i = 0; i < count; ++i) {
    symbol_t *s = &symbols[i];
    printf("%02d) %s: %04X\n", i, s->image, s->pos);
    if (s->reloc_count + s->relreloc_count > 0) {
      printf("\t");
      for (int j = 0; j < s->reloc_count; ++j) {
        printf("%04X ", s->relocs[j]);
      }
      for (int j = 0; j < s->relreloc_count; ++j) {
        printf("$%04X ", s->relrelocs[j]);
      }
      printf("\n");
    }
  }
}

void symbols_list_decode(symbol_t *symbols, uint16_t *count, FILE *file) {
  assert(symbols);
  assert(count);
  assert(file);

  uint16_t n = 0;
  assert(fread(&n, 2, 1, file) == 1);
  assert(*count + n < SYMBOL_MAX_COUNT);
  for (int i = 0; i < n; ++i) {
    symbol_t *s = &symbols[(*count)++];
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

symbol_t *symbols_list_find(symbol_t *symbols, uint16_t count, char *image) {
  assert(symbols);
  for (int i = 0; i < count; ++i) {
    if (strcmp(symbols[i].image, image) == 0) {
      return &symbols[i];
    }
  }
  return NULL;
}

void code_dump(uint8_t *code, uint16_t code_size) {
  assert(code);
  printf("CODE: %d", code_size);
  for (int i = 0; i < code_size; ++i) {
    printf("%s%02X", i % 32 == 0 ? "\n\t" : " ", code[i]);
  }
  printf("\n");
}

void relocs_dump(reloc_entry_t *relocs, uint16_t reloc_count) {
  assert(relocs);

  printf("RELOCS: %d\n", reloc_count);
  for (int i = 0; i < reloc_count; ++i) {
    printf("\t%04X %04X\n", relocs[i].where, relocs[i].what);
  }
}

void obj_dump(obj_t *obj) {
  assert(obj);

  code_dump(obj->code, obj->code_size);
  relocs_dump(obj->relocs, obj->reloc_count);
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
  assert(s->reloc_count + 1 < INTERN_RELOC_MAX_COUNT);
  s->relocs[s->reloc_count++] = pos;
}

void obj_add_symbol_relreloc(obj_t *obj, char *name, uint16_t pos) {
  assert(obj);
  assert(name);

  symbol_t *s = obj_find_symbol(obj, name);
  if (s == NULL) {
    s = &obj->symbols[obj_add_symbol(obj, name, 0xFFFF)];
  }
  assert(s->relreloc_count + 1 < INTERN_RELOC_MAX_COUNT);
  s->relrelocs[s->relreloc_count++] = pos;
}

void obj_check(obj_t *obj, int debug_info) {
  assert(obj);

  uint8_t is_extern[obj->symbol_count];
  memset(is_extern, 0, obj->symbol_count * sizeof(is_extern[0]));

  for (int i = 0; i < obj->extern_count; ++i) {
    assert(obj->symbols[obj->externs[i]].pos == 0xFFFF);
    assert(obj->symbols[obj->externs[i]].relreloc_count == 0);
    is_extern[obj->externs[i]] = 1;
  }
  for (int i = 0; i < obj->symbol_count; ++i) {
    symbol_t *s = &obj->symbols[i];
    for (int j = 0; j < s->relreloc_count; ++j) {
      uint16_t num = s->pos - s->relrelocs[j];
      obj->code[s->relrelocs[j]] = num & 0xFF;
      obj->code[s->relrelocs[j] + 1] = (num >> 8) & 0xFF;
    }
    if (is_extern[i]) {
      continue;
    }
    if (s->pos == 0xFFFF) {
      eprintf("label unset: %s", obj->symbols[i].image);
    }
    for (int j = 0; j < s->reloc_count; ++j) {
      assert(obj->reloc_count + 1 < RELOC_MAX_COUNT);
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
  assert(fread(&obj.reloc_count, 2, 1, file) == 1);
  for (int i = 0; i < obj.reloc_count; ++i) {
    assert(fread(&obj.relocs[i].where, 2, 1, file) == 1);
    assert(fread(&obj.relocs[i].what, 2, 1, file) == 1);
  }
  assert(fread(&obj.global_count, 1, 1, file) == 1);
  assert(fread(obj.globals, 2, obj.global_count, file) == obj.global_count);
  assert(fread(&obj.extern_count, 1, 1, file) == 1);
  assert(fread(obj.externs, 2, obj.extern_count, file) == obj.extern_count);
  symbols_list_decode(obj.symbols, &obj.symbol_count, file);

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
  assert(fwrite(&obj->reloc_count, 2, 1, file) == 1);
  for (int i = 0; i < obj->reloc_count; ++i) {
    assert(fwrite(&obj->relocs[i].where, 2, 1, file) == 1);
    assert(fwrite(&obj->relocs[i].what, 2, 1, file) == 1);
  }
  assert(fwrite(&obj->global_count, 1, 1, file) == 1);
  assert(fwrite(obj->globals, 2, obj->global_count, file) == obj->global_count);
  assert(fwrite(&obj->extern_count, 1, 1, file) == 1);
  assert(fwrite(obj->externs, 2, obj->extern_count, file) == obj->extern_count);
  symbols_list_encode(obj->symbols, obj->symbol_count, file);

  assert(fclose(file) == 0);
}

void exe_dump(exe_t *exe) {
  assert(exe);

  code_dump(exe->code, exe->code_size);
  relocs_dump(exe->relocs, exe->reloc_count);
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
  symbol_list_dump(exe->symbols, exe->symbol_count);
}

void exe_add_symbol_offset(exe_t *exe, symbol_t *from, uint16_t offset) {
  assert(exe);
  assert(from);

  assert(exe->symbol_count + 1 < SYMBOL_MAX_COUNT);
  symbol_t *s = &exe->symbols[exe->symbol_count++];
  memcpy(s, from, sizeof(symbol_t));
  if (from->pos != 0xFFFF) {
    s->pos += offset;
  }
  for (int j = 0; j < s->reloc_count; ++j) {
    s->relocs[j] += offset;
  }
  for (int j = 0; j < s->relreloc_count; ++j) {

    s->relrelocs[j] += offset;
  }
}

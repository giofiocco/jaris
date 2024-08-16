#include <assert.h>
#include <stdio.h>

#include "errors.h"
#include "files.h"

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

    obj_add_reloc(objs, objs->relocs[i].pos, pos);
  }

  for (int i = 0; i < objs->obj.global_num; ++i) {
    uint16_t pos = obj_state_find_symbol(objs, objs->obj.globals[i].name);
    if (pos == 0xFFFF) {
      eprintf("label unset: " SV_FMT, SV_UNPACK(objs->obj.globals[i].name));
    }

    objs->obj.globals[i].pos = pos;
  }
}

void obj_add_reloc(obj_state_t *objs, uint16_t where, uint16_t what) {
  assert(objs);

  assert(objs->obj.reloc_num + 1 < RELOC_COUNT);
  objs->obj.reloc_table[objs->obj.reloc_num++] = (reloc_entry_t){where, what};
}

void obj_add_global(obj_state_t *objs, sv_t image) {
  assert(objs);

  assert(objs->obj.global_num + 1 < GLOBAL_COUNT);
  objs->obj.globals[objs->obj.global_num++] = (global_entry_t){image, obj_state_find_symbol(objs, image)};
}

void obj_add_instruction(obj_state_t *objs, instruction_t inst) {
  assert(objs);

  objs->obj.code[objs->obj.code_size++] = inst;
}

void obj_add_hex(obj_state_t *objs, uint8_t num) {
  assert(objs);

  objs->obj.code[objs->obj.code_size++] = num;
}

void obj_add_hex2(obj_state_t *objs, uint16_t num) {
  assert(objs);

  objs->obj.code[objs->obj.code_size++] = num & 0xFF;
  objs->obj.code[objs->obj.code_size++] = num >> 8;
}

void obj_compile_bytecode(obj_state_t *objs, bytecode_t bc) {
  assert(objs);

  switch (bc.kind) {
    case BNONE:
      assert(0);
    case BINST:
      obj_add_instruction(objs, bc.inst);
      break;
    case BINSTHEX:
      obj_add_instruction(objs, bc.inst);
      obj_add_hex(objs, bc.arg.num);
      break;
    case BINSTHEX2:
      obj_add_instruction(objs, bc.inst);
      obj_add_hex2(objs, bc.arg.num);
      break;
    case BINSTLABEL:
      obj_add_instruction(objs, bc.inst);
      obj_state_add_reloc(objs, bc.arg.sv, objs->obj.code_size);
      objs->obj.code_size += 2;
      break;
    case BINSTRELLABEL:
      {
        obj_add_instruction(objs, bc.inst);
        uint16_t pos = obj_state_find_symbol(objs, bc.arg.sv);
        if (pos != (uint16_t)-1) {
          obj_add_hex2(objs, (uint16_t)(pos - objs->obj.code_size));
        } else {
          obj_state_add_relreloc(objs, bc.arg.sv, objs->obj.code_size);
          objs->obj.code_size += 2;
        }
      }
      break;
    case BHEX:
      obj_add_hex(objs, bc.arg.num);
      break;
    case BHEX2:
      obj_add_hex2(objs, bc.arg.num);
      break;
    case BSTRING:
      for (unsigned int i = 0; i < bc.arg.sv.len - 1; ++i) {
        obj_add_hex(objs, bc.arg.sv.start[1 + i]);
      }
      break;
    case BSETLABEL:
      if (obj_state_find_symbol(objs, bc.arg.sv) != 0xFFFF) {
        eprintf("label redefinition " SV_FMT, SV_UNPACK(bc.arg.sv));
      }
      obj_state_add_symbol(objs, bc.arg.sv, objs->obj.code_size);
      break;
    case BGLOBAL:
      obj_add_global(objs, bc.arg.sv);
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

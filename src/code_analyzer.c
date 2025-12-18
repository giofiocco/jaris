#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "assemble.h"
#include "files.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

#define MAX_NODE_COUNT  4096
#define MAX_LABEL_COUNT 1024
#define MAX_CODE_SIZE   4096

char *stdlib_path = "asm/bin/stdlib";

#define TODO eprintf("TODO")

#define ASSERT_IS_NODE(i__)                                               \
  if (!(0 <= (i__) && (i__) < context->node_count)) {                     \
    eprintf("!((0 <= %d && %d < %d)", (i__), (i__), context->node_count); \
  }

void warning(int node, char *fmt, ...) {
  if (isatty(2)) {
    fprintf(stderr, "\e[31m");
  }
  fprintf(stderr, "WARN: at node %d: ", node);
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  if (isatty(2)) {
    fprintf(stderr, "\e[0m");
  }
}

// TODO: remove visited from the node

typedef struct {
  bytecode_t bc;
  int sp;

  int visited;
  int next;
  int branch;
} node_t;

typedef struct {
  node_t nodes[MAX_NODE_COUNT];
  int node_count;
  char labels[MAX_LABEL_COUNT][LABEL_MAX_LEN];
  int labels_node[MAX_LABEL_COUNT];
  int label_count;
  int ip_nodes[MAX_CODE_SIZE];
  int entry_points[MAX_NODE_COUNT];
  int entry_point_count;
  int visited[MAX_NODE_COUNT];
} context_t;

void context_init(context_t *context) {
  assert(context);
  memset(context->ip_nodes, -1, sizeof(context->ip_nodes));
  memset(context->labels_node, -1, sizeof(context->labels_node));
  memset(context->entry_points, -1, sizeof(context->entry_points));
}

int search_label(context_t *context, char *label) {
  assert(context);
  assert(label);

  for (int i = 0; i < context->label_count; ++i) {
    if (strcmp(context->labels[i], label) == 0) {
      assert(context->labels_node[i] != -1);
      return context->labels_node[i];
      break;
    }
  }
  return -1;
}

void path(context_t *context, int i, int sp) {
  assert(context);

  if (sp < 0) {
    warning(i, "sp < 0");
  }

  if (!(0 <= i && i < context->node_count)) {
    warning(i, "node > %d", context->node_count);
    return;
  }

  if (context->nodes[i].visited) {
    return;
  }

  node_t *node = &context->nodes[i];
  bytecode_kind_t kind = node->bc.kind;
  instruction_t inst = node->bc.inst;
  node->visited = 1;
  node->sp = sp;

  switch (kind) {
    case BNONE:
    case BDB:
    case BHEX:
    case BHEX2:
    case BSTRING:
    case BALIGN:
    case BGLOBAL:
    case BEXTERN:
      break;

    case BSETLABEL:
      node->next = i + 1;
      break;

    case BINST:
    case BINSTHEX:
    case BINSTHEX2:
    case BINSTLABEL:
    case BINSTRELLABEL:
      switch (inst) {
        case RET:
          if (node->sp != 0) {
            warning(i, "expected RET sp to be 0, found %d", node->sp);
          }
          __attribute__((fallthrough));
        case HLT:
        case JMPA:
        case JMPAR:
          break;

        case JMP:
        case JMPR:
        case JMPRZ:
        case JMPRN:
        case JMPRC:
        case JMPRNZ:
        case JMPRNN:
        case JMPRNC:
        case CALL:
        case CALLR:
        {
          int jmp = -1;
          if (kind == BINSTHEX2) {
            for (int j = 0; j < MAX_CODE_SIZE; ++j) {
              if (context->ip_nodes[j] == i) {
                jmp = j;
                break;
              }
            }
            if (inst == JMPR
                || inst == JMPRZ
                || inst == JMPRN
                || inst == JMPRC
                || inst == JMPRNZ
                || inst == JMPRNN
                || inst == JMPRNC
                || inst == CALLR) {
              int rel = jmp + 1 + (int16_t)node->bc.arg.num;
              if ((0 <= rel && rel < MAX_CODE_SIZE)) {
                jmp = context->ip_nodes[rel];
              }
            }

          } else if (kind == BINSTRELLABEL) {
            jmp = search_label(context, node->bc.arg.string);

          } else if (kind == BINSTLABEL) {
            if (inst == CALL && strcmp(node->bc.arg.string, "exit") == 0) {
              break;
            }
            jmp = search_label(context, node->bc.arg.string);

          } else {
            assert(0 && "malformed bytecode");
          }
          if (inst == JMPRZ
              || inst == JMPRN
              || inst == JMPRC
              || inst == JMPRNZ
              || inst == JMPRNN
              || inst == JMPRNC
              || inst == CALL
              || inst == CALLR) {
            node->branch = jmp;
            node->next = i + 1;
          } else {
            node->next = jmp;
          }
        } break;

        case PUSHA:
        case PUSHB:
        case DECSP:
          node->next = i + 1;
          sp += 2;
          break;

        case POPA:
        case POPB:
        case INCSP:
          node->next = i + 1;
          sp -= 2;
          break;

        case PEEKAR:
        case PUSHAR:
          if (node->sp < node->bc.arg.num) {
            warning(i, "%s %d with sp:%d", instruction_to_string(inst), node->bc.arg.num, sp);
          }
          __attribute__((fallthrough));
        case PEEKA:
        case PEEKB:
          if (node->sp < 2) {
            warning(i, "%s with sp:%d", instruction_to_string(inst), sp);
          }
          __attribute__((fallthrough));
        case NOP:
        case INCA:
        case DECA:
        case INCB:
        case RAM_AL:
        case RAM_BL:
        case RAM_A:
        case RAM_B:
        case SUM:
        case SUB:
        case SHR:
        case SHL:
        case AND:
        case CMPA:
        case CMPB:
        case A_B:
        case B_A:
        case B_AH:
        case AL_rB:
        case A_rB:
        case rB_AL:
        case rB_A:
        case A_SP:
        case SP_A:
        case A_SEC:
        case SEC_A:
        case RAM_NDX:
        case INCNDX:
        case NDX_A:
        case A_NDX:
        case MEM_A:
        case MEM_AH:
        case A_MEM:
        case _KEY_A:
        case DRW:
        case RAM_DRW:
          node->next = i + 1;
          break;
      }
  }

  if (node->next != -1) {
    path(context, node->next, sp);
  }
  if (node->branch != -1) {
    if (inst == CALL || inst == CALLR) {
      sp = 0;
    }
    path(context, node->branch, sp);
  }
}

void connect(context_t *context) {
  assert(context);

  for (int i = 0; i < context->entry_point_count; ++i) {
    path(context, context->entry_points[i], 0);
  }

  // TODO: check all the others
  // node_t *node = NULL;
  // for (int i = 0; i < context->node_count; ++i) {
  //   node = &context->nodes[i];
  //   if (!node->visited) {
  //     node->visited = 1;
  //     if (node->bc.kind == BINST
  //         || node->bc.kind == BINSTHEX
  //         || node->bc.kind == BINSTHEX2
  //         || node->bc.kind == BINSTLABEL
  //         || node->bc.kind == BINSTRELLABEL) {
  //     }
  //   }
  // }
}

/*
void path2(context_t *context, int starting_node, int sp) {
  assert(context);
  ASSERT_IS_NODE(starting_node);
  assert(sp >= 0);

  for (int i = starting_node; i < context->node_count;) {
    node_t *node = &context->nodes[i];
    if (node->visited) {
      if (node->sp != sp) {
        warning(i, "expected sp to be %d, found %d", node->sp, sp);
      }
      return;
    }
    node->visited = 1;
    node->sp = sp;

    if ((node->bc.kind == BINSTLABEL && node->bc.inst == CALL && strcmp(node->bc.arg.string, "exit") == 0)
        || node->bc.inst == HLT
        || node->bc.inst == JMPA
        || node->bc.inst == JMPAR) {
      return;

    } else if (node->bc.inst == RET) {
      if (node->sp != 0) {
        warning(i, "expected RET sp to be 0, found %d, at node %d\n", node->sp, i);
      }
      return;

    } else if (node->bc.inst == CALL && node->bc.arg.num == 0xFFFF) {

    } else if (node->bc.inst == JMP || node->bc.inst == CALL) {
      int jmp = -1;
      if (node->bc.kind == BINSTLABEL) {
        jmp = search_label(context, node->bc.arg.string);

        if (jmp == -1) {
          if (node->bc.inst == CALL) {
            node->next = i + 1;
            i++;
            continue;
          } else {
            return;
          }
        }
      } else if (node->bc.kind == BINSTHEX2) {
        assert(0 <= node->bc.arg.num && node->bc.arg.num < MAX_CODE_SIZE);
        jmp = context->ip_nodes[node->bc.arg.num];
      } else {
        printf("TODO: at %d ", __LINE__);
        bytecode_dump(node->bc);
        exit(101);
      }

      ASSERT_IS_NODE(jmp);
      if (node->bc.inst == JMP) {
        node->next = jmp;
      } else {
        node->branch = jmp;
        path(context, jmp, 0);
      }

    } else if (node->bc.inst == JMPR
               || node->bc.inst == JMPRZ
               || node->bc.inst == JMPRN
               || node->bc.inst == JMPRC
               || node->bc.inst == JMPRNZ
               || node->bc.inst == JMPRNN
               || node->bc.inst == JMPRNC
               || node->bc.inst == CALLR) {
      int jmp = -1;
      if (node->bc.kind == BINSTRELLABEL) {
        jmp = search_label(context, node->bc.arg.string);
      } else if (node->bc.kind == BINSTHEX2) {
        int nodeip = -1;
        for (int j = 0; j < MAX_CODE_SIZE; ++j) {
          if (context->ip_nodes[j] == i) {
            nodeip = j;
            break;
          }
        }
        assert(nodeip != -1);
        jmp = context->ip_nodes[nodeip + 1 + (int16_t)node->bc.arg.num];
      } else {
        printf("TODO: at %d ", __LINE__);
        bytecode_dump(node->bc);
        exit(101);
      }
      ASSERT_IS_NODE(jmp);
      if (node->bc.inst == JMPR) {
        node->next = jmp;
      } else if (node->bc.inst == CALLR) {
        node->branch = jmp;
        path(context, jmp, 0);
      } else {
        node->branch = jmp;
        path(context, jmp, sp);
      }
    }

    if (node->next < 0) {
      node->next = i + 1;
    }

    i = node->next;

    if (node->bc.inst == PUSHA || node->bc.inst == PUSHB || node->bc.inst == DECSP) {
      sp += 2;
    } else if (node->bc.inst == POPA || node->bc.inst == POPB || node->bc.inst == INCSP) {
      sp -= 2;
    }
  }
}
*/

void analyze_code(context_t *context, uint16_t code_size, uint8_t *code, uint16_t symbol_count, symbol_t *symbols, uint16_t reloc_count, reloc_entry_t *relocs) {
  assert(context);
  assert(code);

  assert(code_size < MAX_CODE_SIZE);
  if (code_size > MAX_NODE_COUNT) {
    printf("WARN: code_size > MAX_NODE_COUNT\n");
  }

  bytecode_t bc = {0};
  for (int ip = 0; ip < code_size;) {
    int node_ip = ip;
    int label_node = -1;

    for (int i = 0; i < symbol_count; ++i) {
      if (node_ip != symbols[i].pos) {
        continue;
      }

      assert(context->node_count + 1 < MAX_NODE_COUNT);
      bc = (bytecode_t){BSETLABEL, 0, {}};
      strncpy(bc.arg.string, symbols[i].image, LABEL_MAX_LEN);
      context->nodes[context->node_count++] = (node_t){
          .visited = 0,
          .sp = -1,
          .bc = bc,
          .next = -1,
          .branch = -1,
      };

      assert(context->label_count + 1 < MAX_LABEL_COUNT);
      strncpy(context->labels[context->label_count], bc.arg.string, LABEL_MAX_LEN);
      context->labels_node[context->label_count++] = context->node_count - 1;
      label_node = context->node_count - 1;
      break;
    }

    if (instruction_to_string(code[ip])) {
      switch (instruction_stat(code[ip]).arg) {
        case INST_NO_ARGS:
          bc = (bytecode_t){BINST, code[ip], {}};
          ip += 1;
          break;
        case INST_8BITS_ARG:
          bc = (bytecode_t){BINSTHEX, code[ip], {.num = code[ip + 1]}};
          ip += 2;
          break;
        case INST_16BITS_ARG:
          bc = (bytecode_t){BINSTHEX2, code[ip], {.num = code[ip + 1] + (code[ip + 2] << 8)}};
          ip += 3;
          break;
        case INST_LABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, code[ip], {.num = code[ip + 1] + (code[ip + 2] << 8)}};
          ip += 3;
          break;
        case INST_RELLABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, code[ip], {.num = code[ip + 1] + (code[ip + 2] << 8)}};
          ip += 3;
          break;
      }
    } else {
      bc = (bytecode_t){BHEX, 0, {.num = code[ip]}};
      ip += 1;
    }

    assert(context->node_count + 1 < MAX_NODE_COUNT);
    context->nodes[context->node_count++] = (node_t){
        .visited = 0,
        .sp = -1,
        .bc = bc,
        .next = -1,
        .branch = -1,
    };

    assert(context->ip_nodes[node_ip] == -1);
    context->ip_nodes[node_ip] = label_node == -1 ? context->node_count - 1 : label_node;
  }

  node_t *last_node = &context->nodes[context->node_count - 1];
  if (last_node->bc.kind == BINST && last_node->bc.inst == NOP) {
    last_node->bc = (bytecode_t){BHEX, 0, {.num = 0}};
  }

  for (int i = 0; i < symbol_count; ++i) {
    if (symbols[i].pos != 0xFFFF) {
      assert(context->label_count + 1 < MAX_LABEL_COUNT);
      strcpy(context->labels[context->label_count], symbols[i].image);
      assert(context->ip_nodes[symbols[i].pos] != -1);
      context->labels_node[context->label_count] = context->ip_nodes[symbols[i].pos];
      context->label_count++;
    }

    for (int j = 0; j < symbols[i].reloc_count; ++j) {
      if (context->ip_nodes[symbols[i].relocs[j] - 1] == -1) {
        continue;
      }
      node_t *node = &context->nodes[context->ip_nodes[symbols[i].relocs[j] - 1]];
      if (node->bc.kind == BINSTHEX2) {
        node->bc.kind = BINSTLABEL;
        strncpy(node->bc.arg.string, symbols[i].image, LABEL_MAX_LEN);
      }
    }
    for (int j = 0; j < symbols[i].relreloc_count; ++j) {
      if (context->ip_nodes[symbols[i].relrelocs[j] - 1] == -1) {
        continue;
      }
      node_t *node = &context->nodes[context->ip_nodes[symbols[i].relrelocs[j] - 1]];
      // printf("<");
      // bytecode_dump(node->bc);
      if (node->bc.kind == BINSTHEX2) {
        node->bc.kind = BINSTRELLABEL;
        strncpy(node->bc.arg.string, symbols[i].image, LABEL_MAX_LEN);
      }
    }
  }

  // for (int i = 0; i < reloc_count; ++i) {
  //   int from = context->ip_nodes[relocs[i].where - 1]; // to get the inst instead of the addr
  //   int to = context->ip_nodes[relocs[i].what];
  //   ASSERT_IS_NODE(from);
  //   ASSERT_IS_NODE(to);
  //   assert(context->nodes[from].branch == -1);
  //   if (context->nodes[from].bc.inst == JMP
  //       || context->nodes[from].bc.inst == JMPR) {
  //     context->nodes[from].next = to;
  //   } else {
  //     context->nodes[from].branch = to;
  //   }
  // }
}

void analyze_asm(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  FILE *file = fopen(filename, "r");
  if (!file) {
    error_fopen(filename);
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size + 1];
  buffer[size] = 0;
  assert((int)fread(buffer, 1, size, file) == size);
  assert(fclose(file) == 0);

  char globals[GLOBAL_MAX_COUNT][LABEL_MAX_LEN] = {0};
  int global_count = 0;

  asm_tokenizer_t tok = {0};
  asm_tokenizer_init(&tok, buffer, filename, 0);

  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    if (bc.kind == BEXTERN) {
      continue;
    } else if (bc.kind == BGLOBAL) {
      assert(global_count + 1 < GLOBAL_MAX_COUNT);
      strncpy(globals[global_count++], bc.arg.string, LABEL_MAX_LEN);
      continue;
    }

    assert(context->node_count + 1 < MAX_NODE_COUNT);
    context->nodes[context->node_count++] = (node_t){
        .visited = 0,
        .sp = -1,
        .bc = bc,
        .next = -1,
        .branch = -1,
    };

    if (bc.kind == BSETLABEL) {
      assert(context->label_count + 1 < MAX_LABEL_COUNT);
      strncpy(context->labels[context->label_count], bc.arg.string, LABEL_MAX_LEN);
      context->labels_node[context->label_count++] = context->node_count - 1;
    }
  }

  for (int i = 0; i < global_count; ++i) {
    int node = search_label(context, globals[i]);
    ASSERT_IS_NODE(node);
    context->entry_points[context->entry_point_count++] = node;
  }
}

void analyze_obj(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  obj_t obj = obj_decode_file(filename);
  analyze_code(context, obj.code_size, obj.code, obj.symbol_count, obj.symbols, obj.reloc_count, obj.relocs);

  for (int i = 0; i < obj.global_count; ++i) {
    int node = search_label(context, obj.symbols[obj.globals[i]].image);
    assert(node != -1);
    context->entry_points[context->entry_point_count++] = node;
  }
}

void analyze_exe(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  exe_t exe = exe_decode_file(filename);
  analyze_code(context, exe.code_size, exe.code, exe.symbol_count, exe.symbols, exe.reloc_count, exe.relocs);

  if (exe.dynamic_count > 0) {
    assert(exe.dynamic_count == 1);
    assert(exe.dynamics[0].file_name[0] == 1);
    assert(exe.dynamics[0].file_name[1] == 0);
    so_t stdlib = so_decode_file(stdlib_path);
    for (int i = 0; i < exe.dynamics[0].reloc_count; ++i) {
      int from = context->ip_nodes[exe.dynamics[0].relocs[i].where - 1]; // to get the inst instead of the addr
      ASSERT_IS_NODE(from);
      for (int j = 0; j < stdlib.global_count; ++j) {
        if (stdlib.symbols[stdlib.globals[j]].pos == exe.dynamics[0].relocs[i].what) {
          context->nodes[from].bc.kind = BINSTLABEL;
          strncpy(context->nodes[from].bc.arg.string, stdlib.symbols[stdlib.globals[j]].image, LABEL_MAX_LEN);
          break;
        }
      }
    }
  }

  context->entry_points[context->entry_point_count++] = 0;
}

void analyze_so(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  so_t so = so_decode_file(filename);
  analyze_code(context, so.code_size, so.code, so.symbol_count, so.symbols, so.reloc_count, so.relocs);

  //  TODO: TBD

  for (int i = 0; i < so.global_count; ++i) {
    int node = search_label(context, so.symbols[so.globals[i]].image);
    assert(node != -1);
    context->entry_points[context->entry_point_count++] = node;
  }
}

void analyze_bin(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  FILE *file = fopen(filename, "rb");
  if (!file) {
    error_fopen(filename);
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  unsigned int size = ftell(file);
  uint8_t code[size];
  assert(fseek(file, 0, SEEK_SET) == 0);
  assert(fread(code, 1, size, file) == size);
  assert(fclose(file) == 0);
  assert(size < MAX_CODE_SIZE);

  analyze_code(context, size, code, 0, NULL, 0, NULL);

  context->entry_points[context->entry_point_count++] = 0;
}

void dump_dot_path(context_t *context, FILE *file, int i) {
  assert(context);
  assert(file);
  ASSERT_IS_NODE(i);

  if (context->visited[i]) {
    return;
  }

  node_t *node = &context->nodes[i];
  context->visited[i] = 1;

  if (node->next != -1) {
    fprintf(file, "\tN%03d -> N%03d;\n", i, node->next);
    dump_dot_path(context, file, node->next);
  }
  if (node->branch != -1) {
    fprintf(file, "\tN%03d -> N%03d [color=red];\n", i, node->branch);
    dump_dot_path(context, file, node->branch);
  }
}

void dump_dot_digraph(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  FILE *file = strcmp(filename, "-") == 0 ? stdout : fopen(filename, "w");
  if (!file) {
    error_fopen(filename);
  }

  fprintf(file, "digraph {\n\tsplines = ortho;\n\tnode [shape = box];\n");

  memset(context->visited, 0, sizeof(context->visited));
  for (int i = 0; i < context->entry_point_count; ++i) {
    dump_dot_path(context, file, context->entry_points[i]);
  }

  for (int i = 0; i < context->node_count; ++i) {
    if (!context->visited[i]) {
      continue;
    }
    node_t *node = &context->nodes[i];

    fprintf(file, "\tN%03d [label=< %d: ", i, i);
    bytecode_to_asm(file, node->bc);
    fprintf(file, ">, xlabel=\"%d\"];\n", node->sp);
  }

  fprintf(file, "}");

  assert(fclose(file) == 0);
}

void print_help() {
  printf("Usage: code_analyze [options] [kinds] <input>\n\n"
         "Options:\n"
         " --stdlib-path <str>  set path of stdlib\n"
         "  --dot <path>        path of output dot file\n"
         "  --text              print the textual rappresentation of the nodes\n"
         "  -h | --help         show help message\n");
  print_file_kind_list();
  printf("kinds allowed are: asm, obj, exe, so, bin\n");
}

int main(int argc, char **argv) {
  char *input = NULL;
  char *dot_path = NULL;
  int text = 0;
  file_kind_t kind = F_NONE;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                                    //
        else ARG_PARSE_STRING_ARG_(ARG_LFLAG("dot"), dot_path)            //
        else ARG_PARSE_FLAG_(ARG_LFLAG("text"), text)                     //
        else ARG_PARSE_STRING_ARG_(ARG_LFLAG("stdlib-path"), stdlib_path) //
        else if (kind == F_NONE && (kind = parse_argument_file_kind(*argv))) {
    }
    else {
      if (input != NULL) {
        eprintf("file already provided: %s", input);
      }
      input = *argv;
    }
  }

  if (!input) {
    eprintf("input file not provided");
  }

  if (kind == F_NONE) {
    kind = file_deduce_kind(input);
  }

  context_t context = {0};
  context_init(&context);

  switch (kind) {
    case F_NONE: assert(0); break;
    case F_ASM: analyze_asm(&context, input); break;
    case F_OBJ: analyze_obj(&context, input); break;
    case F_EXE: analyze_exe(&context, input); break;
    case F_SO: analyze_so(&context, input); break;
    case F_BIN: analyze_bin(&context, input); break;
    case F_MEM:
    case F_FONT: eprintf("unvalid file kind: %s", file_kind_to_string(kind)); break;
  }

  connect(&context);

  if (text) {
    for (int i = 0; i < context.node_count; ++i) {
      printf("%d: ", i);
      bytecode_dump(context.nodes[i].bc);
      printf("  -> %d %d (sp:%d)\n", context.nodes[i].next, context.nodes[i].branch, context.nodes[i].sp);
    }
  }

  if (dot_path) {
    dump_dot_digraph(&context, dot_path);
  }

  return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "assemble.h"
#include "files.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

#define MAX_NODE_COUNT  4096
#define MAX_LABEL_COUNT 128
#define MAX_CODE_SIZE   4096

char *stdlib_path = "asm/bin/stdlib";

typedef struct {
  int visited;
  int sp;
  bytecode_t bc;

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
} context_t;

void context_init(context_t *context) {
  assert(context);
  memset(context->ip_nodes, -1, sizeof(context->ip_nodes));
  memset(context->labels_node, -1, sizeof(context->labels_node));
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

void path(context_t *context, int starting_node, int sp) {
  assert(context);
  assert(0 <= starting_node && starting_node < context->node_count);

  for (int i = starting_node; i < context->node_count;) {
    node_t *node = &context->nodes[i];
    if (node->visited) {
      if (node->sp != sp) {
        printf("ERROR: expected sp to be 0 at node %i:\t", i);
        bytecode_dump(node->bc);
      }
      return;
    }
    node->visited = 1;
    node->sp = sp;

    if (node->next >= 0) {
      assert(node->next < context->node_count);

    } else if (node->branch >= 0) {
      assert(node->branch < context->node_count);
      path(context, node->branch, sp);

    } else if ((node->bc.kind == BINSTLABEL && node->bc.inst == CALL && strcmp(node->bc.arg.string, "exit") == 0)
               || node->bc.inst == HLT
               || node->bc.inst == JMPA
               || node->bc.inst == JMPAR) {
      return;

    } else if (node->bc.inst == JMPR
               || node->bc.inst == JMP
               || node->bc.inst == JMPR
               || node->bc.inst == JMPRZ
               || node->bc.inst == JMPRN
               || node->bc.inst == JMPRC
               || node->bc.inst == JMPRNZ
               || node->bc.inst == JMPRNN
               || node->bc.inst == JMPRNC
               || node->bc.inst == CALLR) {
      int jmp = -1;
      if (node->bc.kind == BINSTLABEL) {
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
        assert(0);
      }
      assert(0 <= jmp && jmp < context->node_count);
      if (node->bc.inst == JMPR || node->bc.inst == JMP) {
        node->next = jmp;
      } else {
        node->branch = jmp;
        path(context, jmp, sp);
      }

    } else if (node->bc.inst == RET) {
      if (sp != 0) {
        fprintf(stderr, "ERROR: expected sp to be 0 for RET at node %d\n", i);
      }
      return;
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

void analyze_code(context_t *context, uint16_t code_size, uint8_t *code) {
  assert(context);
  assert(code);

  bytecode_t bc = {0};
  for (int ip = 0; ip < code_size;) {
    int node_ip = ip;

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
    context->ip_nodes[node_ip] = context->node_count - 1;
  }
}

void analyze_asm(context_t *context, char *filename) {
  assert(context);
  assert(filename);
  assert(0 && "TODO");
}

void analyze_obj(context_t *context, char *filename) {
  assert(context);
  assert(filename);
  assert(0 && "TODO");
}

void analyze_exe(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  exe_t exe = exe_decode_file(filename);
  assert(exe.code_size < MAX_CODE_SIZE);
  if (exe.code_size > MAX_NODE_COUNT) {
    printf("WARN: code_size > MAX_NODE_COUNT\n");
  }

  analyze_code(context, exe.code_size, exe.code);

  for (int i = 0; i < exe.reloc_count; ++i) {
    int from = context->ip_nodes[exe.relocs[i].where - 1]; // to get the JMP instead of the addr
    assert(from >= 0);
    int to = context->ip_nodes[exe.relocs[i].what];
    assert(to >= 0);
    assert(context->nodes[from].branch == -1);
    if (context->nodes[from].bc.inst == JMP || context->nodes[from].bc.inst == CALL) {
      context->nodes[from].next = to;
    }
  }

  if (exe.dynamic_count > 0) {
    assert(exe.dynamic_count == 1);
    assert(exe.dynamics[0].file_name[0] == 1);
    assert(exe.dynamics[0].file_name[1] == 0);
    so_t stdlib = so_decode_file(stdlib_path);
    for (int i = 0; i < exe.dynamics[0].reloc_count; ++i) {
      int from = context->ip_nodes[exe.dynamics[0].relocs[i].where - 1]; // to get the JMP instead of the addr
      assert(from >= 0);
      for (int j = 0; j < stdlib.global_count; ++j) {
        if (stdlib.symbols[stdlib.globals[j]].pos == exe.dynamics[0].relocs[i].what) {
          context->nodes[from].bc.kind = BINSTLABEL;
          strncpy(context->nodes[from].bc.arg.string, stdlib.symbols[stdlib.globals[j]].image, LABEL_MAX_LEN);
          break;
        }
      }
    }
  }

  for (int i = 0; i < exe.symbol_count; ++i) {
    assert(context->label_count + 1 < MAX_LABEL_COUNT);
    strcpy(context->labels[context->label_count], exe.symbols[i].image);
    assert(context->ip_nodes[exe.symbols[i].pos] != -1);
    context->labels_node[context->label_count] = context->ip_nodes[exe.symbols[i].pos];
    context->label_count++;

    for (int j = 0; j < exe.symbols[i].reloc_count; ++j) {
      if (context->ip_nodes[exe.symbols[i].relocs[j] - 1] == -1) {
        continue;
      }
      node_t *node = &context->nodes[context->ip_nodes[exe.symbols[i].relocs[j] - 1]];
      if (node->bc.kind == BINSTHEX2) {
        node->bc.kind = BINSTLABEL;
        strncpy(node->bc.arg.string, exe.symbols[i].image, LABEL_MAX_LEN);
      }
    }
    for (int j = 0; j < exe.symbols[i].relreloc_count; ++j) {
      if (context->ip_nodes[exe.symbols[i].relrelocs[j] - 1] == -1) {
        continue;
      }
      node_t *node = &context->nodes[context->ip_nodes[exe.symbols[i].relrelocs[j] - 1]];
      if (node->bc.kind == BINSTHEX2) {
        node->bc.kind = BINSTRELLABEL;
        strncpy(node->bc.arg.string, exe.symbols[i].image, LABEL_MAX_LEN);
      }
    }
  }

  path(context, 0, 0);
}

void analyze_so(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  so_t so = so_decode_file(filename);

  assert(so.code_size < MAX_CODE_SIZE);
  if (so.code_size > MAX_NODE_COUNT) {
    printf("WARN: code_size > MAX_NODE_COUNT\n");
  }

  bytecode_t bc = {0};
  for (int ip = 0; ip < so.code_size;) {
    int node_ip = ip;

    for (int i = 0; i < so.global_count; ++i) {
      if (node_ip == so.symbols[so.globals[i]].pos) {
        assert(context->node_count + 1 < MAX_NODE_COUNT);
        bc = (bytecode_t){BSETLABEL, 0, {}};
        strncpy(bc.arg.string, so.symbols[so.globals[i]].image, LABEL_MAX_LEN);
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
      }
    }

    if (instruction_to_string(so.code[ip])) {
      switch (instruction_stat(so.code[ip]).arg) {
        case INST_NO_ARGS:
          bc = (bytecode_t){BINST, so.code[ip], {}};
          ip += 1;
          break;
        case INST_8BITS_ARG:
          bc = (bytecode_t){BINSTHEX, so.code[ip], {.num = so.code[ip + 1]}};
          ip += 2;
          break;
        case INST_16BITS_ARG:
          bc = (bytecode_t){BINSTHEX2, so.code[ip], {.num = so.code[ip + 1] + (so.code[ip + 2] << 8)}};
          ip += 3;
          break;
        case INST_LABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, so.code[ip], {.num = so.code[ip + 1] + (so.code[ip + 2] << 8)}};
          ip += 3;
          break;
        case INST_RELLABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, so.code[ip], {.num = so.code[ip + 1] + (so.code[ip + 2] << 8)}};
          ip += 3;
          break;
      }
    } else {
      bc = (bytecode_t){BHEX, 0, {.num = so.code[ip]}};
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
    context->ip_nodes[node_ip] = context->node_count - 1;
  }

  for (int i = 0; i < so.global_count; ++i) {
    int node = search_label(context, so.symbols[so.globals[i]].image);
    assert(node != -1);
    path(context, node, 0);
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

  analyze_code(context, size, code);

  path(context, 0, 0);
}

void dump_dot_digraph(context_t *context, char *filename) {
  assert(context);
  assert(filename);

  FILE *file = fopen(filename, "w");
  if (!file) {
    error_fopen(filename);
  }

  fprintf(file, "digraph {\n\tnode [shape = box];\n");

  for (int i = 0; i < context->node_count; ++i) {
    node_t *node = &context->nodes[i];

    if (node->bc.kind == BNONE || node->bc.kind == BEXTERN || node->bc.kind == BGLOBAL) {
      continue;
    }
    if (!node->visited) {
      continue;
    }

    fprintf(file, "\tN%03d [label=", i);
    fprintf(file, "< ");
    bytecode_to_asm(file, node->bc);
    fprintf(file, ">");
    if (!node->visited) {
      fprintf(file, ", color=red");
    }
    if (node->sp != -1) {
      fprintf(file, ", xlabel=\"%d\"", node->sp);
    }
    fprintf(file, "];\n");
  }

  for (int i = 0; i < context->node_count; ++i) {
    node_t *node = &context->nodes[i];

    if (node->bc.kind == BNONE || node->bc.kind == BEXTERN || node->bc.kind == BGLOBAL) {
      continue;
    }

    if (node->next != -1) {
      fprintf(file, "\tN%03d -> N%03d;\n", i, node->next);
    }
    if (node->branch != -1) {
      fprintf(file, "\tN%03d -> N%03d [color=red];\n", i, node->branch);
    }
  }

  fprintf(file, "}");

  assert(fclose(file) == 0);
}

void print_help() {
  printf("Usage: inspect [kind] [options] <input>\n\n"
         "Options:\n"
         "  -d                  print the disassembled code\n"
         " --stdlib-path <str>  set path of stdlib\n"
         "  --dot <path>        path of output dot file\n"
         "  --text              print the textual rappresentation of the nodes\n"
         "  -h | --help         show help message\n");
  print_file_kind_list();
  printf("kinds allowed are: asm, obj, exe, so\n");
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

  if (text) {
    for (int i = 0; i < context.node_count; ++i) {
      printf("%d: ", i);
      bytecode_dump(context.nodes[i].bc);
      printf("  -> %d %d\n", context.nodes[i].next, context.nodes[i].branch);
    }
  }

  if (dot_path) {
    dump_dot_digraph(&context, dot_path);
  }

  return 0;
}

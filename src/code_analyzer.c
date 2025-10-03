#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "assemble.h"
#include "files.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

#define STDLIB_PATH "asm/bin/stdlib"

typedef struct {
  int visited;
  int sp;
  bytecode_t bc;
  int ip;

  int next;
  int branch;
} node_t;

#define MAX_NODE_COUNT  1024
#define MAX_LABEL_COUNT 128
typedef struct {
  node_t nodes[MAX_NODE_COUNT];
  int node_count;
  char labels[MAX_LABEL_COUNT][LABEL_MAX_LEN];
  int labels_node[MAX_LABEL_COUNT];
  int label_count;
} context_t;

int search_label(context_t *context, char *label) {
  assert(context);
  assert(label);

  for (int i = 0; i < context->label_count; ++i) {
    if (strcmp(context->labels[i], label) == 0) {
      assert(context->labels_node[i]);
      return context->labels_node[i];
      break;
    }
  }
  return -1;
}

int search_ip(context_t *context, int ip) {
  assert(context);
  assert(ip >= 0);

  for (int j = 0; j < context->node_count && context->nodes[j].ip <= ip; ++j) {
    if (context->nodes[j].ip == ip) {
      return j;
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
        fprintf(stderr, "ERROR: for '");
        bytecode_to_asm(stderr, node->bc);
        fprintf(stderr, "' expected sp %d, found %d\n", node->sp, sp);
      }
      return;
    }
    node->visited = 1;
    node->sp = sp;

    if (node->next >= 0) {
      assert(node->next < context->node_count);

    } else if ((node->bc.kind == BINSTLABEL && node->bc.inst == CALL && strcmp(node->bc.arg.string, "exit") == 0)
               || node->bc.inst == HLT) {
      return;

    } else if ((node->bc.inst == JMPR
                || node->bc.inst == JMPRZ
                || node->bc.inst == JMPRN
                || node->bc.inst == JMPRC
                || node->bc.inst == JMPRNZ
                || node->bc.inst == JMPRNN
                || node->bc.inst == JMPRNC)
               && node->bc.kind == BINSTHEX2) {
      int jmp = search_ip(context, node->ip + 1 + (int16_t)node->bc.arg.num);
      if (0 <= jmp && jmp < context->node_count) {
        if (node->bc.inst == JMPR) {
          node->next = jmp;
          i = jmp;
          continue;
        } else {
          node->branch = jmp;
          path(context, jmp, sp);
          node->next = i + 1;
        }
      }
    }

    if (node->next == -1) {
      node->next = i + 1;
    }

    i = node->next;
  }

  // for (int i = starting_node; i < context->node_count;) {
  //   node_t *node = &context->nodes[i];
  //   if (node->visited) {
  //     if (node->sp != sp) {
  //       fprintf(stderr, "ERROR: for '");
  //       bytecode_to_asm(stderr, node->bc);
  //       fprintf(stderr, "' expected sp %d, found %d\n", node->sp, sp);
  //       // exit(1);
  //     }
  //
  //     return;
  //   }
  //   node->visited = 1;
  //   node->sp = sp;
  //
  //
  //   if ((node->bc.kind == BINSTLABEL && node->bc.inst == CALL && strcmp(node->bc.arg.string, "exit") == 0)
  //       || (node->bc.kind == BINST && node->bc.inst == HLT)) {
  //     return;
  //   } else if (node->bc.kind == BINST && (node->bc.inst == JMPA || node->bc.inst == JMPAR)) {
  //     return;
  //   } else if (node->bc.kind == BINST && node->bc.inst == RET) {
  //     if (sp != 0) {
  //       eprintf("expected sp to be 0 for RET\n");
  //     }
  //     return;
  //
  //   } else if ((node->bc.kind == BINSTLABEL || node->bc.kind == BINSTRELLABEL) && (node->bc.inst == JMP || node->bc.inst == JMPR)) {
  //     int n = search_label(context, node->bc.arg.string);
  //     assert(n != -1);
  //     node->next = n;
  //     i = n;
  //   } else if (node->bc.kind == BINSTRELLABEL
  //              && (node->bc.inst == JMPRZ
  //                  || node->bc.inst == JMPRN
  //                  || node->bc.inst == JMPRC
  //                  || node->bc.inst == JMPRNZ
  //                  || node->bc.inst == JMPRNN
  //                  || node->bc.inst == JMPRNC)) {
  //     int n = search_label(context, node->bc.arg.string);
  //     assert(n != -1);
  //     node->branch = n;
  //     path(context, n, sp);
  //     node->next = i + 1;
  //     i += 1;
  //   } else if ((node->bc.kind == BINSTLABEL && node->bc.inst == CALL)
  //              || (node->bc.kind == BINSTRELLABEL && node->bc.inst == CALLR)) {
  //     // TODO: CALLR doesnt work in asm/string.asm?
  //     int n = search_label(context, node->bc.arg.string);
  //     if (n != -1) {
  //       path(context, n, 0);
  //     }
  //     node->next = i + 1;
  //     i += 1;
  //     //} else if (node->bc.kind == BINSTHEX2 && node->bc.inst == JMP) {
  //     //  bytecode_dump(node->bc);
  //     //  exit(1);
  //     //  for (int j = 0; j < context->node_count && context->nodes[j].ip <= node->bc.arg.num; ++j) {
  //     //    if (context->nodes[j].ip == node->bc.arg.num) {
  //
  //     //    }
  //     //  }
  //
  //   } else {
  //     node->next = i + 1;
  //     i += 1;
  //   }
  //
  //   if (node->bc.kind == BINST && (node->bc.inst == PUSHA || node->bc.inst == PUSHB || node->bc.inst == DECSP)) {
  //     sp += 2;
  //   } else if (node->bc.kind == BINST && (node->bc.inst == POPA || node->bc.inst == POPB || node->bc.inst == INCSP)) {
  //     sp -= 2;
  //   }
  //
  //   if (node->bc.kind == BINSTLABEL) {
  //     int n = search_label(context, node->bc.arg.string);
  //     if (n == -1) {
  //       continue;
  //     }
  //     if (context->nodes[n + 1].bc.kind == BDB) {
  //       context->nodes[n].next = n + 1;
  //       context->nodes[n].visited = 1;
  //       context->nodes[n + 1].visited = 1;
  //
  //     } else if (context->nodes[n + 1].bc.kind == BSTRING) {
  //       context->nodes[n].visited = 1;
  //       for (int j = n + 1; !(context->nodes[j - 1].bc.kind == BHEX && context->nodes[j - 1].bc.arg.num == 0); ++j) {
  //         context->nodes[j - 1].next = j;
  //         context->nodes[j].visited = 1;
  //       }
  //     }
  //   }
  // }
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
      // fprintf(file, "\tN%03d -> N%03d [label=\"branch\"];\n", i, node->branch);
      fprintf(file, "\tN%03d -> N%03d [color=red];\n", i, node->branch);
    }
  }

  fprintf(file, "}");

  assert(fclose(file) == 0);
}

void analyze_exe(context_t *context, exe_t *exe) {
  assert(context);
  assert(exe);

  bytecode_t bc = {0};
  for (int ip = 0; ip < exe->code_size; ++ip) {
    int node_ip = ip;

    if (instruction_to_string(exe->code[ip])) {
      switch (instruction_stat(exe->code[ip]).arg) {
        case INST_NO_ARGS:
          bc = (bytecode_t){BINST, exe->code[ip], {}};
          break;
        case INST_8BITS_ARG:
          bc = (bytecode_t){BINSTHEX, exe->code[ip], {.num = exe->code[ip + 1]}};
          ip += 1;
          break;
        case INST_16BITS_ARG:
          bc = (bytecode_t){BINSTHEX2, exe->code[ip], {.num = exe->code[ip + 1] + (exe->code[ip + 2] << 8)}};
          ip += 2;
          break;
        case INST_LABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, exe->code[ip], {.num = exe->code[ip + 1] + (exe->code[ip + 2] << 8)}};
          ip += 2;
          break;
        case INST_RELLABEL_ARG:
          bc = (bytecode_t){BINSTHEX2, exe->code[ip], {.num = exe->code[ip + 1] + (exe->code[ip + 2] << 8)}};
          ip += 2;
          break;
      }
    } else {
      bc = (bytecode_t){BHEX, 0, {.num = exe->code[ip]}};
    }

    assert(context->node_count + 1 < MAX_NODE_COUNT);
    context->nodes[context->node_count++] = (node_t){
        .visited = 0,
        .sp = -1,
        .bc = bc,
        .ip = node_ip,
        .next = -1,
        .branch = -1,
    };
  }

  for (int i = 0; i < exe->reloc_count; ++i) {
    int from = search_ip(context, exe->relocs[i].where - 1); // to get the JMP instead of the addr
    assert(from >= 0);
    int to = search_ip(context, exe->relocs[i].what);
    assert(to >= 0);
    assert(context->nodes[from].branch == -1);
    if (context->nodes[from].bc.inst == JMP || context->nodes[from].bc.inst == CALL) {
      context->nodes[from].next = to;
    }
  }

  if (exe->dynamic_count > 0) {
    assert(exe->dynamic_count == 1);
    assert(exe->dynamics[0].file_name[0] == 1);
    assert(exe->dynamics[0].file_name[1] == 0);
    so_t stdlib = so_decode_file(STDLIB_PATH);
    for (int i = 0; i < exe->dynamics[0].reloc_count; ++i) {
      int from = search_ip(context, exe->dynamics[0].relocs[i].where - 1); // to get the JMP instead of the addr
      assert(from >= 0);
      for (int j = 0; j < stdlib.global_count; ++j) {
        if (stdlib.symbols[stdlib.globals[j]].pos == exe->dynamics[0].relocs[i].what) {
          assert(context->nodes[from].bc.inst == CALL);
          context->nodes[from].bc.kind = BINSTLABEL;
          strncpy(context->nodes[from].bc.arg.string, stdlib.symbols[stdlib.globals[j]].image, LABEL_MAX_LEN);
          break;
        }
      }
    }
  }

  // for (int i = 0; i < exe->dynamic_count; ++i) {
  //   for (int j = 0; j < exe->dynamics[i].reloc_count; ++j) {
  //   }
  // }

  // for (int i = 0; i < exe->symbol_count; ++i) {
  //   assert(context->label_count + 1 < MAX_LABEL_COUNT);
  //   strcpy(context->labels[context->label_count], exe->symbols[i].image);
  //   int label_node = search_ip(context, exe->symbols[i].pos);
  //   // assert(label_node >= 0);
  //   context->labels_node[context->label_count] = label_node;
  //   context->label_count++;
  // }

  path(context, 0, 0);
}

void print_help() {
  printf("Usage: inspect [kind] [options] <input>\n\n"
         "Options:\n"
         "  -d            print the disassembled code\n"
         "  -h | --help   show help message\n"
         "  --dot <path>  path of output dot file\n"
         "  -a <str>      allow special cases that usually are considered mistakes, <str> can be:\n"
         "                  inst-as-arg    allow insts mnemonics as argument of 8bit inst\n"
         "\nKinds:\n"
         "if the kind is not specified it's deduced from the file extension\n\n"
         "  --obj   analyse the input as an obj\n"
         "  --exe   analyse the input as an exe\n"
         "  --so    analyse the input as a so\n"
         "  --mem   analyse the input as a memory bin\n"
         "  --bin   analyse the input as a bin (just plain code)\n"
         "  --font  analyse the input as a font\n");
}

int main(int argc, char **argv) {
  char *input = NULL;
  char *dot_path = NULL;
  int allowed = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG
    else if (ARG_SFLAG("a")) {
      if (*(argv + 1) == NULL) {
        ARG_ERROR("expected string: '%s'", *argv);
      }
      ++argv;
      if (strcmp(*argv, "inst-as-arg") == 0) {
        allowed |= 1 << ASM_ALLOWED_INST_AS_ARG;
      } else {
        ARG_ERROR_("unexpected arg for '-a'");
      }
    }
    else ARG_PARSE_STRING_ARG_(ARG_LFLAG("dot"), dot_path) //
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

  context_t context = {0};

  exe_t exe = exe_decode_file(input);
  analyze_exe(&context, &exe);

  for (int i = 0; i < context.node_count; ++i) {
    printf("%d: ", i);
    bytecode_dump(context.nodes[i].bc);
    printf("  -> %d %d ip:%d\n", context.nodes[i].next, context.nodes[i].branch, context.nodes[i].ip);
  }

  if (dot_path) {
    dump_dot_digraph(&context, dot_path);
  }

  /*
  FILE *file = fopen(input, "r");
  assert(file);
  fseek(file, 0, SEEK_END);
  unsigned int size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char buffer[size + 1];
  assert(fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  asm_tokenizer_t tok = {0};
  asm_tokenizer_init(&tok, buffer, input, allowed);


  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    assert(context.node_count + 1 < MAX_NODE_COUNT);
    context.nodes[context.node_count++] = (node_t){
        .visited = 0,
        .sp = -1,
        .bc = bc,
        .next = -1,
        .branch = -1};

    if (bc.kind == BSETLABEL) {
      assert(context.label_count + 1 < MAX_LABEL_COUNT);
      strncpy(context.labels[context.label_count], bc.arg.string, LABEL_MAX_LEN);
      context.labels_node[context.label_count] = context.node_count - 1;
      context.label_count++;
    }
  }

  for (int i = 0; context.nodes[i].bc.kind == BGLOBAL; ++i) {
    path(&context, search_label(&context, context.nodes[i].bc.arg.string), 0);
  }
  */

  return 0;
}

#include <assert.h>
#include <stdio.h>

#include "assemble.h"
#include "files.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

typedef struct node_t_ {
  bytecode_t arg;
  struct node_t_ *next;
  struct node_t_ *branch;
} node_t;

node_t *node_malloc(bytecode_t bc) {
  node_t *node = malloc(sizeof(node_t));
  assert(node);
  node->arg = bc;
  node->next = NULL;
  return node;
}

void print_help() {
  printf("Usage: inspect [kind] [options] <input>\n\n"
         "Options:\n"
         "  -d           print the disassembled code\n"
         "  -h | --help  show help message\n"
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

  ARG_PARSE {
    ARG_PARSE_HELP_ARG
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
  asm_tokenizer_init(&tok, buffer, "asm/math.asm", 0);

  node_t *graph = NULL;
  node_t *node = NULL;

  char labels[128][LABEL_MAX_LEN] = {0};
  node_t *labels_node[128] = {0};
  int label_count = 0;

  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    if (graph == NULL) {
      graph = node_malloc(bc);
      node = graph;
    } else {
      node->next = node_malloc(bc);
      node = node->next;
    }

    if (node->arg.kind == BSETLABEL) {
      assert(label_count + 1 < 128);
      strcpy(labels[label_count], node->arg.arg.string);
      labels_node[label_count] = node;
      label_count++;

    } else if ((node->arg.kind == BINSTLABEL
                || node->arg.kind == BINSTRELLABEL)
               && (node->arg.inst == JMP
                   || node->arg.inst == JMPR
                   || node->arg.inst == JMPRZ
                   || node->arg.inst == JMPRN
                   || node->arg.inst == JMPRC
                   || node->arg.inst == JMPRNZ
                   || node->arg.inst == JMPRNN
                   || node->arg.inst == JMPRNC)) {
      for (int i = 0; i < label_count; ++i) {
        if (strcmp(labels[i], node->arg.arg.string) == 0) {
          assert(labels_node[i]);
          node->branch = labels_node[i];
          break;
        }
      }
    }
  }

  printf("digraph {\n\tnode [shape = box];\n");

  int id = 0;
  for (node_t *node = graph; node; node = node->next) {
    if (node->arg.kind == BEXTERN || node->arg.kind == BGLOBAL) {
      continue;
    }
    printf("\tN%03d node [label=\"", id++);
    bytecode_to_asm(stdout, node->arg);
    printf("\"];\n");
  }

  for (int i = 1; i < id; ++i) {
    printf("\tN%03d -> N%03d;\n", i - 1, i);
  }

  printf("}");

  return 0;
}

#include <assert.h>
#include <stdio.h>

#include "assemble.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

typedef struct node_t_ {
  int index;
  int sp;
  bytecode_t bc;

  struct node_t_ *next;
  struct node_t_ *branch;
} node_t;

node_t *node_malloc(bytecode_t bc) {
  node_t *node = malloc(sizeof(node_t));
  assert(node);
  *node = (node_t){-1, -1, bc, NULL, NULL};
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
  asm_tokenizer_init(&tok, buffer, input, 0);

  node_t nodes[1028] = {0};
  int node_count = 0;

  char labels[128][LABEL_MAX_LEN] = {0};
  int labels_node[128] = {0};
  int label_count = 0;

  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    nodes[node_count++] = (node_t){-1, -1, bc, NULL, NULL};

    if (bc.kind == BSETLABEL) {
      assert(label_count + 1 < 128);
      strcpy(labels[label_count], bc.arg.string);
      labels_node[label_count] = node_count - 1;
      label_count++;
    }
  }

  for (int i = 0; i < node_count; ++i) {
    node_t *node = &nodes[i];

    if ((node->bc.kind == BINSTLABEL
         || node->bc.kind == BINSTRELLABEL)
        && (node->bc.inst == JMP
            || node->bc.inst == JMPR
            || node->bc.inst == JMPRZ
            || node->bc.inst == JMPRN
            || node->bc.inst == JMPRC
            || node->bc.inst == JMPRNZ
            || node->bc.inst == JMPRNN
            || node->bc.inst == JMPRNC)) {
      for (int i = 0; i < label_count; ++i) {
        if (strcmp(labels[i], node->bc.arg.string) == 0) {
          assert(labels_node[i]);
          node->branch = &nodes[labels_node[i]];
          break;
        }
      }
    }
  }

  printf("digraph {\n\tnode [shape = box];\n");

  for (int i = 0; i < node_count; ++i) {
    node_t *node = &nodes[i];

    if (node->bc.kind == BNONE || node->bc.kind == BEXTERN || node->bc.kind == BGLOBAL) {
      continue;
    }
    node->index = i;
    printf("\tN%03d node [label=", i);
    printf("< ");
    bytecode_to_asm(stdout, node->bc);
    printf(">");
    printf("];\n");
  }

  for (int i = 0; i < node_count; ++i) {
    node_t *node = &nodes[i];

    if (node->next) {
      printf("\tN%03d -> N%03d;\n", node->index, node->next->index);
    }
    if (node->branch) {
      printf("\tN%03d -> N%03d;\n", node->index + 1, node->branch->index + 1);
    }
  }

  printf("}");

  return 0;
}

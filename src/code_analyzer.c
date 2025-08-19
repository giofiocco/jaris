#include <assert.h>
#include <stdio.h>

#include "assemble.h"
#include "instructions.h"

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

typedef struct {
  int sp;
  bytecode_t bc;

  int next;
  int branch;
} node_t;

#define MAX_NODE_COUNT  1028
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
  assert(0 && "label not found");
}

void path(context_t *context, int starting_node) {
  assert(context);

  for (int i = starting_node; context->nodes[i].next == -1 && i < context->node_count;) {
    node_t *node = &context->nodes[i];

    if (node->bc.kind == BINSTLABEL && node->bc.inst == CALL && strcmp(node->bc.arg.string, "exit") == 0) {
      return;
    } else if ((node->bc.kind == BINSTLABEL || node->bc.kind == BINSTRELLABEL) && (node->bc.inst == JMP || node->bc.inst == JMPR)) {
      int n = search_label(context, node->bc.arg.string);
      node->next = n;
      i = n;
    } else if (node->bc.kind == BINSTRELLABEL
               && (node->bc.inst == JMPRZ
                   || node->bc.inst == JMPRN
                   || node->bc.inst == JMPRC
                   || node->bc.inst == JMPRNZ
                   || node->bc.inst == JMPRNN
                   || node->bc.inst == JMPRNC)) {
      int n = search_label(context, node->bc.arg.string);
      node->branch = n;
      path(context, n);
      node->next = i + 1;
      i += 1;
    } else {
      node->next = i + 1;
      i += 1;
    }
  }
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

  context_t context = {0};

  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    assert(context.node_count + 1 < MAX_NODE_COUNT);
    context.nodes[context.node_count++] = (node_t){-1, bc, -1, -1};

    if (bc.kind == BSETLABEL) {
      assert(context.label_count + 1 < MAX_LABEL_COUNT);
      strncpy(context.labels[context.label_count], bc.arg.string, LABEL_MAX_LEN);
      context.labels_node[context.label_count] = context.node_count - 1;
      context.label_count++;
    }
  }

  path(&context, 0);

  printf("digraph {\n\tnode [shape = box];\n");

  for (int i = 0; i < context.node_count; ++i) {
    node_t *node = &context.nodes[i];

    if (node->bc.kind == BNONE || node->bc.kind == BEXTERN || node->bc.kind == BGLOBAL) {
      continue;
    }
    printf("\tN%03d [label=", i);
    printf("< ");
    bytecode_to_asm(stdout, node->bc);
    printf(">");
    printf("];\n");
  }

  for (int i = 0; i < context.node_count; ++i) {
    node_t *node = &context.nodes[i];

    if (node->bc.kind == BNONE || node->bc.kind == BEXTERN || node->bc.kind == BGLOBAL) {
      continue;
    }

    if (node->next != -1) {
      printf("\tN%03d -> N%03d;\n", i, node->next);
    }
    if (node->branch != -1) {
      printf("\tN%03d -> N%03d [label=\"branch\"];\n", i, node->branch);
    }
  }

  printf("}");

  return 0;
}

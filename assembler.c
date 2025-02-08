#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse/argparse.h"
#include "assemble.h"
#include "mystb/errors.h"

int main(int argc, char **argv) {
  int debug = 0;
  char *output = NULL;
  int debug_info = 0;

  struct argparse_option options[] = {
      OPT_GROUP("Options"),
      OPT_HELP(),
      OPT_STRING('o', "output", &output, "output file name", NULL, 0, 0),
      OPT_BOOLEAN('g', "debug", &debug_info, "include debug info", NULL, 0, 0),
      OPT_BIT(0, "dtok", &debug, "tokenizer debug info", NULL, DEBUG_TOKENIZER, 0),
      OPT_BIT(0, "dbyt", &debug, "bytecodes debug info", NULL, DEBUG_BYTECODES, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                    "assembler [options] file",
                    NULL,
                },
                0);
  argc = argparse_parse(&argparse, argc, (const char **)argv);
  if (argc != 1) {
    fprintf(stderr, "ERROR: expected ONE file\n");
    argparse_usage(&argparse);
    exit(1);
  }

  char *filename = argv[0];

  FILE *file = fopen(filename, "r");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(size >= 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size];
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  obj_t obj = assemble(buffer, filename, debug, debug_info);

  int output_to_free = output == NULL;
  if (output == NULL) {
    int len = strlen(filename);
    int start = len;
    while (start > 0 && filename[start] != '/') {
      --start;
    }
    ++start;
    while (len > 0 && filename[len] != '.') {
      --len;
    }
    if (len == 0) {
      len = strlen(filename);
    }
    sv_t name = {filename + start, len - start};
    output = malloc(len + 3);
    snprintf(output, len + 3, SV_FMT ".o", SV_UNPACK(name));
  }

  obj_encode_file(&obj, output);

  if (output_to_free) {
    free(output);
  }

  return 0;
}

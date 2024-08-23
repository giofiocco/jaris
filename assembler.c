#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"
#include "mystb/arg_parser.h"
#include "mystb/errors.h"

int main(int argc, char **argv) {
  int debug = 0;
  char *output = NULL;

  arg_parser_t parser = {0};
  parser.program_name = "assembler";
  arg_parser_allow_unflag(&parser, "file");
  arg_parser_add_arg(&parser, (arg_t){ARG_BOOL, NULL, 'd', "enable debug info", &debug});
  arg_parser_add_arg(&parser, (arg_t){ARG_STRING, NULL, 'o', "output file name", &output});
  arg_parser_parse(&parser, argc, argv);

  if (parser.unflagi > 1) {
    fprintf(stderr, "ERROR: too many files\n");
    arg_parser_print_help(&parser);
    exit(1);
  }
  if (parser.unflagi < 1) {
    fprintf(stderr, "ERROR: too few files\n");
    arg_parser_print_help(&parser);
    exit(1);
  }

  char *filename = parser.unflag[0];

  FILE *file = fopen(filename, "r");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(size > 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size];
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  obj_t obj = assemble(buffer, parser.unflag[0], debug);

  int output_to_free = output == NULL;
  if (output == NULL) {
    int len = strlen(filename);
    while (len > 0 && filename[len] != '.') {
      --len;
    }
    if (len == 0) {
      len = strlen(filename);
    }
    sv_t name = {filename, len};
    output = malloc(len + 3);
    snprintf(output, len + 3, SV_FMT ".o", SV_UNPACK(name));
  }

  obj_encode_file(&obj, output);

  if (output_to_free) {
    free(output);
  }

  return 0;
}

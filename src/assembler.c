#include <assert.h>
#include <errno.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION

#include "../argparse.h"
#include "../files.h"
#include "../mystb/errors.h"

extern obj_t assemble(char *buffer, char *buffer_name, int debug_info, int debug_tok, int debug_byt, int debug_obj);

void print_help() {
  printf("Usage: assembler [options] <file>\n\n"
         "Options:\n"
         " -o <str>   output name\n"
         " -g         include debug info in obj\n"
         " -d <str>   print debug info of:\n"
         "              tok      tokens\n"
         "              byt      bytecodes\n"
         "              obj      obj produced\n"
         "              all      all the above\n");
}

int main(int argc, char **argv) {
  char *output = NULL;
  char *path = NULL;
  int debug_tok = 0;
  int debug_byt = 0;
  int debug_obj = 0;
  int debug_info = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                     //
        else ARG_PARSE_STRING_ARG_(ARG_SFLAG("o"), output) //
        else ARG_PARSE_FLAG_(ARG_SFLAG("g"), debug_info)   //
        else if (ARG_SFLAG("d")) {
      printf("current:'%s'\n", *argv);
      if (*(argv + 1) == NULL) {
        ARG_ERROR("expected string: '%s'", *argv);
      }
      ++argv;
      if (strcmp(*argv, "tok") == 0) {
        debug_tok = 1;
      } else if (strcmp(*argv, "byt") == 0) {
        debug_byt = 1;
      } else if (strcmp(*argv, "obj") == 0) {
        debug_obj = 1;
      } else if (strcmp(*argv, "all") == 0) {
        debug_tok = 1;
        debug_byt = 1;
        debug_obj = 1;
      } else {
        ARG_ERROR_("unexpected arg from '-d'");
      }
    }
    else {
      if (path != NULL) {
        eprintf("file already provided: %s", path);
      }
      path = *argv;
    }
  }

  if (path == NULL) {
    eprintf("no source file provided");
  }

  FILE *file = fopen(path, "r");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", path, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(size >= 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size];
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  obj_t obj = assemble(buffer, path, debug_info, debug_tok, debug_byt, debug_obj);

  if (output == NULL) {
    int len = strlen(path);
    output = malloc(len + 3);
    strcpy(output, path);
    output[len] = '.';
    output[len + 1] = 'o';
    output[len + 2] = 0;
  }

  obj_encode_file(&obj, output);

  return 0;
}

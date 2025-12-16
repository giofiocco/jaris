#include <assert.h>
#include <errno.h>

#include "../argparse.h"
#include "assemble.h"
#include "files.h"
#define ERRORS_IMPLEMENTATION
#include "../mystb/errors.h"

void print_help() {
  printf("Usage: assembler [options] <file>\n\n"
         "Options:\n"
         "  -o <str>     output name\n"
         "  -g           include debug info in obj\n"
         "  -d <str>     print debug info of:\n"
         "                 tok    tokens\n"
         "                 byt    bytecodes\n"
         "                 obj    obj produced\n"
         "                 all    all the above\n"
         "  -a <str>     allow special cases that usually are considered mistakes, <str> can be:\n"
         "                 inst-as-arg    allow insts mnemonics as argument of 8bit inst\n"
         "  -h | --help  show help message\n");
}

int main(int argc, char **argv) {
  char *output = NULL;
  char *path = NULL;
  int debug_info = 0;
  int debug_flags = 0;
  int allowed = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                     //
        else ARG_PARSE_STRING_ARG_(ARG_SFLAG("o"), output) //
        else ARG_PARSE_FLAG_(ARG_SFLAG("g"), debug_info)   //
        else if (ARG_SFLAG("d")) {
      if (*(argv + 1) == NULL) {
        ARG_ERROR("expected string: '%s'", *argv);
      }
      ++argv;
      if (strcmp(*argv, "tok") == 0) {
        debug_flags |= 1 << ASM_DEBUG_TOK;
      } else if (strcmp(*argv, "byt") == 0) {
        debug_flags |= 1 << ASM_DEBUG_BYT;
      } else if (strcmp(*argv, "obj") == 0) {
        debug_flags |= 1 << ASM_DEBUG_OBJ;
      } else if (strcmp(*argv, "all") == 0) {
        debug_flags = (1 << ASM_DEBUG_MAX) - 1;
      } else {
        ARG_ERROR_("unexpected arg for '-d'");
      }
    }
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
  char buffer[size + 1];
  buffer[size] = 0;
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  obj_t obj = assemble(buffer, path, debug_info, debug_flags, allowed);

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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "argparse/argparse.h"
#include "files.h"
#include "instructions.h"
#include "link.h"

int main(int argc, char **argv) {
  int flags = 0;
  char *output = "a.out";

  struct argparse_option options[] = {
    OPT_GROUP("Options"),
    OPT_HELP(),
    OPT_STRING('o', "output", &output, "output file name", NULL, 0, 0),
    OPT_BIT(0, "bin", &flags, "output file with only code (no header)", NULL, LINK_FLAG_BIN, 0),
    OPT_BIT(0, "dexe", &flags, "exe state debug info", NULL, LINK_FLAG_EXE_STATE, 0),
    OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                  "linker [options] files ...",
                  NULL,
                },
                0);
  argc = argparse_parse(&argparse, argc, (const char **)argv);
  if (argc == 0) {
    fprintf(stderr, "ERROR: expected files\n");
    argparse_usage(&argparse);
    exit(1);
  }

  sv_allocator_t alloc = {0};

  obj_t objs[argc];

  for (int i = 0; i < argc; ++i) {
    objs[i] = obj_decode_file(argv[i], &alloc);
  }

  exe_t exe = link(objs, argc, flags);
  exe_encode_file(&exe, output);

  return 0;
}

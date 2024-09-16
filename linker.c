#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "argparse/argparse.h"
#include "files.h"
#include "link.h"

int main(int argc, char **argv) {
  int flags = 0;
  char *output = "a.out";

  struct argparse_option options[] = {
    OPT_GROUP("Options"),
    OPT_HELP(),
    OPT_STRING('o', "output", &output, "output file name", NULL, 0, 0),
    OPT_BIT(0, "bin", &flags, "output bin file", NULL, LINK_FLAG_BIN, 0),
    OPT_BIT(0, "so", &flags, "output so file", NULL, LINK_FLAG_SO, 0),
    OPT_BIT('d', "debug", &flags, "debug info", NULL, LINK_FLAG_EXE_STATE, 0),
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

  obj_t objs[argc];

  for (int i = 0; i < argc; ++i) {
    objs[i] = obj_decode_file(argv[i]);
  }

  exe_state_t exes = link(objs, argc, flags);
  if (flags & LINK_FLAG_SO) {
    so_encode_file(&exes, output);
  } else if (flags & LINK_FLAG_BIN) {
    bin_encode_file(&exes.exe, output);
  } else {
    exe_encode_file(&exes.exe, output);
  }

  return 0;
}

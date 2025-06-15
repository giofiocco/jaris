#include "../argparse.h"
#include "../files.h"
#include "../link.h"

void print_help() {
  printf("Usage: linker [options] -o <output> <files> ...\n\n"
         "Options:\n"
         " -o <str>              output name\n"
         " -g                    include debug info in exe\n"
         " -l <file>             link dynamically \n"
         " --stdlib-path <str>   set path of stdlib\n"
         " --nostdlib            do not link with stdlib\n"
         " --bin                 output bin file\n"
         " --so                  output so file\n"
         " -d                    print debug info of exe_state\n");
}

int main(int argc, char **argv) {
  char *output = NULL;
  int debug_info = 0;
  int debug_exe_state = 0;
  int no_stdlib = 0;
  char *stdlib_path = "../mem/__stdlib";
  int output_bin = 0;
  int output_so = 0;

  exe_state_t state = {0};

  char *filenames[argc];
  memset(filenames, 0, argc * sizeof(char *));
  int filename_count = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG
    else ARG_PARSE_STRING_ARG_(ARG_SFLAG("o"), output)                    //
        else ARG_PARSE_FLAG_(ARG_SFLAG("g"), debug_info)                  //
        else ARG_PARSE_FLAG_(ARG_SFLAG("d"), debug_exe_state)             //
        else ARG_PARSE_STRING_ARG_(ARG_LFLAG("stdlib-path"), stdlib_path) //
        else ARG_PARSE_FLAG_(ARG_LFLAG("nostdlib"), no_stdlib)            //
        else ARG_PARSE_FLAG_(ARG_LFLAG("bin"), output_bin)                //
        else ARG_PARSE_FLAG_(ARG_LFLAG("so"), output_so)                  //
        else if (ARG_SFLAG("l")) {
      ++argv;
      if (*argv == NULL) {
        ARG_ERROR_("expected string: '-l'")
      }
      so_t so = so_decode_file(*argv);
      exe_link_so(&state, &so, *argv);
    }
    else {
      filenames[filename_count++] = *argv;
    }
  }

  if (output == NULL) {
    ARG_ERROR_("expected output path")
  }

  if (!output_bin && !output_so) {
    exe_link_boilerplate(&state, debug_info);
  }

  for (int i = 0; i < filename_count; ++i) {
    obj_t obj = obj_decode_file(filenames[i]);
    exe_link_obj(&state, &obj, debug_info);
  }

  if (!no_stdlib) {
    so_t so = so_decode_file(stdlib_path);
    exe_link_so(&state, &so, "\001");
  }

  if (debug_exe_state) {
    exe_state_dump(&state);
  }

  exe_state_check_exe(&state);

  if (output_so) {
    so_t out = so_from_exe_state(&state);
    so_encode_file(&out, output);
  } else if (output_bin) {
    bin_encode_file(&state.exe, output);
  } else {
    exe_encode_file(&state.exe, output);
  }

  return 0;
}

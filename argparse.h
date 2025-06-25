/**
 *
 * A simple library to parse arguments using macros
 * to be easy to expanded and include particular cases with more advanced logic
 * keeping simple parsing more frequent ones.
 *
 * Example:
 *
 * int main(int argc, char **argv) {
 *   char *output = NULL;
 *   int wall = 0;
 *
 *   ARG_PARSE {
 *     ARG_PARSE_STRING_ARG("o", "output", output)
 *     else ARG_PARSE_FLAG_(ARG_SFLAG("Wall"), wall)
 *     else if (*argv[1] == 'a') {
 *       ...
 *     }
 *     ARG_ELSE_UNKNOWN
 *   }
 * }
 *
 * Macros:
 *
 *
 * ARG_PARSE: iterate argv untill it's NULL
 * ARG_PARSE_STRING_ARG(short, long, str):
 *   checks if *argv is "-"short or "--"long
 *   checks if the next argv is a string (and throws error if not)
 *   sets str to the string found
 * ARG_PARSE_FLAG(short, long, flag):
 *   checks if *argv is "-"short or "--"long
 *   sets flag to 1
 * ARG_PARSE_INT_ARG(short, long, var)
 *   checks if *argv is "-"short or "--"long
 *   checks if the next argv is valid (and throws error if not)
 *   sets str to the string found
 * ARG_PARSE_STRING_ARG_, ARG_PARSE_FLAG_ and ARG_PARSE_INT_ARG_:
 *   acts like the counterparts but with custom conditions
 *   (STRING_ARG and INT_ARG accepts also an arg_name parameter to display on error)
 * ARG_SFLAG(short): returns true if *argv is equal to "-"short
 * ARG_LFLAG(long): as ARG_SFLAG but checking "--"long
 * ARG_ELSE_UNKNOWN: put it at the end of the if/else if chain to raise an error if *argv doesn't match any previous condition
 * ARG_IF_FLAG(short, long){ ... }: check if *argv is "-"short or "--"long
 * ARG_ERROR(fmt, ...): prints an ARGPARSE error
 * ARG_PRINT_HELP:
 *   it's called when error occurs or when printing the help page is needed
 *   if is not defined before including argparse.h then is set to `print_help()`
 * ARG_PARSE_HELP: parses the "-h" or "--help" arg, calls the ARG_PRINT_HELP and exits with 0
 *
 * remember to put `else` between an ARG_PARSE... and another
 *
 * */

#ifndef ARGPARSE_H__
#define ARGPARSE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARG_PRINT_HELP
#define ARG_PRINT_HELP print_help()
#endif

#define ARG_PARSE             \
  (void)argc;                 \
  for (argv++; *argv; argv++)
#define ARG_SFLAG(short__)           strcmp("-" short__, *argv) == 0
#define ARG_LFLAG(long__)            strcmp("--" long__, *argv) == 0
#define ARG_IF_FLAG(short__, long__) if (ARG_SFLAG(short__) || ARG_LFLAG(long__))
#define ARG_ERROR_(fmt__)                \
  fprintf(stderr, "ERROR: " fmt__ "\n"); \
  ARG_PRINT_HELP;                        \
  exit(1);
#define ARG_ERROR(fmt__, ...)                         \
  fprintf(stderr, "ERROR: " fmt__ "\n", __VA_ARGS__); \
  ARG_PRINT_HELP;                                     \
  exit(1);
#define ARG_ELSE_UNKNOWN                  \
  else {                                  \
    ARG_ERROR("unknown arg: '%s'", *argv) \
  }
#define ARG_PARSE_STRING_ARG_(cond__, str__)       \
  if (cond__) {                                    \
    if (*(argv + 1) == NULL) {                     \
      ARG_ERROR("arg expects string: '%s'", *argv) \
    }                                              \
    argv++;                                        \
    str__ = *argv;                                 \
  }
#define ARG_PARSE_STRING_ARG(short__, long__, str__)                    \
  ARG_PARSE_STRING_ARG_(ARG_SFLAG(short__) || ARG_LFLAG(long__), str__)
#define ARG_PARSE_FLAG_(cond__, flag__) \
  if (cond__) {                         \
    flag__ = 1;                         \
  }
#define ARG_PARSE_FLAG(short__, long__, flag__)                    \
  ARG_PARSE_FLAG_(ARG_SFLAG(short__) || ARG_LFLAG(long__), flag__)
#define ARG_PARSE_HELP_ARG   \
  ARG_IF_FLAG("h", "help") { \
    ARG_PRINT_HELP;          \
    exit(0);                 \
  }
#define ARG_PARSE_INT_ARG_(cond__, var__)                      \
  if (cond__) {                                                \
    if (*(argv + 1) == NULL) {                                 \
      ARG_ERROR("arg expects int: '%s'", *argv)                \
    }                                                          \
    argv++;                                                    \
    var__ = atoi(*argv);                                       \
    if (var__ == 0 && (*argv[0] != '0' || *argv[1] != '\0')) { \
      ARG_ERROR("invalid int: '%s'", *argv)                    \
    }                                                          \
  }
#define ARG_PARSE_INT_ARG(cond__, arg_name__, var__)                         \
  ARG_PARSE_INT_ARG_(ARG_SFLAG(short__) || ARG_LFLAG(long__), long__, var__)

#endif /* ifndef ARGPARSE_H__ */

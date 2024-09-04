#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

void print_location_pretty(location_t location) {
  char *row_end = location.row_start;
  while (*row_end != '\n' && *row_end != '\0') {
    ++row_end;
  }
  int row_len = row_end - location.row_start;
  fprintf(stderr, "%*d | %*.*s\n", location.row < 1000 ? 3 : 5, location.row, row_len, row_len, location.row_start);
  fprintf(stderr,
          "%s   %*.*s%c%*.*s\n",
          location.row < 1000 ? "   " : "     ",
          location.col - 1,
          location.col - 1,
          "                                                                    "
          "                                                                    "
          "                                                                ",
          '^',
          location.len - 1,
          location.len - 1,
          "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
          "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
          "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

location_t location_union(location_t a, location_t b) {
  location_t c = a;
  c.len = b.col + b.len - a.col;
  return c;
}

void eprintfloc_impl(location_t location, const char *file, int line, const char *func, char *fmt, ...) {
  if (1) {
    fprintf(stderr, "ERROR throw at %s:%d in %s\n", file, line, func);
  }
  fprintf(stderr, "ERROR:" LOCATION_FMT " ", LOCATION_UNPACK(location));
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  print_location_pretty(location);
  exit(1);
}

void eprintf_impl(const char *file, int line, const char *func, char *fmt, ...) {
  if (1) {
    fprintf(stderr, "ERROR throw at %s:%d in %s\n", file, line, func);
  }
  fprintf(stderr, "ERROR: ");
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  exit(1);
}

#ifndef ERRORS_H__
#define ERRORS_H__

typedef struct {
  char *filename;
  char *row_start;
  unsigned int row, col;
  unsigned int len;
} location_t;

#define LOCATION_FMT "%s:%02d:%02d:"
#define LOCATION_UNPACK(loc__) (loc__).filename, (loc__).row, (loc__).col

void print_location_pretty(location_t location);
location_t location_union(location_t a, location_t b); 

#define eprintfloc(__loc, ...) eprintfloc_impl((__loc), __LINE__, __FUNCTION__, __VA_ARGS__)
void eprintfloc_impl(location_t location, int line, const char *func, char *fmt, ...);

#define eprintf(...) eprintf_impl(__LINE__, __FUNCTION__, __VA_ARGS__)
void eprintf_impl(int line, const char *func, char *fmt, ...);

#endif // ERRORS_H__
       
#ifdef ERRORS_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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

void eprintfloc_impl(location_t location, int line, const char *func, char *fmt, ...) {
  if (1) {
    fprintf(stderr, "ERROR throw at %d in %s\n", line, func);
  }
  fprintf(stderr, "ERROR:"LOCATION_FMT" ", LOCATION_UNPACK(location));
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  print_location_pretty(location);
  exit(1);
}

void eprintf_impl(int line, const char *func, char *fmt, ...) {
   if (1) {
    fprintf(stderr, "ERROR throw at %d in %s\n", line, func);
  }
  fprintf(stderr, "ERROR: ");
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  exit(1);
}

#endif // ERRORS_IMPLEMENTATION

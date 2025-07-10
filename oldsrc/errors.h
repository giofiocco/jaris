#ifndef ERRORS_H__
#define ERRORS_H__

typedef struct {
  char *filename;
  char *row_start;
  unsigned int row, col;
  unsigned int len;
} location_t;

#define LOCATION_FMT           "%s:%02d:%02d:"
#define LOCATION_UNPACK(loc__) (loc__).filename, (loc__).row, (loc__).col

void print_location_pretty(location_t location);
location_t location_union(location_t a, location_t b);

#define eprintfloc(__loc, ...) eprintfloc_impl((__loc), __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
void eprintfloc_impl(location_t location, const char *file, int line, const char *func, char *fmt, ...);

#define eprintf(...) eprintf_impl(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
void eprintf_impl(const char *file, int line, const char *func, char *fmt, ...);

#endif  // ERRORS_H__

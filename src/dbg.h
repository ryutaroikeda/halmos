#ifndef _HALMOSDBG_H_
#define _HALMOSDBG_H_

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NDEBUG
#define debug(...)
#else
#define debug(...) \
do { \
  fprintf(stderr, "DEBUG %s:%d ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while (0)
#endif

#define clear_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(...) \
do { \
  fprintf(stderr, "ERROR %s:%d [errno: %s] ", \
    __FILE__, __LINE__, clear_errno()); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while (0)

#define log_warn(...) \
do { \
  fprintf(stderr, "WARN %s:%d [errno: %s] ", \
   __FILE__, __LINE__, clear_errno()); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf("\n"); \
} while (0)

#define log_info(...) \
do { \
  fprintf(stderr, "INFO %s:%d ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf("\n"); \
} while (0)

#endif

#ifndef _HALMOSDBG_H_
#define _HALMOSDBG_H_

#include "error.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined (__GNUC__)
#define __FUNC__ __PRETTY_FUNCTION__
#else
#define __FUNC__ "unknown"
#endif

#ifdef NDEBUG
#define debug(...)
#define debug_assert(test, ...)
#define _log(lab, ...)
#else
#define debug(...) _log("DEBUG", __VA_ARGS__)

#define debug_assert(test, ...) \
do { \
  if (!test) { \
    _log("ASSERT FAILED", __VA_ARGS__); \
    abort(); \
  } \
} while (0)

#endif /* NDEBUG */

#define _clearerrno() (errno == 0 ? "None" : strerror(errno))

#define _log(lab, ...) \
do { \
  fprintf(stderr, "%s:%d:%s " lab " [errno: %s] ", __FILE__, __LINE__, \
   __FUNC__, _clearerrno());\
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while (0)

#define _hlog(lab, head, ...) \
do { \
  fprintf(stderr, "%s:%lu:%lu: " lab " ", head.filename, \
    (unsigned long)head.line, (unsigned long)head.offset); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, " [%s]\n", HalmosError_String(head.err)); \
} while (0)

#define log_fat(...) _log("FATAL", __VA_ARGS__)

#define log_err(...) _log("ERROR", __VA_ARGS__)

#define log_warn(...) _log("WARN", __VA_ARGS__)

#define log_info(...) _log("INFO", __VA_ARGS__)

#define hlog_fat(head, ...) _hlog("fatal:", head, __VA_ARGS__)

#define hlog_err(head, ...) _hlog("error:", head, __VA_ARGS__)

#define hlog_warn(head, ...) _hlog("warn:", head, __VA_ARGS__)

#define hlog_info(head, ...) _hlog("info:", head, __VA_ARGS__)

#endif /* _HALMOSDBG_H_ */

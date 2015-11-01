#ifndef _HALMOSLOGGER_H_
#define _HALMOSLOGGER_H_

#define H_LOG(vrf, err, lab, ...) \
do { \
  verifierSetError(vrf, err); \
  fprintf(stderr, "%s:%lu:%lu " lab " [%s] ", \
    vrf->files.vals[vrf->rId].filename.vals, \
    vrf->files.vals[vrf->rId].line, \
    vrf->files.vals[vrf->rId].offset, \
    errorString(err)); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while (0)

#define H_LOG_ERR(vrf, err, ...) H_LOG(vrf, err, "error", __VA_ARGS__)

#define H_LOG_WARN(vrf, err, ...) H_LOG(vrf, err, "warning", __VA_ARGS__)

#endif

#ifndef _HALMOSPREPROC_H_
#define _HALMOSPREPROC_H_
#include "error.h"
struct reader;
struct preproc {
  struct reader* r;
  const char* out;
  enum error err;
  int errCount;
};

void
preprocInit(struct preproc* p);

void
preprocClean(struct preproc* p);

void
preprocParseComment(struct preproc* p);

void
preprocParseInclude(struct preproc* p);

void
preprocParseFile(struct preproc* p, const char* in, const char* out);

#endif

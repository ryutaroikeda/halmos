#ifndef _HALMOSREADER_H_
#define _HALMOSREADER_H_
#include "array.h"
#include "error.h"
#include <stddef.h>
#include <stdio.h>

struct reader;

typedef int (*charGetter)(struct reader*);

struct reader {
  union {
    FILE* f;
    const char* s;
  } stream;
  struct charArray tok;
  struct charArray filename;
  size_t line;
  size_t offset;
  int skipped;
  int didSkip;
  /* the character before a match or EOF with GetToken() and Skip() */
  int last;
  charGetter get;
  enum error err;
};

void
readerInitString(struct reader* r, const char* s);

void
readerInitFile(struct reader* r, FILE* f, const char* filename);

void
readerClean(struct reader* r);

char*
readerGetFilename(struct reader* r);

int
readerGet(struct reader* r);

char*
readerGetToken(struct reader* r, const char* delimiter);

void
readerSkip(struct reader* r, const char* skip);

void
readerFind(struct reader* r, const char* find);

#endif

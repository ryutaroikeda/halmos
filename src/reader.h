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
/* the character before EOF, when using GetToken and Skip */
  int last;
/* file or string */
  int mode;
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

/* initialize the reader depending on mode */
/* if mode is "f" then open the file filename */
/* if mode is "s" then begin reading the string filename */
void
readerOpen(struct reader* r, const char* filename, const char* mode);

/* if a file was opened, close it */
void
readerClose(struct reader* r);

int
readerIsString(const struct reader* r);

int
readerIsFile(const struct reader* r);

typedef struct reader reader;
DECLARE_ARRAY(reader)

#endif

#ifndef _HALMOSREADER_H_
#define _HALMOSREADER_H_

#include "error.h"
#include <stdio.h>

struct HalmosReader;

#define HalmosReader_TokenMax 1024
#define HalmosReader_FilenameMax 256

typedef int (*HalmosCharGetter)(struct HalmosReader*);

struct HalmosReader {
  union {
    FILE* f;
    const char* s;
  } stream;
  char tok[HalmosReader_TokenMax];
  char filename[HalmosReader_FilenameMax];
  size_t line;
  size_t offset;
  int skipped;
  int didSkip;
  HalmosCharGetter get;
  HalmosError err;
};
typedef struct HalmosReader HalmosReader;

HalmosError HalmosReader_InitString(HalmosReader* r, const char* s);

HalmosError 
HalmosReader_InitFile(HalmosReader* r, FILE* f, const char* filename);

int HalmosReader_Get(HalmosReader* r);

HalmosError HalmosReader_GetToken(HalmosReader* r, const char* delimiters);
/* Discard until a char not in skip is found. */
/* The next Get() will return the first such char. */
HalmosError HalmosReader_Skip(HalmosReader* r, const char* skip);
/* Discard until a char in find is found. */
/* The next Get() will return the first such char. */
HalmosError HalmosReader_Find(HalmosReader* r, const char* find);

#endif

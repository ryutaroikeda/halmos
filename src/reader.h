#ifndef _HALMOSREADER_H_
#define _HALMOSREADER_H_

#include "error.h"
#include <stdio.h>

struct HalmosReader;

enum
{
  HalmosReader_TokenSize = 1024
};

typedef int (*HalmosCharGetter)(struct HalmosReader*);

struct HalmosReader {
  union {
    FILE* f;
    char* s;
  } stream;
  char tok[HalmosReader_TokenSize];
  int skipped;
  int didSkip;
  HalmosCharGetter get;
  HalmosError err;
};
typedef struct HalmosReader HalmosReader;

HalmosError HalmosReader_InitString(HalmosReader* r, char* s);

HalmosError HalmosReader_InitFile(HalmosReader* r, FILE* f);

int HalmosReader_Get(HalmosReader* r);

HalmosError HalmosReader_GetToken(HalmosReader* r, const char* delimiters);

HalmosError HalmosReader_Skip(HalmosReader* r, const char* skip);

#endif

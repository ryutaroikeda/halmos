#include "dbg.h"
#include "reader.h"
#include <string.h>

static int HalmosReader_GetFromString(HalmosReader* r)
{
  if (*r->stream.s == '\0') {
    r->err = HalmosError_Reader_EndOfString;
    return '\0';
  }
  return *(r->stream.s++);
}

static int HalmosReader_GetFromFile(HalmosReader* r)
{
  int c = getc(r->stream.f);
  if (c == EOF) {
    r->err = HalmosError_Reader_EndOfFile;
    return EOF;
  }
  return c;
}
HalmosError HalmosReader_Init(HalmosReader* r)
{
  memset(r->tok, 0, HalmosReader_TokenSize);
  r->skipped = 0;
  r->didSkip = 0;
  r->err = HalmosError_None;
  return HalmosError_None;
}

HalmosError HalmosReader_InitString(HalmosReader* r, char* s)
{
  r->stream.s = s;
  r->get = &HalmosReader_GetFromString;
  return HalmosReader_Init(r);
}

HalmosError HalmosReader_InitFile(HalmosReader* r, FILE* f)
{
  r->stream.f = f;
  r->get = &HalmosReader_GetFromFile;
  return HalmosReader_Init(r);
}

int HalmosReader_Get(HalmosReader* r)
{
  if (r->didSkip) {
    r->didSkip = 0;
    return r->skipped;
  }
  return (*r->get)(r);
}

HalmosError HalmosReader_GetToken(HalmosReader* r, const char* delimiters)
{
  int i;
  r->err = HalmosError_None;
  for (i = 0; i < HalmosReader_TokenSize; i++) {
    int c = HalmosReader_Get(r);
    if (r->err != HalmosError_None) {
      r->tok[i] = '\0';
      return r->err;
    }
    if (strchr(delimiters, c)) {
      r->tok[i] = '\0';
      return HalmosError_None;
    }
    r->tok[i] = c;
  }
  r->tok[HalmosReader_TokenSize - 1] = '\0';
  return r->err = HalmosError_Reader_TokenTooBig;
}

HalmosError HalmosReader_Skip(HalmosReader* r, const char* skip)
{
  r->err = HalmosError_None;
  while (1) {
    int c = HalmosReader_Get(r);
    if (r->err != HalmosError_None) {
      return r->err;
    }
    if (!strchr(skip, c)) {
      r->skipped = c;
      r->didSkip = 1;
      return HalmosError_None;
    }
  }
  return r->err = HalmosError_Reader_Impossible;
}

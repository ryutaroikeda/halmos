#include "dbg.h"
#include "reader.h"
#include <string.h>

static int HalmosReader_GetFromString(HalmosReader* r)
{
  if (*r->stream.s == '\0') {
    r->err = HalmosError_EndOfString;
    return '\0';
  }
  return *(r->stream.s++);
}

static int HalmosReader_GetFromFile(HalmosReader* r)
{
  int c = getc(r->stream.f);
  if (c == EOF) {
    r->err = HalmosError_EndOfFile;
    return EOF;
  }
  return c;
}

HalmosError HalmosReader_Init(HalmosReader* r)
{
  memset(r->tok, 0, HalmosReader_TokenMax);
  r->line = 1;
  r->offset = 0;
  r->skipped = 0;
  r->didSkip = 0;
  r->err = HalmosError_None;
  return HalmosError_None;
}

HalmosError HalmosReader_InitString(HalmosReader* r, const char* s)
{
  r->stream.s = s;
  r->get = &HalmosReader_GetFromString;
  memset(r->filename, 0, HalmosReader_FilenameMax);
  return HalmosReader_Init(r);
}

HalmosError
HalmosReader_InitFile(HalmosReader* r, FILE* f, const char* filename)
{
  r->stream.f = f;
  r->get = &HalmosReader_GetFromFile;
  strncpy(r->filename, filename, HalmosReader_FilenameMax);
  return HalmosReader_Init(r);
}

int HalmosReader_Get(HalmosReader* r)
{
  int c;
  if (r->didSkip) {
    r->didSkip = 0;
    c = r->skipped;
  } else {
    c = (*r->get)(r);
  }
  if (c == '\n') {
    r->line++;
    r->offset = 0;
  } else {
    r->offset++;
  }
  return c;
}

HalmosError HalmosReader_GetToken(HalmosReader* r, const char* delimiters)
{
  int i;
  r->err = HalmosError_None;
  for (i = 0; i < HalmosReader_TokenMax; i++) {
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
  r->tok[HalmosReader_TokenMax - 1] = '\0';
  return r->err = HalmosError_TokenTooBig;
}

HalmosError
HalmosReader_SkipExplicit(HalmosReader* r, const char* s, int skipOnMatch)
{
  r->err = HalmosError_None;
  while (1) {
    int c = HalmosReader_Get(r);
    if (r->err != HalmosError_None) {
      return r->err;
    }
    char* isMatch = strchr(s, c);
    if ((isMatch && !skipOnMatch) || (!isMatch && skipOnMatch)) {
      r->skipped = c;
      r->didSkip = 1;
      break;
    }
  }
  return HalmosError_None;
}

HalmosError HalmosReader_Skip(HalmosReader* r, const char* skip)
{
  return HalmosReader_SkipExplicit(r, skip, 1);
}

HalmosError HalmosReader_Find(HalmosReader* r, const char* find)
{
  return HalmosReader_SkipExplicit(r, find, 0);
}

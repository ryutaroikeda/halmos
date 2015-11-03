#include "dbg.h"
#include "reader.h"
#include <string.h>

static const int mode_none = 0;
static const int mode_string = 1;
static const int mode_file = 2;

int
readerGetString(struct reader* r)
{
  if (*r->stream.s == '\0') {
    r->err = error_endOfString;
    return '\0';
  }
  return *(r->stream.s++);
}

int
readerGetFile(struct reader* r)
{
  int c = getc(r->stream.f);
  if (c == EOF) {
    r->err = error_endOfFile;
    return EOF;
  }
  return c;
}

void
readerInit(struct reader* r)
{
  charArrayInit(&r->tok, 256);
  charArrayInit(&r->filename, 256);
  r->line = 1;
  r->offset = 0;
  r->skipped = 0;
  r->didSkip = 0;
  r->last = 0;
  r->mode = mode_none;
  r->get = NULL;
  r->err = error_none;
}

void
readerInitString(struct reader* r, const char* s)
{
  readerInit(r);
  charArrayAppend(&r->filename, "", 1);
  r->stream.s = s;
  r->mode = mode_string;
  r->get = &readerGetString;
}

void
readerInitFile(struct reader* r, FILE* f, const char* filename)
{
  readerInit(r);
  charArrayAppend(&r->filename , filename, strlen(filename) + 1);
  r->stream.f = f;
  r->mode = mode_file;
  r->get = &readerGetFile;
}

void
readerClean(struct reader* r)
{
  charArrayClean(&r->tok);
  charArrayClean(&r->filename);
}

char*
readerGetFilename(struct reader* r)
{
  return r->filename.vals;
}

int
readerGet(struct reader* r)
{
  if (r->didSkip) {
    r->didSkip = 0;
    return r->skipped;
  }
  int c = (*r->get)(r);
  if (c == '\n') {
    r->line++;
    r->offset = 0;
  } else {
    r->offset++;
  }
  return c;
}

int
readerPeek(struct reader* r)
{
  if (r->didSkip) {
    return r->skipped;
  } else {
    int c = readerGet(r);
    r->didSkip = 1;
    r->skipped = c;
    return c;
  }
}

char*
readerGetToken(struct reader* r, const char* delimiters)
{
  r->err = error_none;
  charArrayEmpty(&r->tok);
  while (1) {
    int c = readerGet(r);
    if (r->err != error_none) {
      charArrayAdd(&r->tok, '\0');
      break;
    }
    if (strchr(delimiters, c)) {
      charArrayAdd(&r->tok, '\0');
      r->last = c;
      break;
    }
    charArrayAdd(&r->tok, c);
    r->last = c;
  }
  return r->tok.vals;
}

void
readerSkipExplicit(struct reader* r, const char* s, int skipOnMatch)
{
  r->err = error_none;
  while (1) {
    int c = readerGet(r);
    if (r->err != error_none) {
      break;
    }
    char* isMatch = strchr(s, c);
    if ((isMatch && !skipOnMatch) || (!isMatch && skipOnMatch)) {
      r->skipped = c;
      r->didSkip = 1;
      break;
    }
    r->last = c;
  }
}

void
readerSkip(struct reader* r, const char* skip)
{
  readerSkipExplicit(r, skip, 1);
}

void
readerFind(struct reader* r, const char* find)
{
  readerSkipExplicit(r, find, 0);
}

DEFINE_ARRAY(reader)

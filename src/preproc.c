#include "array.h"
#include "dbg.h"
#include "logger.h"
#include "preproc.h"
#include "reader.h"

static const char* whitespace = " \f\n\r\t";

void
preprocInit(struct preproc* p)
{
  p->rs = xmalloc(sizeof(struct readerArray));
  readerArrayInit(p->rs, 1);
  p->r = NULL;
  p->err = error_none;
  p->errCount = 0;
}

void
preprocClean(struct preproc* p)
{
  readerArrayClean(p->rs);
  free(p->rs);
}

void
preprocSetError(struct preproc* p, enum error err)
{
  p->err = err;
  p->errCount++;
}

int
preprocIsFresh(const struct preproc* p, const char* filename)
{
  size_t i;
  for (i = 0; i < p->rs->size; i++) {
    if (strcmp(readerGetFilename(&p->rs->vals[i]), filename) == 0) {
      return 0;
    }
  }
  return 1;
}

void
preprocBeginReading(struct preproc* p, FILE* f, const char* filename)
{
  struct reader r;
  readerInitFile(&r, f, filename);
  readerArrayAdd(p->rs, r);
  p->r = &p->rs->vals[p->rs->size - 1];
}

void
preprocParseComment(struct preproc* p)
{
  while (!p->r->err) {
    readerFind(p->r, "$");
    char* tok = readerGetToken(p->r, whitespace);
    if (p->r->err) { break; }
    if (strlen(tok) != 2) { continue; }
    if (tok[1] == ')') { break; }
    if (tok[1] == '(') {
      P_LOG_ERR(p, error_nestedComment, "comments cannot be nested");
    }
  }
  if (p->r->err) {
    P_LOG_ERR(p, error_unterminatedComment, "reached end of file before $)");
  }
}

void
preprocParseInclude(struct preproc* p, FILE* fOut)
{
  readerSkip(p->r, whitespace);
  char* tok = readerGetToken(p->r, whitespace);
  if (p->r->err) {
    P_LOG_ERR(p, error_unterminatedFileInclusion, 
      "reached end of file before $]");
    return;
  }
/* if it is a new file, parse it */
  if (preprocIsFresh(p, tok)) {
    struct reader* r = p->r;
    preprocParseFile(p, tok, fOut);
    p->r = r;
  }
/* find $] */
  tok = readerGetToken(p->r, whitespace);
  if (p->r->err) {
    P_LOG_ERR(p, error_unterminatedFileInclusion, 
      "reached end of file before $]");
    return;
  }
  if (strcmp(tok, "$]") != 0) {
    P_LOG_ERR(p, error_expectedClosingBracket, "%s found instead of $]", tok);
    while (!p->r->err) {
      readerFind(p->r, "$");
      tok = readerGetToken(p->r, whitespace);
      if (p->r->err) { break; }
      if (strlen(tok) != 2) { continue; }
      if (tok[1] == ']') { break; }
    }
    if (p->r->err) {
      P_LOG_ERR(p, error_unterminatedFileInclusion, 
        "reached end of file before $]");
    }
  }
}

void
preprocParseFile(struct preproc* p, const char* in, FILE* fOut)
{
  FILE* fIn = fopen(in, "r");
  if (!fIn) {
    P_LOG_ERR(p, error_failedOpenFile, "failed to open input file %s", in);
    return;
  }
  preprocBeginReading(p, fIn, in);
  while (!p->r->err) {
    int c = readerGet(p->r);
    if (p->r->err) { break; }
    if (c == '$') {
      c = readerGet(p->r);
      if (p->r->err) { break; }
      if (c == '(') {
        preprocParseComment(p);
      } else if (c == '[') {
        preprocParseInclude(p, fOut);
      } else {
        fprintf(fOut, "$%c", c);
      }
    } else {
      fprintf(fOut, "%c", c);
    }
  }
  readerClean(p->r);
  fclose(fIn);
}

void
preprocCompile(struct preproc* p, const char* in, const char* out)
{
  FILE* fOut = fopen(out, "w");
  if (!fOut) {
    P_LOG_ERR(p, error_failedOpenFile, "failed to open output file %s", out);
    return;
  }
  preprocParseFile(p, in, fOut);
  fclose(fOut);
}

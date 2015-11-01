#include "array.h"
#include "dbg.h"
#include "logger.h"
#include "preproc.h"
#include "reader.h"

static const char* whitespace = " \f\n\r\t";

void
preprocInit(struct preproc* p)
{
  p->r = xmalloc(sizeof(struct reader));
  p->out = NULL;
  p->err = error_none;
  p->errCount = 0;
}

void
preprocClean(struct preproc* p)
{
  free(p->r);
}

void
preprocSetError(struct preproc* p, enum error err)
{
  p->err = err;
  p->errCount++;
}

void
preprocParseComment(struct preproc* p)
{
  while (!p->r->err) {
    readerFind(p->r, "$");
    char* tok = readerGetToken(p->r, whitespace);
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
preprocParseInclude(struct preproc* p)
{
  readerSkip(p->r, whitespace);
  char* tok = readerGetToken(p->r, whitespace);
  struct preproc q;
  preprocInit(&q);
  preprocParseFile(&q, tok, p->out);
  preprocClean(&q);
/* find $] */
  tok = readerGetToken(p->r, whitespace);
  if (strcmp(tok, "$]") != 0) {
    P_LOG_ERR(p, error_expectedClosingBracket, "%s found instead of $]", tok);
    while (!p->r->err) {
      readerFind(p->r, "$");
      tok = readerGetToken(p->r, whitespace);
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
preprocParseFile(struct preproc* p, const char* in, const char* out)
{
  p->out = out;
  FILE* fIn = fopen(in, "r");
  if (!fIn) {
    P_LOG_ERR(p, error_failedOpenFile, "failed to open input file %s", in);
  }
  FILE* fOut = fopen(out, "w");
  if (!fOut) {
    P_LOG_ERR(p, error_failedOpenFile, "failed to open output file %s", out);
  }
  readerInitFile(p->r, fIn, in);
  while (!p->r->err) {
    int c = readerGet(p->r);
    if (p->r->err) { break; }
    if (c == '$') {
      c = readerGet(p->r);
      if (p->r->err) { break; }
      if (c == '(') {
        preprocParseComment(p);
      } else if (c == '[') {
        preprocParseInclude(p);
      } else {
        fprintf(fOut, "$%c", c);
      }
    } else {
      fprintf(fOut, "%c", c);
    }
  }
  readerClean(p->r);
  fclose(fOut);
  fclose(fIn);
}

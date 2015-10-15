#include "verifier.h"

DEFINE_ARRAY(symbol)
DEFINE_ARRAY(statement)
DEFINE_ARRAY(frame)
DEFINE_ARRAY(reader)

void
symbolInit(struct symbol* sym)
{
  charArrayInit(&sym->sym, 1);
  sym->type = symType_none;
}

void
symbolClean(struct symbol* sym)
{
  charArrayClean(&sym->sym);
}

char*
symbolGetName(struct symbol* sym)
{
  return sym->sym.vals;
}

void
statementInit(struct statement* stmt)
{
  size_tArrayInit(&stmt->syms, 1);
  stmt->isMandatory = 0;
}

void
statementClean(struct statement* stmt)
{
  size_tArrayClean(&stmt->syms);
}

void
frameInit(struct frame* frm)
{
  size_tArrayInit(&frm->stmts, 1);
  size_tArrayInit(&frm->disjoint1, 1);
  size_tArrayInit(&frm->disjoint2, 1);
}

void
frameClean(struct frame* frm)
{
  size_tArrayClean(&frm->stmts);
  size_tArrayClean(&frm->disjoint1);
  size_tArrayClean(&frm->disjoint2);
}

void
verifierInit(struct verifier* vrf)
{
  readerArrayInit(&vrf->files, 1);
  symbolArrayInit(&vrf->symbols, 1);
  statementArrayInit(&vrf->stmts, 1);
  frameArrayInit(&vrf->frames, 1);
  vrf->r = NULL;
}

void
verifierClean(struct verifier* vrf)
{
  size_t i;
  for (i = 0; i < vrf->files.size; i++) {
    readerClean(&vrf->files.vals[i]);
  }
  readerArrayClean(&vrf->files);
  for (i = 0; i < vrf->symbols.size; i++) {
    symbolClean(&vrf->symbols.vals[i]);
  }
  symbolArrayClean(&vrf->symbols);
  for (i = 0; i < vrf->stmts.size; i++) {
    statementClean(&vrf->stmts.vals[i]);
  }
  statementArrayClean(&vrf->stmts);
  for (i = 0; i < vrf->frames.size; i++) {
    frameClean(&vrf->frames.vals[i]);
  }
  frameArrayClean(&vrf->frames);
}
static const char tokBeginKeyword = '$';
static const char tokEndStatement = '.';
static const char whitespace[] = " \t\r\f\n";

void
verifierSetError(struct verifier* vrf, enum error err)
{
  vrf->err = err;
  /* to do: add vrf->errorsum for more details on the error */
}

int
verifierIsFreshSymbol(struct verifier* vrf, const char* sym)
{
  size_t i;
  for (i = 0; i < vrf->symbols.size; i++) {
    if (strcmp(symbolGetName(&vrf->symbols.vals[i]), sym) == 0) {
      verifierSetError(vrf, error_duplicateSymbol);
      return 0;
    }
  }
  return 1;
}

int
verifierIsFreshFile(struct verifier* vrf, const char* file)
{
  size_t i;
  for (i = 0; i < vrf->files.size; i++) {
    if (strcmp(readerGetFilename(&vrf->files.vals[i]), file) == 0) {
      verifierSetError(vrf, error_duplicateFile);
      return 0;
    }
  }
  return 1;
}

void verifierAddFile(struct verifier* vrf, struct reader* r)
{
  if (!verifierIsFreshFile(vrf, readerGetFilename(r))) {
    LOG_INFO("file %s was already added", readerGetFilename(r));
    return;
  }
  readerArrayAdd(&vrf->files, *r);
}

void
verifierAddSymbol(struct verifier* vrf, const char* sym, enum symType type)
{
  if (!verifierIsFreshSymbol(vrf, sym)) {
    LOG_ERR("%s was already used", sym);
    return;
  }
  struct symbol s;
  symbolInit(&s);
  charArrayAppend(&s.sym, sym, strlen(sym));
  charArrayAdd(&s.sym, '\0');
  s.type = type;
  symbolArrayAdd(&vrf->symbols, s);
}

void
verifierAddConstant(struct verifier* vrf, const char* sym)
{
  verifierAddSymbol(vrf, sym, symType_constant);
}

void
verifierAddVariable(struct verifier* vrf, const char* sym)
{
  verifierAddSymbol(vrf, sym, symType_variable);
}

void 
verifierParseSymbol(struct verifier* vrf, int* isEndOfStatement)
{
  char* tok;
  vrf->err = error_none;
  *isEndOfStatement = 0;
  readerSkip(vrf->r, whitespace);
  tok = readerGetToken(vrf->r, whitespace);
/* check for end of statement */
  if (tok[0] == tokBeginKeyword) {
    size_t len = strlen(tok);
    if (len <= 1 || (tok[1] != tokEndStatement)) {
      verifierSetError(vrf, error_expectedEndStatement);
      LOG_ERR("expected . after $ instead of %s", tok);
      return;
    } else if (len > 2) {
      verifierSetError(vrf, error_expectedWhitespace);
      LOG_ERR("expected whitespace after $. in %s", tok);
      return;
    } else if (vrf->r->err != error_none) {
/* len == 2 and end of file without new line */
      verifierSetError(vrf, error_expectedNewLine);
      LOG_ERR("expected line break after $.");
      return;
    }
    *isEndOfStatement = 1;
    return;
  }
/* check for end of file */
  if (vrf->r->err == error_endOfString || vrf->r->err == error_endOfFile) {
    verifierSetError(vrf, error_unterminatedStatement);
    LOG_ERR("reached end of file before $.");
    return;
  }
/* check the symbol name does not contain $ */
  if (strchr(tok, tokBeginKeyword)) {
    verifierSetError(vrf, error_invalidSymbolName);
    LOG_ERR("the symbol %s contains $", tok);
    return;
  }
  return;
}

void
verifierParseConstants(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  while (1) {
    verifierParseSymbol(vrf, &isEndOfStatement);
    if (vrf->err) { break; }
    if (isEndOfStatement) { break; }
    verifierAddConstant(vrf, vrf->r->tok.vals);
    if (vrf->err) { break; }
  }
}

void
verifierParseVariables(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  while (1) {
    verifierParseSymbol(vrf, &isEndOfStatement);
    if (vrf->err) { break; }
    if (isEndOfStatement) { break; }
    verifierAddVariable(vrf, vrf->r->tok.vals);
    if (vrf->err) { break; }
  }
}

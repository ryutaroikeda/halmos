#include "verifier.h"

DEFINE_ARRAY(symbol)
DEFINE_ARRAY(frame)
DEFINE_ARRAY(reader)

void
symbolInit(struct symbol* sym)
{
  charArrayInit(&sym->sym, 1);
  sym->type = symType_none;
  sym->isActive = 0;
  sym->isTyped = 0;
  sym->scope = 0;
  sym->stmt = 0;
  sym->frame = 0;
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
frameInit(struct frame* frm)
{
  (void) frm;
  // size_tArrayInit(&frm->stmts, 1);
  // size_tArrayInit(&frm->disjoint1, 1);
  // size_tArrayInit(&frm->disjoint2, 1);
}

void
frameClean(struct frame* frm)
{
  (void) frm;
  // size_tArrayClean(&frm->stmts);
  // size_tArrayClean(&frm->disjoint1);
  // size_tArrayClean(&frm->disjoint2);
}

void
verifierInit(struct verifier* vrf)
{
  readerArrayInit(&vrf->files, 1);
  symbolArrayInit(&vrf->symbols, 1);
  symstringArrayInit(&vrf->stmts, 1);
  frameArrayInit(&vrf->frames, 1);
  vrf->r = NULL;
  size_tArrayInit(&vrf->scope, 1);
  symstringArrayInit(&vrf->stack, 1);
  vrf->rId = 0;
  vrf->err = error_none;
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
    symstringClean(&vrf->stmts.vals[i]);
  }
  symstringArrayClean(&vrf->stmts);
  for (i = 0; i < vrf->frames.size; i++) {
    frameClean(&vrf->frames.vals[i]);
  }
  frameArrayClean(&vrf->frames);
  size_tArrayClean(&vrf->scope);
  symstringArrayClean(&vrf->stack);
}

// static const char tokBeginKeyword = '$';
// static const char tokEndStatement = '.';
// static const char tokEndComment = ')';
static const char whitespace[] = " \t\r\f\n";

void
verifierSetError(struct verifier* vrf, enum error err)
{
  vrf->err = err;
  /* to do: add vrf->errorsum for more details on the error */
}

size_t
verifierGetSymId(struct verifier* vrf, const char* sym)
{
  size_t i;
  vrf->err = error_none;
  for (i = 0; i < vrf->symbols.size; i++) {
    if (strcmp(vrf->symbols.vals[i].sym.vals, sym) == 0) {
      return i;
    }
  }
  verifierSetError(vrf, error_undefinedSymbol);
  return 0;
}

int
verifierIsFreshSymbol(struct verifier* vrf, const char* sym)
{
  size_t symId = verifierGetSymId(vrf, sym);
  if (vrf->err == error_undefinedSymbol) {
    vrf->err = error_none;
    return 1;
  }
  if (!vrf->symbols.vals[symId].isActive) {
    return 1;
  }
  verifierSetError(vrf, error_duplicateSymbol);
  return 0;
}

int
verifierIsFreshFile(struct verifier* vrf, const char* file)
{
  size_t i;
  vrf->err = error_none;
  for (i = 0; i < vrf->files.size; i++) {
    if (strcmp(readerGetFilename(&vrf->files.vals[i]), file) == 0) {
      verifierSetError(vrf, error_duplicateFile);
      return 0;
    }
  }
  return 1;
}

int
verifierIsType(struct verifier* vrf, size_t symId, enum symType type)
{
  enum symType t = vrf->symbols.vals[symId].type;
  if (t == type) { return 1; }
  if (type == symType_constant) {
    verifierSetError(vrf, error_expectedConstantSymbol);
  } else if (type == symType_variable) {
    verifierSetError(vrf, error_expectedVariableSymbol);
  }
  return 0;
}

void
verifierAddFile(struct verifier* vrf, struct reader* r)
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
  struct symbol s;
  symbolInit(&s);
  charArrayAppend(&s.sym, sym, strlen(sym));
  charArrayAdd(&s.sym, '\0');
  s.type = type;
  s.isActive = 1;
  s.scope = vrf->scope.vals[vrf->rId];
  s.stmt = vrf->stmts.size;
  s.frame = vrf->frames.size;
  s.file = vrf->rId;
  s.line = vrf->r->line;
  s.offset = vrf->r->offset;
  symbolArrayAdd(&vrf->symbols, s);
}

void
verifierAddStatement(struct verifier* vrf, struct symstring* stmt)
{
  symstringArrayAdd(&vrf->stmts, *stmt);
}

void
verifierAddConstant(struct verifier* vrf, const char* sym)
{
  if (!verifierIsFreshSymbol(vrf, sym)) {
    LOG_ERR("%s was already used", sym);
    return;
  }
  verifierAddSymbol(vrf, sym, symType_constant);
}

void
verifierAddVariable(struct verifier* vrf, const char* sym)
{
  if (!verifierIsFreshSymbol(vrf, sym)) {
    LOG_ERR("%s was already used", sym);
    return;
  }
  verifierAddSymbol(vrf, sym, symType_variable);
}

/* deactivate non-global symbols in the given nesting level */
void
verifierDeactivateSymbols(struct verifier* vrf, size_t scope)
{
  size_t i;
  for (i = 0; i < vrf->symbols.size; i++) {
    struct symbol* s = &vrf->symbols.vals[i];
    if (s->type == symType_constant) {
      continue;
    } else if (s->type == symType_assertion) {
      continue;
    } else if (s->type == symType_provable) {
      continue;
    }
    if (s->scope == scope) {
      s->isActive = 0;
    }
  }
}

void
verifierParseComment(struct verifier* vrf)
{
  char* tok;
  vrf->err = error_none;
  while (1) {
    readerFind(vrf->r, "$");
    if (vrf->r->err) {
      verifierSetError(vrf, error_unterminatedComment);
      LOG_ERR("reached end of file before $)");
      return;
    }
    tok = readerGetToken(vrf->r, whitespace);
    if (vrf->r->err) {
      verifierSetError(vrf, error_expectedNewLine);
      LOG_ERR("expected line break in the last line");
      return;
    }
    size_t len = strlen(tok);
    if (len != 2) { continue; }
    if (tok[1] == '(') {
      verifierSetError(vrf, error_unexpectedKeyword);
      LOG_ERR("nested comments are forbidden");
      return;
    }
    if (tok[1] == ')') {
      break;
    }
  }
}

char* 
verifierParseSymbol(struct verifier* vrf, int* isEndOfStatement)
{
  char* tok;
  vrf->err = error_none;
  *isEndOfStatement = 0;
  readerSkip(vrf->r, whitespace);
/* check for end of file */
  if (vrf->r->err == error_endOfString || vrf->r->err == error_endOfFile) {
    verifierSetError(vrf, error_unterminatedStatement);
    LOG_ERR("reached end of file before end of statement");
    return NULL;
  }
  tok = readerGetToken(vrf->r, whitespace);
/* check for end of statement */
  if (tok[0] == '$') {
    size_t len = strlen(tok);
    if (len != 2) {
      verifierSetError(vrf, error_invalidKeyword);
      LOG_ERR("%s is not a valid keyword", tok);
    }
    if (tok[1] == '.') {
      *isEndOfStatement = 1;
    } else if (tok[1] == '=') {
/* for $p statements */
      *isEndOfStatement = 1;
    } else if (tok[1] == '(') {
      verifierParseComment(vrf);
      return verifierParseSymbol(vrf, isEndOfStatement);
    } else {
      verifierSetError(vrf, error_unexpectedKeyword);
      LOG_ERR("expected $. $= or $( instead of %s", tok);
    }
    return tok;
  }
/* check for end of file */
  if (vrf->r->err == error_endOfString || vrf->r->err == error_endOfFile) {
    verifierSetError(vrf, error_unterminatedStatement);
    LOG_ERR("reached end of file before $.");
    return tok;
  }
/* check the symbol name does not contain $ */
  if (strchr(tok, '$')) {
    verifierSetError(vrf, error_invalidSymbol);
    LOG_ERR("the symbol %s contains $", tok);
    return tok;
  }
  return tok;
}

void
verifierParseConstants(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  char* tok;
  while (1) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement);
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    verifierAddConstant(vrf, tok);
    if (vrf->err) { return; }
  }
}

void
verifierParseVariables(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  char* tok;
  while (1) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement);
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    verifierAddVariable(vrf, tok);
    if (vrf->err) { return; }
  }
}

void 
verifierParseFloat(struct verifier* vrf, struct symstring* stmt)
{
  vrf->err = error_none;
  int isEndOfStatement;
  char* tok;
  tok = verifierParseSymbol(vrf, &isEndOfStatement);
  if (vrf->err) { return; }
  if (isEndOfStatement) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("statement ended before constant symbol");
    return;
  }
  size_t symId = verifierGetSymId(vrf, tok);
  if (vrf->err) {
    LOG_ERR("%s is not defined", tok);
    return;
  }
  if (!verifierIsType(vrf, symId, symType_constant)) {
    LOG_ERR("%s is not a constant symbol", tok);
    return;
  }
  symstringAdd(stmt, symId);
  tok = verifierParseSymbol(vrf, &isEndOfStatement);
  if (vrf->err) { return; }
  if (isEndOfStatement) {
    verifierSetError(vrf, error_expectedVariableSymbol);
    LOG_ERR("statement ended before variable symbol");
    return;
  }
  symId = verifierGetSymId(vrf, tok);
  if (vrf->err) {
    LOG_ERR("%s is not defined", tok);
    return;
  }
  if (verifierIsType(vrf, symId, symType_variable)) {
    LOG_ERR("%s is not a variable symbol", tok);
    return;
  }
  symstringAdd(stmt, symId);
  vrf->symbols.vals[symId].isTyped = 1;
}

/* return the symId of a variable or set isEndOfStatement or set vrf->err */
size_t verifierParseDisjoint(struct verifier* vrf, int* isEndOfStatement)
{
  char* tok;
  vrf->err = error_none;
  *isEndOfStatement = 0;
  readerSkip(vrf->r, whitespace);
  tok = readerGetToken(vrf->r, whitespace);
  if (strcmp(tok, "$.") == 0) {
    *isEndOfStatement = 1;
    return 0;
  }
  size_t symId = verifierGetSymId(vrf, tok);
  if (vrf->err) {
    LOG_ERR("%s is not defined", tok);
  } else if (!verifierIsType(vrf, symId, symType_variable)) {
    verifierSetError(vrf, error_expectedVariableSymbol);
    LOG_ERR("%s is not a variable symbol", tok);
  } else if (!vrf->symbols.vals[symId].isTyped) {
    verifierSetError(vrf, error_untypedVariable);
    LOG_ERR("%s must be typed with a $f statement", tok);
  }
  return symId;
}

void
verifierParseDisjoints(struct verifier* vrf)
{
  vrf->err = error_none;
  size_t symId;
  int isEndOfStatement;
  struct size_tArray vars;
  size_tArrayInit(&vars, 2);
  symId = verifierParseDisjoint(vrf, &isEndOfStatement);
  size_tArrayAdd(&vars, symId);
  while (!isEndOfStatement && !vrf->err) {
    symId = verifierParseDisjoint(vrf, &isEndOfStatement);
    if (isEndOfStatement) {
      break;
    }
/* add an entry for each pair of disjoint variables */
    size_t i;
    for (i = 0; i < vars.size; i++) {
/* add a symbol for the disjoint pair */
      verifierAddSymbol(vrf, "", symType_disjoint);
      struct symstring stmt;
      symstringInit(&stmt);
      symstringAdd(&stmt, symId);
      symstringAdd(&stmt, vars.vals[i]);
      symstringArrayAdd(&vrf->stmts, stmt);
    }
    size_tArrayAdd(&vars, symId);
  }
  size_tArrayClean(&vars);
}

void
verifierParseStatementContent(struct verifier* vrf, struct symstring* stmt,
  struct frame* frm)
{
  (void) frm;
  vrf->err = error_none;
  char* tok;
  int isEndOfStatement;
  size_t symId;
  tok = verifierParseSymbol(vrf, &isEndOfStatement);
  if (isEndOfStatement) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("expected constant symbol before end of statement");
    return;
  }
  symId = verifierGetSymId(vrf, tok);
  if (vrf->err) {
    LOG_ERR("%s is not defined", tok);
    return;
  }
  if (!verifierIsType(vrf, symId, symType_constant)) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("%s is not a constant", tok);
    return;
  }
  symstringAdd(stmt, symId);
  while (1) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement);
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    symId = verifierGetSymId(vrf, tok);
    if (vrf->err) {
      LOG_ERR("%s is not defined", tok);
      return;
    }
    if (verifierIsType(vrf, symId, symType_variable)) {
      if (!vrf->symbols.vals[symId].isTyped) {
        verifierSetError(vrf, error_invalidSymbol);
        LOG_ERR("%s needs to be typed by $f", tok);
        return;
      }
    }
    symstringAdd(stmt, symId);
  }
}

void
verifierParseAssertion(struct verifier* vrf, struct symstring* stmt,
 struct frame* frm)
{
  verifierParseStatementContent(vrf, stmt, frm);

}

/* push an entry on to the rpn stack */
void
verifierPush(struct verifier* vrf, struct symstring* str)
{
  symstringArrayAdd(&vrf->stack, *str);
}

/* pop the top of the stack. The caller must clean the returned symstring */
struct symstring
verifierPop(struct verifier* vrf)
{
  struct symstring str;
  vrf->err = error_none;
  if (vrf->stack.size == 0) {
    verifierSetError(vrf, error_stackUnderflow);
    LOG_ERR("stack is empty");
    return str;
  }
  str = vrf->stack.vals[vrf->stack.size - 1];
  vrf->stack.size--;
  return str;
}

/* get substitution for floating to match a. a will be modified */
void
verifierUnify(struct verifier* vrf, struct substitution* sub,
 struct symstring* a, const struct symstring* floating)
{
  if (a->vals[0] != floating->vals[0]) {
    verifierSetError(vrf, error_mismatchedType);
/* to do: should report details of symbol, not just the symstring */
    LOG_ERR("type error");
    return;
  }
  symstringDelete(a, 0);
  substitutionAdd(sub, floating->vals[1], a);
}

/* use an assertion or a theorem. Pop the appropriate number of entries,  */
/* type-check, do unification and push the result */
void
verifierApplyAssertion(struct verifier* vrf, size_t symId)
{
  size_t i;
  vrf->err = error_none;
/* pop into this array. The last one out is the first argument */
  struct symstringArray args;
  symstringArrayInit(&args, 1);
  const struct symbol* sym = &vrf->symbols.vals[symId];
  const struct frame* frm = &vrf->frames.vals[sym->frame];
  for (i = 0; i < frm->stmts.size; i++) {
    struct symstring str = verifierPop(vrf);
    if (vrf->err) { break; }
    symstringArrayAdd(&args, str);
  }
  struct substitution sub;
  substitutionInit(&sub);

/* build this entry to push */
  struct symstring res;
  symstringInit(&res);
  symstringAppend(&res, &vrf->stmts.vals[sym->stmt]);

/* to do: pop, type-check, unify and substitute, */
/* check disjoint variables */  
  symstringArrayAdd(&vrf->stack, res);
/* clean up */
  for (i = 0; i < args.size; i++) {
    symstringClean(&args.vals[i]);
  }
  symstringArrayClean(&args);
  substitutionClean(&sub);
}

void
verifierParseProofSymbol(struct verifier* vrf, int* isEndOfProof)
{
  char* tok;
  vrf->err = error_none;
  *isEndOfProof = 0;
  readerSkip(vrf->r, whitespace);
  tok = readerGetToken(vrf->r, whitespace);
  if (strcmp(tok, "$.") == 0) {
    *isEndOfProof = 1;
    return;
  }
  const struct symbol* sym;
  const struct symstring* stmt;
  struct symstring entry;
  symstringInit(&entry);
  size_t symId;
  symId = verifierGetSymId(vrf, tok);
  sym = &vrf->symbols.vals[symId];
  stmt = &vrf->stmts.vals[sym->stmt];
/* push float and assertions on the stack */
  if (sym->type == symType_floating || sym->type == symType_assertion) {
    symstringAppend(&entry, stmt);
    symstringArrayAdd(&vrf->stack, entry);
  }
/* for theorems, pop 'arguments' and push back the result */
  else if (sym->type == symType_provable) {
    struct frame* frm;
    frm = &vrf->frames.vals[sym->frame];

  }

}
void
verifierParseProof(struct verifier* vrf)
{
  (void) vrf;
}
// void
// verifierParseProvable(struct verifier* vrf, struct statement* stmt)
// {
//   verifierParseStatementContent(vrf, stmt);
//   /* to do: parse the proof */
// }

void
verifierParseStatement(struct verifier* vrf, int* isEndOfScope)
{
  vrf->err = error_none;
  char* tok;
  char* keyword;
  enum symType type = symType_none;
  *isEndOfScope = 0;
/* the beginning of the statement. Get the label */
  readerSkip(vrf->r, whitespace);
  tok = readerGetToken(vrf->r, whitespace);
/* check if we are at the end of the current scope or the end of file */
  if (vrf->r->err) {
    *isEndOfScope = 1;
/* the file must end with a new line character */
    if (!strchr("\n\f", vrf->r->last)) {
      verifierSetError(vrf, error_expectedNewLine);
      LOG_ERR("expected new line at the end of file");
    }
    return;
  }
  if (tok[0] == '$') {
    size_t len = strlen(tok);
    if (len != 2) {
      verifierSetError(vrf, error_invalidKeyword);
      LOG_ERR("%s is not a keyword", tok);
      return;
    }
    if (tok[1] == 'c') {
      verifierParseConstants(vrf);
    } else if (tok[1] == 'v') {
      verifierParseVariables(vrf);
    } else if (tok[1] == 'd') {
      verifierParseDisjoints(vrf);
    } else if (tok[1] == '{') {
      vrf->scope.vals[vrf->rId]++;
      verifierParseBlock(vrf);
    } else if (tok[1] == '}') {
      *isEndOfScope = 1;
    } else if (tok[1] == '(') {
      verifierParseComment(vrf);
    } else {
      verifierSetError(vrf, error_unexpectedKeyword);
      LOG_ERR("expected $c, $v, or $} instead of %s", tok);
    }
    return;
  }
/* we have a label. Determine the type of statement */
  readerSkip(vrf->r, whitespace);
  keyword = readerGetToken(vrf->r, whitespace);
  if (keyword[0] != '$') {
    verifierSetError(vrf, error_expectedKeyword);
    LOG_ERR("expected a keyword after the label %s instead of %s", tok,
     keyword);
    return;
  }
  size_t len = strlen(keyword);
  if (len != 2) {
    verifierSetError(vrf, error_invalidKeyword);
    LOG_ERR("%s is not a valid keyword", keyword);
    return;
  }
  struct symstring stmt;
  symstringInit(&stmt);
  struct frame frm;
  if (keyword[1] == 'f') {
    type = symType_floating;
    verifierParseFloat(vrf, &stmt);
  } else if (keyword[1] == 'e') {
    type = symType_essential;
    verifierParseStatementContent(vrf, &stmt, &frm);
  } else if (keyword[1] == 'a') {
    struct frame frm;
    frameInit(&frm);
    type = symType_assertion;
    verifierParseAssertion(vrf, &stmt, &frm);
  } else if (keyword[1] == 'p') {
    type = symType_provable;
    /* to do */
  }
  if (vrf->err) {
    symstringClean(&stmt);
    return;
  }
  if (type == symType_none) {
    verifierSetError(vrf, error_unexpectedKeyword);
    LOG_ERR("expected $f, $e, $a, or $p instead of %s", keyword);
    return;
  }
  verifierAddSymbol(vrf, tok, type);
  symstringArrayAdd(&vrf->stmts, stmt);
}

void
verifierParseBlock(struct verifier* vrf)
{
  int isEndOfScope;
  while (1) {
    verifierParseStatement(vrf, &isEndOfScope);
    if (vrf->err) {
      break;
    }
    if (isEndOfScope) {
      vrf->scope.vals[vrf->rId]--;
      break;
    }
  }
/* deactivate local symbols in the current nesting level */
  verifierDeactivateSymbols(vrf, vrf->scope.vals[vrf->rId]);
}

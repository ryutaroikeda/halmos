#include "verifier.h"

DEFINE_ARRAY(symbol)
DEFINE_ARRAY(frame)
DEFINE_ARRAY(reader)

const size_t symbol_none_id = 0;

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
  size_tArrayClean(&frm->disjoint2);
  size_tArrayClean(&frm->disjoint1);
  size_tArrayClean(&frm->stmts);
}

int
frameAreDisjoint(const struct frame* frm, size_t v1, size_t v2)
{
  size_t i;
  for (i = 0; i < frm->disjoint1.size; i++) {
    if ((frm->disjoint1.vals[i] == v1) && (frm->disjoint2.vals[i] == v2)) {
      return 1;
    }
    if ((frm->disjoint1.vals[i] == v2) && (frm->disjoint2.vals[i] == v1)) {
      return 1;
    }
  }
  return 0;
}

void
frameAddDisjoint(struct frame* frm, size_t v1, size_t v2)
{
  size_tArrayAdd(&frm->disjoint1, v1);
  size_tArrayAdd(&frm->disjoint2, v2);
}

void
verifierInit(struct verifier* vrf)
{
  size_t i;
/* initialize symbol_none */
  symbolInit(&vrf->symbol_none);
  charArrayAppend(&vrf->symbol_none.sym, "$none", 5 + 1);
  readerArrayInit(&vrf->files, 1);
  symbolArrayInit(&vrf->symbols, 1);
  symbolArrayAdd(&vrf->symbols, vrf->symbol_none);
  symstringArrayInit(&vrf->stmts, 1);
  frameArrayInit(&vrf->frames, 1);
  for (i = 0; i < symType_size; i++) {
    symstringArrayInit(&vrf->active[i], 1);
  }
  symstringArrayInit(&vrf->stack, 1);
  size_tArrayInit(&vrf->scope, 1);
  vrf->r = NULL;
  vrf->rId = 0;
  vrf->err = error_none;
  vrf->errc = 0;
}

void
verifierClean(struct verifier* vrf)
{
  size_t i, j;
  size_tArrayClean(&vrf->scope);
  for (i = 0; i < vrf->stack.size; i++) {
    symstringClean(&vrf->stack.vals[i]);
  }
  symstringArrayClean(&vrf->stack);
  for (i = 0; i < symType_size; i++) {
    for (j = 0; j < vrf->active[i].size; j++) {
      symstringClean(&vrf->active[i].vals[j]);
    }
    symstringArrayClean(&vrf->active[i]);
  }
  for (i = 0; i < vrf->frames.size; i++) {
    frameClean(&vrf->frames.vals[i]);
  }
  frameArrayClean(&vrf->frames);
  for (i = 0; i < vrf->stmts.size; i++) {
    symstringClean(&vrf->stmts.vals[i]);
  }
  symstringArrayClean(&vrf->stmts);
  for (i = 0; i < vrf->symbols.size; i++) {
    symbolClean(&vrf->symbols.vals[i]);
  }
  symbolArrayClean(&vrf->symbols);
  for (i = 0; i < vrf->files.size; i++) {
    readerClean(&vrf->files.vals[i]);
  }
  readerArrayClean(&vrf->files);
  vrf->r = NULL;
}

// static const char tokBeginKeyword = '$';
// static const char tokEndStatement = '.';
// static const char tokEndComment = ')';
static const char whitespace[] = " \t\r\f\n";

void
verifierSetError(struct verifier* vrf, enum error err)
{
  vrf->err = err;
  vrf->errc++;
/* to do: add vrf->errorsum for more details on the error */
}

size_t
verifierGetSymId(const struct verifier* vrf, const char* sym)
{
  size_t i;
  DEBUG_ASSERT(sym, "given symbol is NULL");
  for (i = 0; i < vrf->symbols.size; i++) {
    DEBUG_ASSERT(vrf->symbols.vals[i].sym.vals, 
      "symbol from symbol table is NULL");
    if (strcmp(vrf->symbols.vals[i].sym.vals, sym) == 0) {
      return i;
    }
  }
  // verifierSetError(vrf, error_undefinedSymbol);
  // LOG_ERR("%s is not defined", sym);
  return symbol_none_id;
}

const char*
verifierGetSymName(const struct verifier* vrf, size_t symId)
{
  return vrf->symbols.vals[symId].sym.vals;
}

const char*
verifierPrintSym(const struct verifier* vrf, struct charArray* msg,
  const struct symstring* str)
{
  size_t i;
  msg->size = 0;
  for (i = 0; i < str->size; i++) {
    const char* name = verifierGetSymName(vrf, str->vals[i]);
    charArrayAppend(msg, name, strlen(name));
    charArrayAdd(msg, ' ');
  }
/* get rid of the extra space */
  if (msg->size > 0) {
    msg->size--;
  }
  charArrayAdd(msg, '\0');
  return msg->vals;
}

int
verifierIsFreshSymbol(struct verifier* vrf, const char* sym)
{
  DEBUG_ASSERT(sym, "sym is NULL");
  size_t symId = verifierGetSymId(vrf, sym);
  if (symId == symbol_none_id) {
    return 1;
  }
  if (!vrf->symbols.vals[symId].isActive) {
    return 1;
  }
  verifierSetError(vrf, error_duplicateSymbol);
  LOG_ERR("%s was used before", sym);
  return 0;
}

int
verifierIsFreshFile(const struct verifier* vrf, const char* file)
{
  DEBUG_ASSERT(file, "file is NULL");
  size_t i;
  for (i = 0; i < vrf->files.size; i++) {
    if (strcmp(readerGetFilename(&vrf->files.vals[i]), file) == 0) {
      // verifierSetError(vrf, error_duplicateFile);
      return 0;
    }
  }
  return 1;
}

int
verifierIsType(const struct verifier* vrf, size_t symId, enum symType type)
{
  DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId");
  return vrf->symbols.vals[symId].type == type;
}

void
verifierAddFileExplicit(struct verifier* vrf, struct reader* r)
{
  readerArrayAdd(&vrf->files, *r);
  size_tArrayAdd(&vrf->scope, 0);
  size_t i;
  for (i = 0; i < symType_size; i++) {
    struct symstring syms;
    symstringInit(&syms);
    symstringArrayAdd(&vrf->active[i], syms);
  }
}

void
verifierAddFile(struct verifier* vrf, struct reader* r)
{
  if (!verifierIsFreshFile(vrf, readerGetFilename(r))) {
    LOG_INFO("file %s was already added", readerGetFilename(r));
    return;
  }
  verifierAddFileExplicit(vrf, r);
}

void
verifierBeginReadingFile(struct verifier* vrf, size_t rId)
{
  DEBUG_ASSERT(rId < vrf->files.size, "%lu is an invalid file", rId);
  vrf->rId = rId;
  vrf->r = &vrf->files.vals[rId];
}

/* this does not update vrf->active. Call verifierAddSymbol instead */
size_t
verifierAddSymbolExplicit(struct verifier* vrf, const char* sym,
 enum symType type, int isActive, int isTyped, size_t scope, size_t stmt, 
 size_t frame, size_t file, size_t line, size_t offset)
{
  size_t symId = vrf->symbols.size;
  struct symbol s;
  symbolInit(&s);
  charArrayAppend(&s.sym, sym, strlen(sym));
  charArrayAdd(&s.sym, '\0');
  s.type = type;
  s.isActive = isActive;
  s.isTyped = isTyped;
  s.scope = scope;
  s.stmt = stmt;
  s.frame = frame;
  s.file = file;
  s.line = line;
  s.offset = offset;
  symbolArrayAdd(&vrf->symbols, s);
  return symId;
}

size_t
verifierAddSymbol(struct verifier* vrf, const char* sym, enum symType type)
{
  if (!verifierIsFreshSymbol(vrf, sym)) { return 0; }
  size_t symId = verifierAddSymbolExplicit(vrf, sym, type, 1, 0,
    vrf->scope.vals[vrf->rId], vrf->stmts.size, vrf->frames.size, vrf->rId,
    vrf->r->line, vrf->r->offset);
  symstringAdd(&vrf->active[type].vals[vrf->rId], symId);
  return symId;
}

size_t
verifierAddConstant(struct verifier* vrf, const char* sym)
{
  DEBUG_ASSERT(vrf->rId < vrf->files.size, "invalid file id");
  return verifierAddSymbol(vrf, sym, symType_constant);
}

size_t
verifierAddVariable(struct verifier* vrf, const char* sym)
{
  return verifierAddSymbol(vrf, sym, symType_variable);
}

size_t
verifierAddStatement(struct verifier* vrf, struct symstring* stmt)
{
  symstringArrayAdd(&vrf->stmts, *stmt);
  return vrf->stmts.size - 1;
}

/* add every pair of variables */
void
verifierAddDisjoint(struct verifier* vrf, struct symstring* stmt)
{
  size_t i, j;
  LOG_DEBUG("adding entries for each disjoint variable restriction pair");
  if (stmt->size > 0) {
    for (i = 0; i < stmt->size - 1; i++) {
      for (j = i + 1; j < stmt->size; j++) {
        size_t symId = verifierAddSymbolExplicit(vrf, "", symType_disjoint, 1,
         0, vrf->scope.vals[vrf->rId], 0, 0, vrf->rId, vrf->r->line,
         vrf->r->offset);
        symstringAdd(&vrf->active[symType_disjoint].vals[vrf->rId], symId);
        struct symstring pair;
        symstringInit(&pair);
        symstringAdd(&pair, stmt->vals[i]);
        symstringAdd(&pair, stmt->vals[j]);
        vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, &pair);
      }
    }
  }
/* clean up the stmt */
  symstringClean(stmt);
}

size_t
verifierAddFloating(struct verifier* vrf, const char* sym, 
  struct symstring* stmt)
{
  DEBUG_ASSERT(stmt->size == 2, "floating has size %lu, must be 2",
    stmt->size);
  size_t symId = verifierAddSymbol(vrf, sym, symType_floating);
  vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, stmt);
  vrf->symbols.vals[stmt->vals[1]].isTyped = 1;
  return symId;
}

size_t
verifierAddEssential(struct verifier* vrf, const char* sym,
 struct symstring* stmt)
{
  size_t symId = verifierAddSymbol(vrf, sym, symType_essential);
  vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, stmt);
  return symId;
}

size_t
verifierAddFrame(struct verifier* vrf, struct frame* frm)
{
  frameArrayAdd(&vrf->frames, *frm);
  return vrf->frames.size - 1;
}

size_t
verifierAddAssertion(struct verifier* vrf, const char* sym, 
  struct symstring* stmt)
{
  size_t symId = verifierAddSymbol(vrf, sym, symType_assertion);
  vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, stmt);
  struct frame frm; 
  frameInit(&frm);
  verifierMakeFrame(vrf, &frm, stmt);
  vrf->symbols.vals[symId].frame = verifierAddFrame(vrf, &frm);
  return symId;
}

size_t
verifierAddProvable(struct verifier* vrf, const char* sym,
  struct symstring* stmt)
{
  size_t symId = verifierAddSymbol(vrf, sym, symType_provable);
  vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, stmt);
  struct frame frm; 
  frameInit(&frm);
  verifierMakeFrame(vrf, &frm, stmt);
  vrf->symbols.vals[symId].frame = verifierAddFrame(vrf, &frm);
  return symId;
}

/* deactivate non-global symbols in the current nesting level */
void
verifierDeactivateSymbols(struct verifier* vrf)
{
  size_t i, j;
  size_t scope = vrf->scope.vals[vrf->rId];
  for (i = 0; i < symType_size; i++) {
    struct symstring* syms = &vrf->active[i].vals[vrf->rId];
    if (syms->size == 0) { continue; }
    for (j = syms->size; j > 0; j--) {
      struct symbol* sym = &vrf->symbols.vals[syms->vals[j - 1]];
      if (scope != sym->scope) { break; }
      sym->isActive = 0;
      sym->isTyped = 0;
      syms->size--;
    }
  }
}

/* get the set of variables in str */
void
verifierGetVariables(struct verifier* vrf, struct symstring* set, 
  const struct symstring* str)
{
  vrf->err = error_none;
  size_t i;
  for (i = 0; i < str->size; i++) {
    size_t sym = str->vals[i];
    DEBUG_ASSERT(sym < vrf->symbols.size, "invalid symId %lu, size is %lu",
      sym, vrf->symbols.size);
    if (vrf->symbols.vals[sym].type != symType_variable) { continue; }
    if (symstringIsIn(set, sym)) { continue; }
    symstringAdd(set, sym);
  }
}

/* make the mandatory frame for the assertion stmt */
void
verifierMakeFrame(struct verifier* vrf, struct frame* frm,
  const struct symstring* stmt)
{
  size_t i, j;
  LOG_DEBUG("adding all disjoint-variable restrictions");
  const struct symstring* syms = &vrf->active[symType_disjoint].vals[vrf->rId];
  for (i = 0; i < syms->size; i++) {
    frameAddDisjoint(frm, syms->vals[0], syms->vals[1]);
  }
/* add floating and essential hypothesis in order of appearance. */
/* The smaller symId is earlier. */
  struct symstring varset;
  symstringInit(&varset);
  verifierGetVariables(vrf, &varset, stmt);
  const struct symstring*
  floating = &vrf->active[symType_floating].vals[vrf->rId];
  const struct symstring*
  essential = &vrf->active[symType_essential].vals[vrf->rId];
  i = 0;
  j = 0;
  size_t floatingId = 0;
  size_t essentialId = 0;
/* do a 'merge sort' */
  while (i < floating->size || j < essential->size) {
/* find the next floating, if any */
    while (i < floating->size) {
      floatingId = floating->vals[i];
/* we found a mandatory hypothesis */
      if (symstringIsIn(&varset, floatingId)) { break; }
      i++;
    }
    if (j < essential->size) {
      essentialId = essential->vals[j];
    }
    if (((floatingId < essentialId) || (j >= essential->size) )
      && i < floating->size) {
      symstringAdd(&frm->stmts, floatingId);
      i++;
    } else if (((essentialId < floatingId) || (i >= floating->size))
      && j < essential->size) {
      symstringAdd(&frm->stmts, essentialId);
      j++;
    }
  }
/* check the ordering */
  if (frm->stmts.size > 0) {
    for (i = 0; i < frm->stmts.size - 1; i++) {
      DEBUG_ASSERT(frm->stmts.vals[i] < frm->stmts.vals[i + 1],
        "incorrect order of symbols in frame");
    }
  }
  LOG_DEBUG("cleaning up");
  symstringClean(&varset);
}

/* v1 and v2 are indices into sub->vars */
int
verifierIsValidDisjointPairSubstitution(struct verifier* vrf, 
  const struct frame* frm, const struct substitution* sub, size_t v1,
   size_t v2)
{
  vrf->err = error_none;
  DEBUG_ASSERT(sub->vars.size == sub->subs.size, "invalid substitution");
  DEBUG_ASSERT(v1 < sub->vars.size, "invalid variable index to substitution");
  DEBUG_ASSERT(v2 < sub->vars.size, "invalid variable index to substitution");
  size_t varId1 = sub->vars.vals[v1];
  size_t varId2 = sub->vars.vals[v2];
  DEBUG_ASSERT(varId1 < vrf->symbols.size, "invalid varId %lu", varId1);
  DEBUG_ASSERT(varId2 < vrf->symbols.size, "invalid varId %lu", varId2);
  DEBUG_ASSERT(frameAreDisjoint(frm, varId1, varId2),
    "%s and %s are not disjoint", verifierGetSymName(vrf, varId1),
    verifierGetSymName(vrf, varId2));
  struct symstring s1, s2;
  symstringInit(&s1);
  symstringInit(&s2);
/* check the substitution has no common variables */
  verifierGetVariables(vrf, &s1, &sub->subs.vals[v1]);
  verifierGetVariables(vrf, &s2, &sub->subs.vals[v2]);
  if (symstringIsIntersecting(&s1, &s2)) { 
    verifierSetError(vrf, error_invalidSubstitutionOfDisjoint);
/* to do: pretty-print the intersecting set */
    LOG_ERR("disjoint variables %s and %s share a variable in their " 
      "substitutions", verifierGetSymName(vrf, varId1), 
      verifierGetSymName(vrf, varId2));
  }
/* check each pair of variables from s1 and s2 have the disjoint variable */
/* restriction on them */
  size_t i, j;
  for (i = 0; i < s1.size; i++) {
    if (vrf->err) { break; }
    for (j = 0; j < s2.size; j++) {
      if (!frameAreDisjoint(frm, s1.vals[i], s2.vals[j])) {
        verifierSetError(vrf, error_missingDisjointRestriction);
        LOG_ERR("the variables %s and %s should be disjoint", 
          verifierGetSymName(vrf, s1.vals[i]),
          verifierGetSymName(vrf, s2.vals[j]));
        break;
      }
    }
  }
/* clean up */
  symstringClean(&s2);
  symstringClean(&s1);
  return !(vrf->err);
}

/* ensure that the substitution respects the disjoint-variable restriction */
int
verifierIsValidSubstitution(struct verifier* vrf, const struct frame* frm,
  const struct substitution* sub)
{
  vrf->err = error_none;
  if (sub->vars.size == 0) { return 1; }
  size_t i, j;
  for (i = 0; i < sub->vars.size - 1; i++) {
    for (j = i + 1; j < sub->vars.size; j++) {
      if (!frameAreDisjoint(frm, sub->vars.vals[i], sub->vars.vals[j])) { 
        continue;
      }
      if (!verifierIsValidDisjointPairSubstitution(vrf, frm, sub, i, j)) {
        return 0;
      }
    }
  }
  return 1;
}

void
verifierPushSymbol(struct verifier* vrf, size_t symId)
{
  DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId");
  const struct symbol* sym = &vrf->symbols.vals[symId];
  DEBUG_ASSERT(sym->stmt < vrf->stmts.size, "invalid statement");
  const struct symstring* stmt = &vrf->stmts.vals[sym->stmt];
  struct symstring entry;
  symstringInit(&entry);
  symstringAppend(&entry, stmt);
  symstringArrayAdd(&vrf->stack, entry);
}

/* pop the top of the stack. The caller is responsible for cleaning the */
/* returned symstring */
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

/* get substitution for floating to match a. */
void
verifierUnify(struct verifier* vrf, struct substitution* sub,
 const struct symstring* a, const struct symstring* floating)
{
  DEBUG_ASSERT(floating->size == 2, "floating has size %lu, should be 2", 
    floating->size);
  DEBUG_ASSERT(a->size >= 1, "cannot unify empty string");
  if (a->vals[0] != floating->vals[0]) {
    verifierSetError(vrf, error_mismatchedType);
    struct charArray ca1, ca2;
    charArrayInit(&ca1, 1);
    charArrayInit(&ca2, 1);
    LOG_ERR("cannot unify %s with %s", verifierPrintSym(vrf, &ca1, a),
      verifierPrintSym(vrf, &ca2, floating));
    charArrayClean(&ca1);
    charArrayClean(&ca2);
  }
  struct symstring str;
  symstringInit(&str);
/* get rid of the first constant symbol (the type symbol) */
  size_tArrayAppend(&str, &a->vals[1], a->size - 1);
  substitutionAdd(sub, floating->vals[1], &str);
}

/* use an assertion or a theorem. Pop the appropriate number of entries,  */
/* type-check, do unification and push the result */
void
verifierApplyAssertion(struct verifier* vrf, size_t symId)
{
  size_t i;
  vrf->err = error_none;
  DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId %lu", symId);
  const struct symbol* sym = &vrf->symbols.vals[symId];
  DEBUG_ASSERT(sym->frame < vrf->frames.size, "invalid frame %lu", sym->frame);
  const struct frame* frm = &vrf->frames.vals[sym->frame];
  const size_t argc = frm->stmts.size;
/* we pop into this array. The last one out is the first argument to the */
/* assertion. */
  struct symstringArray args;
  symstringArrayInit(&args, argc);
  for (i = 0; i < argc; i++) {
/* popping and adding to the array causes the order of arguments to be */
/* reversed */
    struct symstring str = verifierPop(vrf);
    if (vrf->err) { break; }
    symstringArrayAdd(&args, str);
  }
/* get the patterns to match with the arguments. */
  struct symstringArray pats;
  symstringArrayInit(&pats, 1);
  for (i = 0; i < argc; i++) {
    struct symstring str;
    symstringInit(&str);
    size_t symId = frm->stmts.vals[i];
    size_t stmtId = vrf->symbols.vals[symId].stmt;
    symstringAppend(&str, &vrf->stmts.vals[stmtId]);
    symstringArrayAdd(&pats, str);
  }
/* create the substitution by unifying $f statements */
  struct substitution sub;
  substitutionInit(&sub);
  for (i = 0; i < args.size; i++) {
    if (!verifierIsType(vrf, frm->stmts.vals[i], symType_floating)) {
      continue;
    }
/* reverse the order of args and unify */
    verifierUnify(vrf, &sub, &args.vals[args.size - 1 - i], &pats.vals[i]);
  }
/* check that the disjoint-variable restrictions are satisfied. If invalid, */
/* vrf->err will be set */
  verifierIsValidSubstitution(vrf, frm, &sub);
/* apply the substitution to $e hypotheses and check if they match the args */
  for (i = 0; i < args.size; i++) {
    if (vrf->err) { break; }
    if (!verifierIsType(vrf, frm->stmts.vals[i], symType_essential)) {
      continue;
    }
    substitutionApply(&sub, &pats.vals[i]);
    if (!symstringIsEqual(&args.vals[args.size - 1 - i], &pats.vals[i])) {
      verifierSetError(vrf, error_mismatchedEssentialHypothesis);
      struct charArray ca1, ca2;
      charArrayInit(&ca1, 1);
      charArrayInit(&ca2, 1);
      LOG_ERR("the argument %s does not match hypothesis %s",
        verifierPrintSym(vrf, &ca1, &args.vals[args.size - 1 - i]),
        verifierPrintSym(vrf, &ca2, &pats.vals[i]));
      charArrayClean(&ca1);
      charArrayClean(&ca2);
    }
  }
/* build the result to push */
  struct symstring res;
  symstringInit(&res);
  symstringAppend(&res, &vrf->stmts.vals[sym->stmt]);
  substitutionApply(&sub, &res);
  symstringArrayAdd(&vrf->stack, res);
/* clean up */
  substitutionClean(&sub);
  for (i = 0; i < pats.size; i++) {
    symstringClean(&pats.vals[i]);
  }
  symstringArrayClean(&pats);
  for (i = 0; i < args.size; i++) {
    symstringClean(&args.vals[i]);
  }
  symstringArrayClean(&args);
}

/* check all variables in the statement are typed */
/* fix me: vrf should be const vrf */
int
verifierIsTyped(struct verifier* vrf, struct symstring* stmt)
{
  size_t i;
  vrf->err = error_none;
  for (i = 0; i < stmt->size; i++) {
    const size_t symId = stmt->vals[i];
    DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId");
    const struct symbol* sym = &vrf->symbols.vals[symId];
    if (sym->type != symType_variable) { continue; }
    if (!sym->isTyped) {
      verifierSetError(vrf, error_untypedVariable);
      LOG_ERR("The symbol %s is untyped", verifierGetSymName(vrf, symId));
      return 0;
    }
  }
  return 1;
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
verifierParseSymbol(struct verifier* vrf, int* isEndOfStatement, char end)
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
    if (tok[1] == end) {
      *isEndOfStatement = 1;
    } else if (tok[1] == '(') {
      verifierParseComment(vrf);
      return verifierParseSymbol(vrf, isEndOfStatement, end);
    } else {
      verifierSetError(vrf, error_unexpectedKeyword);
      LOG_ERR("expected $%c or $( instead of %s", end, tok);
    }
    return tok;
  }
/* check for end of file */
  if (vrf->r->err == error_endOfString || vrf->r->err == error_endOfFile) {
    verifierSetError(vrf, error_unterminatedStatement);
    LOG_ERR("reached end of file before $%c", end);
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

/* parses statements (excluding proofs) of the form */
/* <constant> <symbol list>. */
/* fix me: remove definition & type checks and let the caller handle them. */
void
verifierParseStatementContent(struct verifier* vrf, struct symstring* stmt,
  char end)
{
  vrf->err = error_none;
  char* tok;
  int isEndOfStatement;
  size_t symId;
  while (1) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement, end);
    if (vrf->err) { break; }
    DEBUG_ASSERT(tok, "tok is NULL");
    if (isEndOfStatement) { break; }
    symId = verifierGetSymId(vrf, tok);
    if (symId == symbol_none_id) {
      verifierSetError(vrf, error_undefinedSymbol);
      LOG_ERR("%s was not defined", tok);
    }
    symstringAdd(stmt, symId);
  }
}

void
verifierParseConstants(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  char* tok;
  while (1) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement, '.');
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
    tok = verifierParseSymbol(vrf, &isEndOfStatement, '.');
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    verifierAddVariable(vrf, tok);
    if (vrf->err) { return; }
  }
}

void
verifierParseDisjoints(struct verifier* vrf, struct symstring* stmt)
{
  size_t i;
  verifierParseStatementContent(vrf, stmt, '.');
  verifierIsTyped(vrf, stmt);
/* all symbols must be variables */
  for (i = 0; i < stmt->size; i++) {
    if (!verifierIsType(vrf, stmt->vals[i], symType_variable)) {
      verifierSetError(vrf, error_expectedVariableSymbol);
      LOG_ERR("%s is not a variable", verifierGetSymName(vrf, stmt->vals[i]));
    }
  }
}

void 
verifierParseFloating(struct verifier* vrf, struct symstring* stmt)
{
  verifierParseStatementContent(vrf, stmt, '.');
  if (stmt->size != 2) {
    verifierSetError(vrf, error_invalidFloatingStatement);
    LOG_ERR("floating statement has size %lu, expected 2", stmt->size);
    return;
  }
  DEBUG_ASSERT(stmt->vals[0] < vrf->symbols.size, "invalid symId at vals[0]");
  if (!verifierIsType(vrf, stmt->vals[0], symType_constant)) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("%s is not a constant symbol", 
      verifierGetSymName(vrf, stmt->vals[0]));
  }
  DEBUG_ASSERT(stmt->vals[1] < vrf->symbols.size, "invalid symId at vals[1]");
  if (!verifierIsType(vrf, stmt->vals[1], symType_variable)) {
    verifierSetError(vrf, error_expectedVariableSymbol);
    LOG_ERR("%s is not a variable symbol",
      verifierGetSymName(vrf, stmt->vals[1]));
  }
}

void
verifierParseEssential(struct verifier* vrf, struct symstring* stmt)
{
  verifierParseStatementContent(vrf, stmt, '.');
  LOG_DEBUG("done parsing, checking size");
  if (stmt->size == 0) {
    verifierSetError(vrf, error_invalidEssentialStatement);
    LOG_ERR("essential statements must at least contain a constant symbol");
    return;
  }
  LOG_DEBUG("checking first symbol is a constant");
  DEBUG_ASSERT(stmt->vals[0] < vrf->symbols.size, "invalid symId");
  if (!verifierIsType(vrf, stmt->vals[0], symType_constant)) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("%s is not a constant symbol",
      verifierGetSymName(vrf, stmt->vals[0]));
  }
  LOG_DEBUG("checking variables are typed");
  verifierIsTyped(vrf, stmt);
}

void
verifierParseAssertion(struct verifier* vrf, struct symstring* stmt)
{
  verifierParseStatementContent(vrf, stmt, '.');
  if (stmt->size == 0) {
    verifierSetError(vrf, error_invalidAssertionStatement);
    LOG_ERR("an assertion must have at least a constant symbol");
    return;
  }
  if (!verifierIsType(vrf, stmt->vals[0], symType_constant)) {
    verifierSetError(vrf, error_expectedConstantSymbol);
    LOG_ERR("%s is not a constant symbol",
      verifierGetSymName(vrf, stmt->vals[0]));
  }
  verifierIsTyped(vrf, stmt);
}

void
verifierParseProofSymbol(struct verifier* vrf, int* isEndOfProof)
{
  char* tok;
  vrf->err = error_none;
  *isEndOfProof = 0;
  tok = verifierParseSymbol(vrf, isEndOfProof, '.');
  if (*isEndOfProof) {
    return;
  }
  size_t symId = verifierGetSymId(vrf, tok);
  if (symId == symbol_none_id) { 
    verifierSetError(vrf, error_undefinedSymbol);
    LOG_ERR("%s was not defined", tok);
    return;
  }
  const struct symbol* sym = &vrf->symbols.vals[symId];
  if ((sym->type == symType_floating) 
    || (sym->type == symType_essential)) {
/* push floating or essential on the stack */
    verifierPushSymbol(vrf, symId);
  } else if ((sym->type == symType_provable) 
    || (sym->type == symType_assertion)) {
    verifierApplyAssertion(vrf, symId);
  } else {   
    verifierSetError(vrf, error_invalidSymbolInProof);
    LOG_ERR("symbols in proofs must be $f, $e, $a, or $p");
  }
}

/* thm is the theorem to prove */
void
verifierParseProof(struct verifier* vrf, const struct symstring* thm)
{
  vrf->err = error_none;
  int isEndOfProof = 0;
  while (!vrf->err && !isEndOfProof) {
    verifierParseProofSymbol(vrf, &isEndOfProof);
  }
  if (vrf->stack.size > 1) {
    verifierSetError(vrf, error_unusedTermInProof);
/* to do: show which terms are unused */
    LOG_ERR("the proof contains unused terms");
  }
  if (vrf->stack.size == 0) {
    verifierSetError(vrf, error_incorrectProof);
    LOG_ERR("the proof is empty");
  } else if (!symstringIsEqual(&vrf->stack.vals[0], thm)) {
    verifierSetError(vrf, error_incorrectProof);
/* to do: show why the proof is wrong */
    LOG_ERR("the proof is wrong");
  }
}

void
verifierParseProvable(struct verifier* vrf, struct symstring* stmt)
{
  verifierParseStatementContent(vrf, stmt, '=');
  verifierIsTyped(vrf, stmt);
  verifierParseProof(vrf, stmt);
}

/* parse $f, $e, $a, or $p after the label */
/* to do: error check on $f and handle case when stmt.size != 2, otherwise */
/* it will trigger an assert */
void
verifierParseLabelledStatement(struct verifier* vrf, const char* tok)
{
  vrf->err = error_none;
  readerSkip(vrf->r, whitespace);
  char* keyword = readerGetToken(vrf->r, whitespace);
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
  if (keyword[1] == '(') {
    verifierParseComment(vrf);
    verifierParseLabelledStatement(vrf, tok);
    return;
  }
  enum symType type = symType_none;
  struct symstring stmt;
  symstringInit(&stmt);
  if (keyword[1] == 'f') {
    type = symType_floating;
    verifierParseFloating(vrf, &stmt);
    verifierAddFloating(vrf, tok, &stmt);
  } else if (keyword[1] == 'e') {
    type = symType_essential;
    verifierParseEssential(vrf, &stmt);
    verifierAddEssential(vrf, tok, &stmt);
  } else if (keyword[1] == 'a') {
    type = symType_assertion;
    verifierParseAssertion(vrf, &stmt);
    verifierAddAssertion(vrf, tok, &stmt);
  } else if (keyword[1] == 'p') {
    type = symType_provable;
    verifierParseProvable(vrf, &stmt);
    verifierAddProvable(vrf, tok, &stmt);
  }
  if (type == symType_none) {
    verifierSetError(vrf, error_unexpectedKeyword);
    LOG_ERR("expected $f, $e, $a, or $p instead of %s", keyword);
  }
}

void
verifierParseStatement(struct verifier* vrf, int* isEndOfScope)
{
  vrf->err = error_none;
  char* tok;
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
      struct symstring stmt;
      symstringInit(&stmt);
      verifierParseDisjoints(vrf, &stmt);
      verifierAddDisjoint(vrf, &stmt);
    } else if (tok[1] == '{') {
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
/* we have a label */
  verifierParseLabelledStatement(vrf, tok);
}

void
verifierParseBlock(struct verifier* vrf)
{
  int isEndOfScope;
  vrf->scope.vals[vrf->rId]++;
  while (!vrf->err) {
    verifierParseStatement(vrf, &isEndOfScope);
    if (isEndOfScope) {
      break;
    }
  }
/* deactivate local symbols in the current nesting level */
  verifierDeactivateSymbols(vrf);
/* go back to the previous nesting level */
  vrf->scope.vals[vrf->rId]--;
}

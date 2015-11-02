#include "verifier.h"
#include "logger.h"

DEFINE_ARRAY(symbol)
DEFINE_ARRAY(frame)

const char* symTypeStrings[symType_size] = {
  "none",
  "constant",
  "variable",
  "disjoint",
  "floating",
  "essential",
  "assertion",
  "provable"
};

const char*
symTypeString(enum symType type)
{
  return symTypeStrings[type];
}

const size_t symbol_none_id = 0;
const size_t file_none_id = 0;

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
  symbolArrayInit(&vrf->symbols, 1);
  /* add symbol_none */
  symbolInit(&vrf->symbol_none);
  charArrayAppend(&vrf->symbol_none.sym, "$none", 5 + 1);
  symbolArrayAdd(&vrf->symbols, vrf->symbol_none);
  symstringArrayInit(&vrf->stmts, 1);
  frameArrayInit(&vrf->frames, 1);
  symstringInit(&vrf->disjoints);
  symstringInit(&vrf->hypotheses);
  symstringInit(&vrf->variables);
  symstringArrayInit(&vrf->stack, 1);
  charstringArrayInit(&vrf->files, 1);
/* add 'none' file */
  charstringInit(&vrf->file_none);
/* henceforth, no file is allowed to have the name " " */
  charArrayAppend(&vrf->file_none, " ", 1 + 1);
  charstringArrayAdd(&vrf->files, vrf->file_none);
  vrf->rId = file_none_id;
  vrf->scope = 0;
  for (i = 0; i < symType_size; i++) {
    vrf->symCount[i] = 0;
  }
  vrf->err = error_none;
  vrf->errc = 0;
}

void
verifierClean(struct verifier* vrf)
{
  size_t i;
  for (i = 0; i < vrf->files.size; i++) {
    charstringClean(&vrf->files.vals[i]);
  }
  charstringArrayClean(&vrf->files);
  for (i = 0; i < vrf->stack.size; i++) {
    symstringClean(&vrf->stack.vals[i]);
  }
  symstringArrayClean(&vrf->stack);
  symstringClean(&vrf->variables);
  symstringClean(&vrf->hypotheses);
  symstringClean(&vrf->disjoints);
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
  vrf->r = NULL;
}

void
verifierEmptyStack(struct verifier* vrf)
{
  size_t i;
  for (i = 0; i < vrf->stack.size; i++) {
    symstringClean(&vrf->stack.vals[i]);
  }
  symstringArrayEmpty(&vrf->stack);
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
    const struct symbol* s = &vrf->symbols.vals[i];
    DEBUG_ASSERT(s->sym.vals, 
      "symbol from symbol table is NULL");
    if ((strcmp(s->sym.vals, sym) == 0) && s->isActive) {
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

size_t
verifierGetFileId(const struct verifier* vrf, const char* file)
{
  DEBUG_ASSERT(file, "file is NULL");
  size_t i;
  for (i = 0; i < vrf->files.size; i++) {
    if (strcmp(vrf->files.vals[i].vals, file) == 0) {
      return i;
    }
  }
/* reserved value */
  return 0;
}

int
verifierIsType(const struct verifier* vrf, size_t symId, enum symType type)
{
  DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId");
  return vrf->symbols.vals[symId].type == type;
}

size_t
verifierPreprocAddFile(struct verifier* vrf, const char* filename)
{
  size_t fid = verifierGetFileId(vrf, filename);
/* did we already add the file? */
  if (fid != file_none_id) { return fid; }
/* we add the new file */
  size_t len = strlen(filename) + 1;
  struct charstring f;
  charArrayInit(&f, len);
  charArrayAppend(&f, filename, len);
  charstringArrayAdd(&vrf->files, f);
  return vrf->files.size - 1;
}

void
verifierPreprocBeginReading(struct verifier* vrf, size_t rId, size_t line)
{
  DEBUG_ASSERT(rId < vrf->files.size, "%lu is an invalid file", rId);
  vrf->rId = rId;
/* reset the line number and offset */
  vrf->r->line = line;
  vrf->r->offset = 0;
}

/* check the label contains only alphanumeric characters and -, _, and .. */
int
verifierIsValidLabel(const struct verifier* vrf, const char* lab)
{
  const char* valid = "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
  (void) vrf;
  size_t i;
  const size_t len = strlen(lab);
  for (i = 0; i < len; i++) {
    if (!strchr(valid, lab[i])) { return 0; }
  }
  return 1;
}

size_t
verifierAddSymbolExplicit(struct verifier* vrf, const char* sym,
 enum symType type, int isActive, int isTyped, size_t scope, size_t stmt, 
 size_t frame, size_t file, size_t line, size_t offset)
{
  size_t symId = vrf->symbols.size;
  struct symbol s;
  symbolInit(&s);
  charArrayAppend(&s.sym, sym, strlen(sym) + 1);
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
  vrf->symCount[type]++;
  return symId;
}

size_t
verifierAddSymbol(struct verifier* vrf, const char* sym, enum symType type)
{
/* check if the symbol is fresh */
  size_t testId = verifierGetSymId(vrf, sym);
  if (testId != symbol_none_id) { 
    verifierSetError(vrf, error_duplicateSymbol);
    LOG_ERR("%s was used before", sym);
    return symbol_none_id;
  }
/* symbol names cannot contain $ */
  if (strchr(sym, '$')) {
    verifierSetError(vrf, error_invalidSymbol);
    LOG_ERR("%s contains $", sym);
    return symbol_none_id;
  }
  if ((type == symType_floating) || (type == symType_essential)
    || (type == symType_assertion) || (type == symType_provable)) {
    if (!verifierIsValidLabel(vrf, sym)) {
      H_LOG_ERR(vrf, error_invalidLabel,
        "label %s contains an invalid character", sym);
      return symbol_none_id;
    }
  }
  size_t symId = verifierAddSymbolExplicit(vrf, sym, type, 1, 0,
    vrf->scope, vrf->stmts.size, vrf->frames.size, vrf->rId,
    vrf->r->line, vrf->r->offset);
/* for $d, $f, $e, or $v, update the active list */
  if (type == symType_disjoint) {
    symstringAdd(&vrf->disjoints, symId);
  } else if (type == symType_floating || type == symType_essential) {
    symstringAdd(&vrf->hypotheses, symId);
  } else if (type == symType_variable) {
    symstringAdd(&vrf->variables, symId);
  }
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
         0, vrf->scope, 0, 0, vrf->rId, vrf->r->line,
         vrf->r->offset);
        symstringAdd(&vrf->disjoints, symId);
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
  size_t symId = verifierAddSymbol(vrf, sym, symType_floating);
  vrf->symbols.vals[symId].stmt = verifierAddStatement(vrf, stmt);
  if (stmt->size >= 2) {
    vrf->symbols.vals[stmt->vals[1]].isTyped = 1;
  }
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
  size_t i;
  size_t scope = vrf->scope;
  struct symstring* disjoints = &vrf->disjoints;
  for (i = disjoints->size; i > 0; i--) {
    struct symbol* sym = &vrf->symbols.vals[disjoints->vals[i - 1]];
    if (sym->scope < scope) { break; }
    sym->isActive = 0;
    disjoints->size--;
  }
  struct symstring* hypotheses = &vrf->hypotheses;
  for (i = hypotheses->size; i > 0; i--) {
    struct symbol* sym = &vrf->symbols.vals[hypotheses->vals[i - 1]];
    if (sym->scope < scope) { break; }
    if (sym->type == symType_floating) {
/* untype the variable referenced by the floating hypothesis */
      size_t var = vrf->stmts.vals[sym->stmt].vals[1];
      vrf->symbols.vals[var].isTyped = 0;
    }
    sym->isActive = 0;
    hypotheses->size--;
  }
  struct symstring* variables = &vrf->variables;
  for (i = variables->size; i > 0; i--) {
    struct symbol* sym = &vrf->symbols.vals[variables->vals[i - 1]];
    if (sym->scope < scope) { break; }
    sym->isActive = 0;
    variables->size--;
  }
}

/* add the set of variables in str to set */
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

/* make the mandatory frame for the assertion stmt. frm->stmts will be in */
/* reverse order i.e. frm->stmts.vals[0] will be the last item */
void
verifierMakeFrame(struct verifier* vrf, struct frame* frm,
  const struct symstring* stmt)
{
  size_t i;
/* add disjoints */
  struct symstring* disjoints = &vrf->disjoints;
  for (i = 0; i < disjoints->size; i++) {
    struct symbol* disjoint = &vrf->symbols.vals[disjoints->vals[i]];
    struct symstring* pair = &vrf->stmts.vals[disjoint->stmt];
    frameAddDisjoint(frm, pair->vals[0], pair->vals[1]);
  }
/* the set of mandatory variables */
  struct symstring varset;
  symstringInit(&varset);
/* add the variables referenced in the assertion */
  verifierGetVariables(vrf, &varset, stmt);
/* look at the hypotheses in reverse order (latest first) */
  struct symstring* hypotheses = &vrf->hypotheses;
  for (i = hypotheses->size; i > 0; i--) {
    size_t symId = hypotheses->vals[i - 1];
    DEBUG_ASSERT(symId < vrf->symbols.size, "invalid symId");
    struct symbol* hypothesis = &vrf->symbols.vals[symId];
    if (hypothesis->type == symType_floating) {
/* get the variable symbol of the floating */
      size_t varId = vrf->stmts.vals[hypothesis->stmt].vals[1];
      if (symstringIsIn(&varset, varId)) {
/* we found a mandatory hypothesis */
        symstringAdd(&frm->stmts, symId);
      }
    } else if (hypothesis->type == symType_essential) {
/* add variables referenced in the hypothesis to varset */
      verifierGetVariables(vrf, &varset, &vrf->stmts.vals[hypothesis->stmt]);
      symstringAdd(&frm->stmts, symId);
    }
  }
  symstringClean(&varset);
}

/* v1 and v2 are indices into sub->vars */
/* From the metamath specification: */
/* */
/* Each substitution made in a proof must be checked to verify that any */
/* disjoint variable restrictions are satisfied, as follows. */
/* If two variables replaced by a substitution exist in a mandatory $d */
/* statement of the assertion referenced, the two expressions resulting */
/* from the substitution must meet satisfy the following conditions. */
/* First, the two ex- pressions must have no variables in common. Second, */
/* each possible pair of variables, one from each expression, must exist in */
/* an active $d statement of the $p statement containing the proof. */
/* */
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
  /* to do: pretty-print the intersecting set */
    H_LOG_ERR(vrf, error_invalidSubstitutionOfDisjoint,
      "disjoint variables %s and %s share a variable in their " 
      "substitutions", verifierGetSymName(vrf, varId1), 
      verifierGetSymName(vrf, varId2));
  }
/* don't do this if there already was an error above. */
/* Check each pair of variables from s1 and s2 have the disjoint variable */
/* restriction on them */
  size_t i, j;
  for (i = 0; i < s1.size; i++) {
    if (vrf->err) { break; }
    for (j = 0; j < s2.size; j++) {
      if (!frameAreDisjoint(frm, s1.vals[i], s2.vals[j])) {
        H_LOG_ERR(vrf, error_missingDisjointRestriction,
        "the variables %s and %s should be disjoint", 
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
/* to do: ... in proof of what? */
    H_LOG_ERR(vrf, error_stackUnderflow,
      "stack is empty");
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
  if (floating->size != 2) {
    struct charArray ca;
    charArrayInit(&ca, 1);
    H_LOG_ERR(vrf, error_invalidFloatingStatement,
     "cannot unify with an invalid floating statement %s",
     verifierPrintSym(vrf, &ca, floating));
    return;
  }
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

/* use an assertion or a theorem. Pop the appropriate number of entries, */
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
/* note: popping and adding to the array causes the order of arguments to be */
/* reversed */
    struct symstring str = verifierPop(vrf);
    if (vrf->err) { break; }
    symstringArrayAdd(&args, str);
  }
/* get the patterns to match with the arguments, i.e. the mandatory */
/* hypotheses of the assertion */
/* note: frm->stmts are in reverse order */
  struct symstringArray pats;
  symstringArrayInit(&pats, 1);
  for (i = 0; i < argc; i++) {
    struct symstring str;
    symstringInit(&str);
    size_t frmSymId = frm->stmts.vals[i];
    size_t stmtId = vrf->symbols.vals[frmSymId].stmt;
    symstringAppend(&str, &vrf->stmts.vals[stmtId]);
    symstringArrayAdd(&pats, str);
  }
/* create the substitution by unifying $f statements */
  struct substitution sub;
  substitutionInit(&sub);
  for (i = 0; i < args.size; i++) {
    if (!verifierIsType(vrf, frm->stmts.vals[argc - 1 - i],
      symType_floating)) {
      continue;
    }
/* reverse the order of args and pats, then unify */
    verifierUnify(vrf, &sub, &args.vals[args.size - 1 - i],
     &pats.vals[argc - 1 - i]);
  }
/* check that the disjoint-variable restrictions are satisfied. If invalid, */
/* vrf->err will be set */
  verifierIsValidSubstitution(vrf, frm, &sub);
/* apply the substitution to $e hypotheses and check if they match the args */
  for (i = 0; i < args.size; i++) {
    if (vrf->err) { break; }
    if (!verifierIsType(vrf, frm->stmts.vals[argc - 1 - i],
      symType_essential)) {
      continue;
    }
    substitutionApply(&sub, &pats.vals[argc - 1 - i]);
    if (!symstringIsEqual(&args.vals[args.size - 1 - i],
      &pats.vals[argc - 1 - i])) {
      struct charArray ca1, ca2;
      charArrayInit(&ca1, 1);
      charArrayInit(&ca2, 1);
      H_LOG_ERR(vrf, error_mismatchedEssentialHypothesis, 
        "the argument %s does not match hypothesis %s",
        verifierPrintSym(vrf, &ca1, &args.vals[args.size - 1 - i]),
        verifierPrintSym(vrf, &ca2, &pats.vals[argc - 1 - i]));
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
    } else {
      H_LOG_ERR(vrf, error_unexpectedKeyword,
        "expected $%c instead of %s", end, tok);
    }
    return tok;
  }
/* check for end of file */
  if (vrf->r->err == error_endOfString || vrf->r->err == error_endOfFile) {
    verifierSetError(vrf, error_unterminatedStatement);
    LOG_ERR("reached end of file before $%c", end);
    return tok;
  }
  return tok;
}

/* Parse the comment of the form '$( filename line $)' written by the */
/* preprocessor to indicate change of file */
void
verifierParsePreprocFile(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement = 0;
/* get the filename */
  char* tok = verifierParseSymbol(vrf, &isEndOfStatement, ')');
  if (vrf->r->err) { return; }
  if (isEndOfStatement) {
/* fix me: should we keep track of the file line of the raw preprocessed file to */
/* report errors like this? */
    H_LOG_ERR(vrf, error_expectedFilename, "a filename must follow $(");
    return;
  }
  size_t rId = verifierPreprocAddFile(vrf, tok);
  tok = verifierParseSymbol(vrf, &isEndOfStatement, ')');
  if (vrf->r->err) { return; }
  if (isEndOfStatement) {
    H_LOG_ERR(vrf, error_expectedLineNumber,
      "a line number must follow filename");
  }
/* convert the string to a number */
/* fix me: do error checking? */
  size_t line = strtoul(tok, NULL, 10);
  verifierPreprocBeginReading(vrf, rId, line);
/* go to the end of the comment, ignoring anything else without reporting */
  while (1) {
    readerFind(vrf->r, "$");
    tok = readerGetToken(vrf->r, whitespace);
    if (vrf->r->err) { break; }
    if (strlen(tok) != 2) { continue; }
    if (tok[1] == ')') { break; }
  }
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
  while (!vrf->err) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement, '.');
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    verifierAddConstant(vrf, tok);
  }
}

void
verifierParseVariables(struct verifier* vrf)
{
  vrf->err = error_none;
  int isEndOfStatement;
  char* tok;
  while (!vrf->err) {
    tok = verifierParseSymbol(vrf, &isEndOfStatement, '.');
    if (vrf->err) { return; }
    if (isEndOfStatement) { break; }
    verifierAddVariable(vrf, tok);
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
    H_LOG_ERR(vrf, error_invalidFloatingStatement,
      "invalid floating statement has size %lu, expected 2", stmt->size);
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
  if (vrf->err) { return; }
  if (*isEndOfProof) { return; }
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
    H_LOG_ERR(vrf, error_invalidSymbolInProof,
      "%s is %s statement", verifierGetSymName(vrf, symId),
      symTypeString(sym->type));
  }
}

/* thm is the theorem to prove */
void
verifierParseProof(struct verifier* vrf, const struct symstring* thm)
{
  vrf->err = error_none;
  int isEndOfProof = 0;
  verifierEmptyStack(vrf);
  while (!vrf->err && !isEndOfProof) {
    verifierParseProofSymbol(vrf, &isEndOfProof);
  }
  if (vrf->stack.size > 1) {
/* to do: show which terms are unused */
    H_LOG_ERR(vrf, error_unusedTermInProof,
      "the proof contains unused terms");
  }
  if (vrf->stack.size == 0) {
    H_LOG_ERR(vrf, error_incorrectProof, "the proof is empty");
  } else if (!symstringIsEqual(&vrf->stack.vals[0], thm)) {
    struct charArray res, theorem;
    charArrayInit(&res, 1);
    charArrayInit(&theorem, 1);
    H_LOG_ERR(vrf, error_incorrectProof,
      "%s was derived but the proof requires %s",
      verifierPrintSym(vrf, &res, &vrf->stack.vals[0]),
      verifierPrintSym(vrf, &theorem, thm));
    charArrayClean(&res);
    charArrayClean(&theorem);
  }
}

void
verifierParseProvable(struct verifier* vrf, struct symstring* stmt)
{
  verifierParseStatementContent(vrf, stmt, '=');
  verifierIsTyped(vrf, stmt);
  verifierParseProof(vrf, stmt);
}

/* parse $c, $v, or $d statements, or a ${ block. */
void
verifierParseUnlabelledStatement(struct verifier* vrf, int* isEndOfScope,
  const char* tok)
{
  size_t len = strlen(tok);
  if (len != 2) {
    H_LOG_ERR(vrf, error_invalidKeyword, "%s is not a keyword", tok);
    return;
  }
  if (tok[1] == '(') {
/* this is a special comment left by the preprocessor to indicate change of */
/* filename */
    verifierParsePreprocFile(vrf);
  } else if (tok[1] == 'c') {
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
  } else {
    H_LOG_ERR(vrf, error_unexpectedKeyword,
      "expected $c, $v, $d, ${, or $} instead of %s", tok);
  }
}

/* parse $f, $e, $a, or $p after the label */
/* to do: error check on $f and handle case when stmt.size != 2, otherwise */
/* it will trigger an assert */
/* to do: validate label token */
void
verifierParseLabelledStatement(struct verifier* vrf, const char* tok)
{
  vrf->err = error_none;
  readerSkip(vrf->r, whitespace);
  char* keyword = readerGetToken(vrf->r, whitespace);
  if (vrf->r->err) {
    H_LOG_ERR(vrf, error_expectedKeyword,
      "expected keyword after label %s", tok);
    return;
  }
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
  enum symType type = symType_none;
  if (keyword[1] == 'f') {
    type = symType_floating;
    struct symstring stmt;
    symstringInit(&stmt);
    verifierParseFloating(vrf, &stmt);
    verifierAddFloating(vrf, tok, &stmt);
  } else if (keyword[1] == 'e') {
    type = symType_essential;
    struct symstring stmt;
    symstringInit(&stmt);
    verifierParseEssential(vrf, &stmt);
    verifierAddEssential(vrf, tok, &stmt);
  } else if (keyword[1] == 'a') {
    type = symType_assertion;
    struct symstring stmt;
    symstringInit(&stmt);
    verifierParseAssertion(vrf, &stmt);
    verifierAddAssertion(vrf, tok, &stmt);
  } else if (keyword[1] == 'p') {
    type = symType_provable;
    struct symstring stmt;
    symstringInit(&stmt);
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
/* the beginning of the statement. Get the keyword or the label */
  readerSkip(vrf->r, whitespace);
  tok = readerGetToken(vrf->r, whitespace);
/* check if we are at the end of file */
  if (vrf->r->err) {
    *isEndOfScope = 1;
/* the file must end with a new line character, except for empty files. */
/* note: strchr(s, 0) is always true. vrf->r->last is initialized to 0, so */
/* empty files shouldn't raise this error */
    if (!strchr("\n\f", vrf->r->last)) {
      H_LOG_ERR(vrf, error_expectedNewLine,
        "expected new line at the end of file");
    }
    return;
  }
  if (tok[0] == '$') {
/* we have a keyword */
    verifierParseUnlabelledStatement(vrf, isEndOfScope, tok);
  } else {
/* we have a label */
/* copy the label, because reader will change tok */
    struct charArray key;
    charArrayInit(&key, strlen(tok) + 1);
/* to do: this is error prone. Define a charstring struct */
    charArrayAppend(&key, tok, strlen(tok) + 1);
    verifierParseLabelledStatement(vrf, key.vals);
    charArrayClean(&key);
  }
}

void
verifierParseBlock(struct verifier* vrf)
{
  int isEndOfScope = 0;
  vrf->scope++;
  while (!isEndOfScope) {
    verifierParseStatement(vrf, &isEndOfScope);
  }
/* deactivate local symbols in the current nesting level */
  verifierDeactivateSymbols(vrf);
/* go back to the previous nesting level */
  vrf->scope--;
}

void
verifierBeginReadingFile(struct verifier* vrf, struct reader* r)
{
  vrf->r = r;
}

/* to do: have an output file, for compressed proofs */
void
verifierCompile(struct verifier* vrf, const char* in)
{
  FILE* fin = fopen(in, "r");
  if (!fin) {
    G_LOG_ERR(vrf, error_failedFileOpen, "failed to open input file %s", in);
    return;
  }
  struct reader r;
  readerInitFile(&r, fin, in);
  verifierBeginReadingFile(vrf, &r);
  verifierParseBlock(vrf);
  readerClean(&r);
  fclose(fin);
}

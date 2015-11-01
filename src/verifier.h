#ifndef _HALMOSVERIFIER_H_
#define _HALMOSVERIFIER_H_
#include "array.h"
#include "error.h"
#include "reader.h"
#include "symstring.h"

struct symbol;
typedef struct symbol symbol;
DECLARE_ARRAY(symbol)
struct frame;
typedef struct frame frame;
DECLARE_ARRAY(frame)
typedef struct reader reader;
DECLARE_ARRAY(reader)

enum symType {
  symType_none,
  symType_constant,
  symType_variable,
  symType_disjoint,
  symType_floating,
  symType_essential,
  symType_assertion,
  symType_provable,
  symType_size
};

const char* symTypeString(enum symType type);

struct symbol {
  struct charArray sym;
  enum symType type;
/* 1 if the symbol is currently in scope */
/* used for checking freshness */
  int isActive;
/* 1 if the symbol is a variable and typed through $f */
  int isTyped;
/* the nesting level */
  size_t scope;
/* for $f, $e, $a, and $p statements */
  size_t stmt;
/* for $a and $p assertions */
  size_t frame;
/* index to verifier->files, which is an array of readers */
  size_t file; 
  size_t line; 
  size_t offset; 
};

void symbolInit(struct symbol* sym);

void symbolClean(struct symbol* sym);

// struct statement {
/* indices to verifier->symbols */
  // struct symstring syms;
/* 1 if it is a mandatory hypothesis */
/* fix me: how does this work? This should be in frame */
/* int isMandatory; */
// };

// void statementInit(struct statement* stmt);

// void statementClean(struct statement* stmt);

/* frame */
struct frame {
/* indices to verifier->stmts. These are mandatory hypotheses */
  struct size_tArray stmts;
/* indices to verifier->symbols for pairwise disjoint variables */
  struct size_tArray disjoint1;
  struct size_tArray disjoint2;
};

void
frameInit(struct frame* frm);

void
frameClean(struct frame* frm);

void
frameAddDisjoint(struct frame* frm, size_t v1, size_t v2);

int
frameAreDisjoint(const struct frame* frm, size_t v1, size_t v2);

extern const size_t symbol_none_id;

struct verifier {
/* a special symbol with symId 0 */
  struct symbol symbol_none;
/* files opened for verification */
  struct readerArray files;
  struct symbolArray symbols;
  struct symstringArray stmts;
  struct frameArray frames;
/* disjoint symbols currently in scope, for each open file */
  struct symstringArray disjoints;
/* floating and essential hypotheses currently in scope, for each open file */
  struct symstringArray hypotheses;
/* variables currently in scope, for each open file */
  struct symstringArray variables;
/* reverse polish notation stack for verifying proofs */
  struct symstringArray stack;
/* the file currently being verified */
  struct reader* r;
/* nesting level for each file */
  struct size_tArray scope;
/* count of symbols parsed */
  size_t symCount[symType_size];
/* index of the current file */
  size_t rId;
  enum error err;
/* the number of errors reported */
  size_t errc;
/* to do: have a dynamic array of errors */
};

void
verifierInit(struct verifier* vrf);

void
verifierClean(struct verifier* vrf);

void
verifierEmptyStack(struct verifier* vrf);

size_t
verifierGetSymId(const struct verifier* vrf, const char* sym);

size_t
verifierAddFileExplicit(struct verifier* vrf, struct reader* r);

size_t
verifierAddFile(struct verifier* vrf, struct reader* r);

void
verifierBeginReadingFile(struct verifier* vrf, size_t rId);

/* return the symId of the symbol added */
size_t
verifierAddSymbolExplicit(struct verifier* vrf, const char* sym,
 enum symType type, int isActive, int isTyped, size_t scope, size_t stmt, 
 size_t frame, size_t file, size_t line, size_t offset);

size_t
verifierAddSymbol(struct verifier* vrf, const char* sym, enum symType type);

size_t
verifierAddConstant(struct verifier* vrf, const char* sym);

size_t
verifierAddVariable(struct verifier* vrf, const char* sym);

/* return the id of the statement */
size_t
verifierAddStatement(struct verifier* vrf, struct symstring* stmt);

/* add every pair of variables in stmt as a disjoint variable restriction  */
void
verifierAddDisjoint(struct verifier* vrf, struct symstring* stmt);

size_t
verifierAddFloating(struct verifier* vrf, const char* sym, 
  struct symstring* stmt);

size_t
verifierAddEssential(struct verifier* vrf, const char* sym,
  struct symstring* stmt);

/* return the id of the frame */
size_t
verifierAddFrame(struct verifier* vrf, struct frame* frm);

size_t
verifierAddAssertion(struct verifier* vrf, const char* sym,
  struct symstring* stmt);

size_t
verifierAddProvable(struct verifier* vrf, const char* sym,
  struct symstring* stmt);

void
verifierDeactivateSymbols(struct verifier* vrf);

void
verifierGetVariables(struct verifier* vrf, struct symstring* set,
  const struct symstring* stmt);

void
verifierMakeFrame(struct verifier* vrf, struct frame* frm, 
  const struct symstring* stmt);

int
verifierIsValidDisjointPairSubstitution(struct verifier* vrf,
 const struct frame* frm, const struct substitution* sub, size_t v1,
 size_t v2);

int
verifierIsValidSubstitution(struct verifier* vrf, const struct frame* frm,
  const struct substitution* sub);

void
verifierUnify(struct verifier* vrf, struct substitution* sub, 
  const struct symstring* a, const struct symstring* floating);

void
verifierApplyAssertion(struct verifier* vrf, size_t symId);

char*
verifierParseSymbol(struct verifier* vrf, int* isEndOfStatement, char end);

void
verifierParseStatementContent(struct verifier* vrf, struct symstring* stmt,
  char end);

void 
verifierParseConstants(struct verifier* vrf);

void
verifierParseVariables(struct verifier* vrf);

void
verifierParseDisjoints(struct verifier* vrf, struct symstring* stmt);

/* stmt must be initialized */
void
verifierParseFloating(struct verifier* vrf, struct symstring* stmt);

void
verifierParseEssential(struct verifier* vrf, struct symstring* stmt);

void
verifierParseAssertion(struct verifier* vrf, struct symstring* stmt);

void
verifierParseProofSymbol(struct verifier* vrf, int* isEndOfProof);

void
verifierParseProof(struct verifier* vrf, const struct symstring* thm);

void
verifierParseProvable(struct verifier* vrf, struct symstring* stmt);

void
verifierParseFileInclusion(struct verifier* vrf);

void
verifierParseUnlabelledStatement(struct verifier* vrf, int* isEndOfScope,
 const char* tok);

void
verifierParseLabelledStatement(struct verifier* vrf, const char* tok);

void
verifierParseStatement(struct verifier* vrf, int* isEndOfScope);

void
verifierParseBlock(struct verifier* vrf);

void
verifierParseFileExplicit(struct verifier* vrf, const char* filename,
  const char* mode);

void
verifierParseFile(struct verifier* vrf, const char* filename);

#endif

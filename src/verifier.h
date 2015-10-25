#ifndef _HALMOSVERIFIER_H_
#define _HALMOSVERIFIER_H_
#include "array.h"
#include "error.h"
#include "reader.h"
#include "symstring.h"

struct symbol;
typedef struct symbol symbol;
DECLARE_ARRAY(symbol)
/*
struct statement;
typedef struct statement statement;
DECLARE_ARRAY(statement)
*/
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

struct symbol {
  struct charArray sym;
  enum symType type;
/* 1 if the symbol is currently in scope */
  int isActive;
/* 1 if the symbol is a variable and typed through $f */
  int isTyped;
/* the nesting level */
  size_t scope;
/* index to verifier->stmts and verifier->frames. Only valid when type is */
/* floating, essential, assertion, or provable (or disjoint) */
  size_t stmt;
  size_t frame;
/* index to verifier->files, which is an array of readers */
  size_t file; 
  size_t line; 
  size_t offset; 
};

void symbolInit(struct symbol* sym);

void symbolClean(struct symbol* sym);

char* symbolGetName(struct symbol* sym);

struct statement {
/* indices to verifier->symbols */
  struct symstring syms;
/* 1 if it is a mandatory hypothesis */
/* fix me: how does this work? This should be in frame */
/* int isMandatory; */
};

void statementInit(struct statement* stmt);

void statementClean(struct statement* stmt);

/* extended frame */
struct frame {
/* indices to verifier->stmts */
  struct size_tArray stmts;
/* indices to verifier->symbols */
/* or treat disjoints as statements too? */
/*
  struct size_tArray disjoint1;
  struct size_tArray disjoint2;
*/
};

void frameInit(struct frame* frm);

void frameClean(struct frame* frm);

struct verifier {
/* files opened for verification */
  struct readerArray files;
  struct symbolArray symbols;
  struct symstringArray stmts;
  struct frameArray frames;
/* reverse polish notation stack for verifying proofs */
  struct symstringArray stack;
  size_t stackTop;
/* the file currently being verified */
  struct reader* r;
  /* nesting level for each file */
  struct size_tArray scope;
  /* index of the current file */
  size_t rId;
  enum error err;
};

void
verifierInit(struct verifier* vrf);

void
verifierClean(struct verifier* vrf);

void
verifierAddFile(struct verifier* vrf, struct reader* r);

char*
verifierParseSymbol(struct verifier* vrf, int* isEndOfStatement);

void 
verifierParseConstants(struct verifier* vrf);

void
verifierParseVariables(struct verifier* vrf);
/* stmt must be initialized */
void
verifierParseFloat(struct verifier* vrf, struct symstring* stmt);

void
verifierParseBlock(struct verifier* vrf);
#endif

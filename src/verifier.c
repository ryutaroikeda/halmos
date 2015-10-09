#include "dbg.h"
#include "reader.h"
#include "verifier.h"
#include <stdlib.h>
#include <string.h>

// static const char* stateStrings[] =
// {
//   "None",
//   "Error",
//   "Inside a comment",
//   "Expecting whitespace",
//   "Inside a block"
// };

static const char* whitespace = " \t\r\f\n";

static const int tokBeginKeyword = '$';
// static const int tokBeginBlock = '{';
// static const int tokEndBlock = '}';
// static const int tokConstant = 'c';
// static const int tokVariable = 'v';
// static const int tokFloating = 'f';
// static const int tokEssential = 'e';
// static const int tokDisjoint = 'd';
// static const int tokAxiom = 'a';
// static const int tokProposition = 'p';
static const int tokEndStatement = '.';
// static const int tokBeginProof = '=';
// static const int tokBeginFile = '[';
// static const int tokEndFile = ']';
// static const int tokBeginComment = '(';
static const int tokEndComment = ')';
// static const int tokBeginMathMode = '`';
// static const int tokEndMathMode = '`';
// static const int tokLabelMode = '~';

// const char* HalmosState_String(HalmosState s)
// {
//   return stateStrings[s];
// }

static void* xmalloc(size_t size)
{
  void* p = malloc(size);
  if (!p) {
    log_fat("Out of memory");
    abort();
  }
  return p;
}

HalmosError HalmosConstant_Init(HalmosConstant* c, const char* symbol)
{
  strncpy(c->symbol, symbol, HalmosVerifier_ConstantSymbolMax - 1);
  return HalmosError_None;
}

HalmosError HalmosVerifier_Init(HalmosVerifier* vrf, HalmosReader* r,
  const HalmosConfiguration* conf)
{
  vrf->constants = xmalloc(sizeof(HalmosConstant) * conf->constantMax);
  vrf->constantSize = 0;
  vrf->constantMax = conf->constantMax;
  // vrf->variables =
  // xmalloc(sizeof(HalmosVariable) * )
  // vrf->ccmp = &HalmosConstant_CmpSymbol;
  vrf->r = r;
  vrf->err = HalmosError_None;
  return HalmosError_None;
}

HalmosError HalmosVerifier_Clear(HalmosVerifier* vrf)
{
  free(vrf->constants);
  vrf->constants = NULL;
  vrf->constantSize = 0;
  vrf->constantMax = 0;
  return HalmosError_None;
}

int HalmosVerifier_IsFresh(const HalmosVerifier* vrf, const char* symbol)
{
  size_t i;
  for (i = 0; i < vrf->constantSize; i++) {
    if (strcmp(vrf->constants[i].symbol, symbol) == 0) {
      return 0;
    }
  }
  /* to do: check symbols in vrf->variables */
  return 1;
}

HalmosError HalmosVerifier_SetError(HalmosVerifier* vrf, HalmosError err)
{
  vrf->err = err;
  vrf->head.err = err;
  vrf->head.filename = vrf->r->filename;
  vrf->head.line = vrf->r->line;
  vrf->head.offset = vrf->r->offset;
  return HalmosError_None;
}

HalmosError
HalmosVerifier_ResizeConstants(HalmosVerifier* vrf, size_t size)
{
  void* p = realloc(vrf->constants, sizeof(HalmosConstant) * size);
  if (p == NULL) {
    log_fat("realloc failed, size == %ld", size);
    abort();
  }
  vrf->constantMax = size;
  vrf->constants = p;
  return HalmosError_None;
}

HalmosError
HalmosVerifier_AddConstant(HalmosVerifier* vrf, const char* symbol)
{
  if (!HalmosVerifier_IsFresh(vrf, symbol)) {
    HalmosVerifier_SetError(vrf, HalmosError_DuplicateSymbol);
    hlog_err(vrf->head, "duplicate declaration of symbol %s", symbol);
    return vrf->err;
  }
  if (strlen(symbol) >= HalmosVerifier_ConstantSymbolMax) {
    HalmosVerifier_SetError(vrf, HalmosError_SymbolTooBig);
    hlog_err(vrf->head,
      "the constant token %s exceeds the limit of %d characters",
      symbol, HalmosVerifier_ConstantSymbolMax);
    return vrf->err;
  }
  if (vrf->constantSize >= vrf->constantMax) {
    size_t size = vrf->constantMax * 2;
    HalmosVerifier_ResizeConstants(vrf, size);
  }
  HalmosConstant_Init(&vrf->constants[vrf->constantSize], symbol);
  vrf->constantSize++;
  return HalmosError_None;
}

// HalmosError HalmosVerifier_ParseWhitespace(HalmosVerifier* vrf)
// {
//   vrf->err = HalmosError_None;
//   int c = HalmosReader_Get(vrf->r);
//   if (!strchr(whitespace, c)) {
//     HalmosVerifier_SetError(vrf, HalmosError_ExpectedWhitespace);
//     hlog_err(vrf->head, "Expected whitespace after $(");
//     return vrf->err;
//   }
//   HalmosReader_Skip(vrf->r, whitespace);
//   return HalmosError_None;
// }

HalmosError HalmosVerifier_ParseComment(HalmosVerifier* vrf)
{
  vrf->err = HalmosError_None;
  while (vrf->r->err == HalmosError_None) {
    HalmosReader_Find(vrf->r, "$");
    /* discard the $ symbol */
    HalmosReader_Get(vrf->r);
    if (HalmosReader_Get(vrf->r) == tokEndComment) {
      return HalmosError_None;
    }
  }
  HalmosVerifier_SetError(vrf, HalmosError_UnterminatedComment);
  hlog_err(vrf->head, "reached end of file before $)");
  return vrf->err;
}

HalmosError HalmosVerifier_ParseConstants(HalmosVerifier* vrf)
{
  vrf->err = HalmosError_None;
  HalmosError err;
  while (1) {
    HalmosReader_Skip(vrf->r, whitespace);
    err = HalmosReader_GetToken(vrf->r, whitespace);
    /* check for end of statement */
    if (vrf->r->tok[0] == tokBeginKeyword) {
      size_t len = strlen(vrf->r->tok);
      if (len <= 1 || (vrf->r->tok[1] != tokEndStatement)) {
        HalmosVerifier_SetError(vrf, HalmosError_ExpectedEndStatement);
        hlog_err(vrf->head, "expected . after $ instead of %s", vrf->r->tok);
        return vrf->err;
      } else if (len > 2) {
        HalmosVerifier_SetError(vrf, HalmosError_ExpectedWhitespace);
        hlog_err(vrf->head, "expected whitespace after $. in %s", vrf->r->tok);
        return vrf->err;
      } else if (err != HalmosError_None) {
        /* len == 2 and end of file without new line */
        HalmosVerifier_SetError(vrf, HalmosError_ExpectedNewLine);
        hlog_err(vrf->head, "expected line break after $.");
        return vrf->err;
      }
      break;
    }
    /* check for end of file */
    if (err == HalmosError_EndOfString || err == HalmosError_EndOfFile) {
      HalmosVerifier_SetError(vrf, HalmosError_UnterminatedStatement);
      hlog_err(vrf->head, "reached end of file before $.");
      return vrf->err;
    } else if (err == HalmosError_TokenTooBig) {
      HalmosVerifier_SetError(vrf, err);
      hlog_err(vrf->head, "token %s... too long", vrf->r->tok);
      return vrf->err;
    }
    /* check the symbol name does not contain $ */
    if (strchr(vrf->r->tok, tokBeginKeyword)) {
      HalmosVerifier_SetError(vrf, HalmosError_InvalidSymbolName);
      hlog_err(vrf->head, "the symbol %s contains $", vrf->r->tok);
      return vrf->err;
    }
    /* add the constant symbol */
    err = HalmosVerifier_AddConstant(vrf, vrf->r->tok);
    if (err) { return err; }
  }
  return HalmosError_None;
}

// HalmosError HalmosVerifier_ParseWhitespace(HalmosVerifier* vrf, HalmosReader* r)
// {
//   // debug_assert(vrf->state == HalmosState_ExpectingWhitespace,
//   //   "state is %s, expected %s", HalmosState_String(vrf->state),
//     // HalmosState_String(HalmosState_ExpectingWhitespace));
//   vrf->err = HalmosError_None;
//   int c = HalmosReader_Get(r);
//   if (!strchr(whitespace, c)) {
//     // vrf->state = HalmosState_Error;
//     return vrf->err = HalmosError_ExpectedWhitespace;
//   }
//   HalmosReader_Skip(r, whitespace);
//   // vrf->state = HalmosState_None;
//   return HalmosError_None;
// }

// HalmosError HalmosVerifier_ParseStatement(HalmosVerifier* vrf, HalmosReader* r)
// {
//   int c = HalmosReader_Get(r);

//   return HalmosError_None;
// }

// HalmosError HalmosVerifier_ParseBlock(HalmosVerifier* vrf, HalmosReader* r)
// {
//   // debug_assert(vrf->state == HalmosState_InsideBlock,
//   //   "state is %s, expected Inside a block", HalmosState_String(vrf->state));
//   // vrf->state = HalmosState_ExpectingWhitespace;
//   HalmosVerifier_ParseWhiteSpace(vrf, r);

// }



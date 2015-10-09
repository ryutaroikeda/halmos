#ifndef _HALMOSERROR_H_
#define _HALMOSERROR_H_

#include <stddef.h>

enum HalmosError {
  HalmosError_None,
  /* HalmosReader */
  HalmosError_EndOfString,
  HalmosError_EndOfFile,
  HalmosError_TokenTooBig,
  /* HalmosConfig */
  HalmosError_ConstantMaxZero,
  /* HalmosVerifier */
  HalmosError_ReallocFailed,
  HalmosError_SymbolTooBig,
  HalmosError_DuplicateSymbol,
  HalmosError_UnterminatedComment,
  HalmosError_UnterminatedStatement,
  HalmosError_ExpectedWhitespace,
  HalmosError_ExpectedEndStatement,
  HalmosError_ExpectedKeyword,
  HalmosError_Max
};
typedef enum HalmosError HalmosError;

const char* HalmosError_String(HalmosError err);

struct HalmosErrorHeader {
  HalmosError err;
  char* filename;
  size_t line;
  size_t offset;
};
typedef struct HalmosErrorHeader HalmosErrorHeader;

HalmosError HalmosErrorHeader_Init(HalmosErrorHeader* head);

#endif

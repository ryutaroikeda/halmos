#include "error.h"

static const char* errorStrings[HalmosError_Max] = {
  "None",
  /* HalmosReader */
  "End of string",
  "End of file",
  "Token too big",
  /* HalmosConfiguration */
  "ConstantMaxZero",
  /* HalmosVerifier */
  "Realloc failed",
  "Constant symbol too big",
  "Duplicate symbol",
  "Unterminated comment",
  "Unterminated statement",
  "Expected whitespace after a keyword token",
  "ExpectedEndStatement",
  "Expected keyword symbol $",
  /* */
};

const char* HalmosError_String(HalmosError err)
{
  return errorStrings[err];
}

HalmosError HalmosErrorHeader_Init(HalmosErrorHeader* head)
{
  head->err = HalmosError_None;
  head->filename = NULL;
  head->line = 0;
  head->offset = 0;
  return HalmosError_None;
}

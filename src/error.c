#include "error.h"

static const char* errorStrings[error_size] = {
  "none",
  "endOfString",
  "endOfFile",
  "expectedNewLine",
  "expectedConstantSymbol",
  "expectedVariableSymbol",
  "expectedKeyword",
  "unexpectedKeyword",
  "unterminatedComment",
  "unterminatedStatement",
  "untypedVariable",
  "invalidSymbol",
  "undefinedSymbol",
  "duplicateSymbol",
  "duplicateFile",
  "invalidKeyword",
  "stackUnderflow",
  "mismatchedType"
};

const char*
errorString(enum error err)
{
  return errorStrings[err];
}

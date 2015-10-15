#include "error.h"

static const char* errorStrings[error_size] = {
  "none",
  "endOfString",
  "endOfFile",
  "expectedEndStatement",
  "expectedWhitespace",
  "expectedNewLine",
  "expectedConstantSymbol",
  "expectedVariableSymbol",
  "unterminatedStatement",
  "invalidSymbol",
  "undefinedSymbol",
  "duplicateSymbol",
  "duplicateFile"
};

const char*
errorString(enum error err)
{
  return errorStrings[err];
}

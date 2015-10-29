#include "error.h"

static const char* errorStrings[error_size] = {
  "none",
  "endOfString",
  "endOfFile",
  "expectedNewLine",
  "expectedConstantSymbol",
  "expectedVariableSymbol",
  "expectedFloatingSymbol",
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
  "mismatchedType",
  "mismatchedEssentialHypothesis",
  "invalidSubstitutionOfDisjoint",
  "missingDisjointRestriction",
  "unusedTermInProof",
  "incorrectProof"
};

const char*
errorString(enum error err)
{
  return errorStrings[err];
}

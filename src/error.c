#include "error.h"

/* check this matches with enum error in error.h by looking at line number */
static const char* errorStrings[error_size] = {
  "none",
/* internal errors */
  "endOfString",
  "endOfFile",
  "failedFileOpen",
  "failedFileClose",
  "invalidReaderMode",
/* external (user) errors */
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
  "invalidLabel",
  "undefinedSymbol",
  "duplicateSymbol",
  "duplicateFile",
  "invalidKeyword",
  "invalidFloatingStatement",
  "invalidEssentialStatement",
  "invalidAssertionStatement",
  "stackUnderflow",
  "mismatchedType",
  "mismatchedEssentialHypothesis",
  "invalidSubstitutionOfDisjoint",
  "missingDisjointRestriction",
  "invalidSymbolInProof",
  "unusedTermInProof",
  "incorrectProof",
  "invalidFile",
  "expectedFilename",
  "unexpectedFilename"
/* error_size */
};

const char*
errorString(enum error err)
{
  return errorStrings[err];
}

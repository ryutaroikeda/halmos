#ifndef _HALMOSERROR_H_
#define _HALMOSERROR_H_

enum error {
  error_none = 0,
  error_endOfString,
  error_endOfFile,
  error_expectedNewLine,
  error_expectedConstantSymbol,
  error_expectedVariableSymbol,
  error_expectedFloatingSymbol,
  error_expectedKeyword,
  error_unexpectedKeyword,
  error_unterminatedComment,
  error_unterminatedStatement,
  error_untypedVariable,
  error_invalidSymbol,
  error_undefinedSymbol,
  error_duplicateSymbol,
  error_duplicateFile,
  error_invalidKeyword,
  error_invalidFloatingStatement,
  error_stackUnderflow,
  error_mismatchedType,
  error_mismatchedEssentialHypothesis,
  error_invalidSubstitutionOfDisjoint,
  error_missingDisjointRestriction,
  error_invalidSymbolInProof,
  error_unusedTermInProof,
  error_incorrectProof,
  error_size
};

const char* errorString(enum error err);

#endif

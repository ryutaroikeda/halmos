#ifndef _HALMOSERROR_H_
#define _HALMOSERROR_H_

enum error {
  error_none = 0,
  error_endOfString,
  error_endOfFile,
  error_expectedEndStatement,
  error_expectedWhitespace,
  error_expectedNewLine,
  error_expectedConstantSymbol,
  error_expectedVariableSymbol,
  error_unterminatedStatement,
  error_invalidSymbol,
  error_undefinedSymbol,
  error_duplicateSymbol,
  error_duplicateFile,
  error_size
};

const char* errorString(enum error err);

#endif

#ifndef _HALMOSERROR_H_
#define _HALMOSERROR_H_

enum HalmosError {
  HalmosError_None,
  HalmosError_Reader_EndOfString,
  HalmosError_Reader_EndOfFile,
  HalmosError_Reader_TokenTooBig,
  HalmosError_Reader_Impossible,
  HalmosError_Size
};
typedef enum HalmosError HalmosError;

char* HalmosError_String(HalmosError err);

#endif

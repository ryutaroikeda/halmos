#include "error.h"

static char* errorStrings[] = {
  "None",
  "End of string",
  "End of file",
  "Token too big",
  "Impossible"
};

char* HalmosError_String(HalmosError err)
{
  return errorStrings[err];
}

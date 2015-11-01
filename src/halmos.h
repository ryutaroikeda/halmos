#ifndef _HALMOSHALMOS_H_
#define _HALMOSHALMOS_H_

#include "verifier.h"

struct halmos {
  struct verifier vrf;
};

void
halmosInit(struct halmos* h);

void
halmosClean(struct halmos* h);

void
halmosCompile(struct halmos* h, const char* filename);

/* the front end */
int
halmosMain(int argc, char* argv[]);

#endif

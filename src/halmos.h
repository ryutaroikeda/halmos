#ifndef _HALMOSHALMOS_H_
#define _HALMOSHALMOS_H_

#include "verifier.h"

enum halmosflag {
  halmosflag_none,
  halmosflag_verbose,
  halmosflag_summary,
  halmosflag_size
};

struct halmos {
  struct verifier vrf;
  char flags[halmosflag_size];
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

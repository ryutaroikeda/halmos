#ifndef _HALMOSHALMOS_H_
#define _HALMOSHALMOS_H_

#include "preproc.h"
#include "verifier.h"

enum halmosflag {
  halmosflag_none = 0,
  halmosflag_verbose,
  halmosflag_summary,
  halmosflag_preproc,
  halmosflag_size
};

struct halmos {
  struct verifier vrf;
  struct preproc p;
  char flags[halmosflag_size];
  char** flagsArgv[halmosflag_size];
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

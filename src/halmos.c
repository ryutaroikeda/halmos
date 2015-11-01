#include "dbg.h"
#include "halmos.h"
#include <stdio.h>

void
halmosInit(struct halmos* h)
{
  verifierInit(&h->vrf);
}

void
halmosClean(struct halmos* h)
{
  verifierClean(&h->vrf);
}

void
halmosCompile(struct halmos* h, const char* filename)
{
  verifierParseFile(&h->vrf, filename);
  printf("Found %lu errors\n", h->vrf.errc);
}

int
halmosMain(int argc, char* argv[])
{
  (void) argc;
  struct halmos h;
  halmosInit(&h);
  halmosCompile(&h, argv[1]);
  halmosClean(&h);
  return 0;
}

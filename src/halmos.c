#include "dbg.h"
#include "halmos.h"
#include "verifier.h"
#include <stdio.h>

struct halmos {
  struct verifier vrf;
};

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
  FILE* f = fopen(filename, "r");
  if (!f) {
    LOG_ERR("could not open file %s", filename);
    return;
  }
  struct reader r;
  readerInitFile(&r, f, filename);
  size_t rId = verifierAddFile(&h->vrf, &r);
  verifierBeginReadingFile(&h->vrf, rId);
  verifierParseBlock(&h->vrf);
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

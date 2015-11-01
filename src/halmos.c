#include "dbg.h"
#include "halmos.h"
#include <stdio.h>

void
halmosInit(struct halmos* h)
{
  size_t i;
  verifierInit(&h->vrf);
  for (i = 0; i < halmosflag_size; i++) {
    h->flags[i] = 0;
  }
}

void
halmosClean(struct halmos* h)
{
  verifierClean(&h->vrf);
}

void
halmosCompile(struct halmos* h, const char* filename)
{
  size_t i;
  verifierParseFile(&h->vrf, filename);
  printf("Found %lu errors\n", h->vrf.errc);
  if (h->flags[halmosflag_summary]) {
    printf("------summary\n");
    for (i = symType_constant; i < symType_size; i++) {
      printf("Parsed %lu %s symbols\n", h->vrf.symCount[i], symTypeString(i));
    }
  }
}

int
halmosMain(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Usage: halmos <filename> [flags]\n");
    return 0;
  }
  struct halmos h;
  halmosInit(&h);
  if (argc > 2) {
    h.flags[halmosflag_summary] = 1;
  }
  halmosCompile(&h, argv[1]);
  halmosClean(&h);
  return 0;
}

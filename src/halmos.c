#include "dbg.h"
#include "halmos.h"
#include <stdio.h>

static const char* flags[halmosflag_size] = {
  "",
  "--verbose",
  "--summary",
  "--preproc",
};

static const size_t flagsArgc[halmosflag_size] = {
  0, /* none */
  0, /* verbose */
  0, /* summary */
  1, /* preproc - the output file */
};

void
halmosInit(struct halmos* h)
{
  size_t i;
  verifierInit(&h->vrf);
  preprocInit(&h->p);
  for (i = 0; i < halmosflag_size; i++) {
    h->flags[i] = 0;
    h->flagsArgv[i] = NULL;
  }
}

void
halmosClean(struct halmos* h)
{
  preprocClean(&h->p);
  verifierClean(&h->vrf);
}

void
halmosCompile(struct halmos* h, const char* filename)
{
  size_t i;
  if (h->flags[halmosflag_preproc]) {
    printf("------preproc\n");
    preprocParseFile(&h->p, filename, h->flagsArgv[halmosflag_preproc][0]);
  } else {
    verifierParseFile(&h->vrf, filename);
    printf("Found %lu errors\n", h->vrf.errc);
  }
  if (h->flags[halmosflag_summary]) {
    printf("------summary\n");
    for (i = symType_constant; i < symType_size; i++) {
      printf("Parsed %lu %s symbols\n", h->vrf.symCount[i], symTypeString(i));
    }
  }
}

enum halmosflag
halmosParseFlag(struct halmos* h, const char* tok)
{
  size_t i;
  for (i = 0; i < halmosflag_size; i++) {
    if (strcmp(flags[i], tok) == 0) {
      h->flags[i] = 1;
      return i;
    }
  }
  return halmosflag_none;
}

int
halmosMain(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Usage: halmos [flags] <filename>\n");
    return 0;
  }
  struct halmos h;
  halmosInit(&h);
  int i;
  for (i = 1; i < argc - 1; i++) {
    enum halmosflag flag = halmosParseFlag(&h, argv[i]);
    if (flagsArgc[flag] > 0) {
      h.flagsArgv[flag] = &argv[i + 1];
    }
    i += flagsArgc[flag];
    if (i >= argc - 1) {
      printf("%s takes %lu arguments\n", flags[flag], flagsArgc[flag]);
      return 0;
    }
  }
  halmosCompile(&h, argv[argc - 1]);
  halmosClean(&h);
  return 0;
}

#include "dbg.h"
#include "halmos.h"
#include "preproc.h"
#include "verifier.h"
#include <errno.h>
#include <stdio.h>

static const char* flags[halmosflag_size] = {
  "",
  "--verbose",
  "--summary",
  "--preproc",
  "--no-preproc",
  "--no-verify",
  // "--include",
};

static const size_t flagsArgc[halmosflag_size] = {
  0, /* none */
  1, /* verbose */
  0, /* summary */
  1, /* preproc - the output file */
  0, /* no-preproc */
  0, /* no-verify */
  // 0, /* include */
};

void
halmosInit(struct halmos* h)
{
  size_t i;
  for (i = 0; i < halmosflag_size; i++) {
    h->flags[i] = 0;
    h->flagsArgv[i] = NULL;
  }
}

void
halmosClean(struct halmos* h)
{
  (void) h;
}

void
halmosCompile(struct halmos* h, const char* filename)
{
  size_t i;
  struct verifier vrf;
  struct preproc p;
  verifierInit(&vrf);
  preprocInit(&p);
  if (h->flags[halmosflag_verbose]) {
    errno = 0;
    size_t verb = strtoul(h->flagsArgv[halmosflag_verbose][0], NULL, 10);
    if (errno) {
      printf("%s requires a positive integer\n", flags[halmosflag_verbose]);
      h->flags[halmosflag_no_preproc] = 1;
      h->flags[halmosflag_no_verify] = 1;
    } else {
      verifierSetVerbosity(&vrf, verb);
    }
  }
  if (!h->flags[halmosflag_no_preproc]) {
    printf("------preproc\n");
    printf("------%s\n", filename);
    preprocCompile(&p, filename, "out.mm");
    printf("Found %lu errors\n", p.errCount);
  }
/* don't compile if preproc was specified */
  if (!h->flags[halmosflag_preproc] && !h->flags[halmosflag_no_verify]) {
    printf("------verifier\n");
    verifierCompile(&vrf, "out.mm");
    printf("Found %lu errors\n", vrf.errc);
  }
  if (h->flags[halmosflag_summary]) {
    printf("------summary\n");
    for (i = symType_constant; i < symType_size; i++) {
      printf("Parsed %lu %s symbols\n", vrf.symCount[i], symTypeString(i));
    }
  }
  preprocClean(&p);
  verifierClean(&vrf);
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
  if (i > argc) {
    printf("Missing input file\n");
    return 0;
  }
  halmosCompile(&h, argv[argc - 1]);
  halmosClean(&h);
  return 0;
}

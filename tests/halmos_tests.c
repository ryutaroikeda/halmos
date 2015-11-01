#include "unittest.h"
#include "halmos.h"

#define test_file(filename, count) \
static int test_ ## filename(void) \
{ struct halmos h; \
  halmosInit(&h); \
  halmosCompile(&h, "tests/" #filename ".mm"); \
  ut_assert(h.vrf.errc == count, "got %lu errors, expected %d", \
    h.vrf.errc, count); \
  return 0; \
}

test_file(test1, 0)
test_file(big_unifier, 0)
test_file(recursive_include, 0)
test_file(symbol_import, 0)

static int
all(void)
{
  ut_run(test_test1);
  ut_run(test_big_unifier);
  ut_run(test_recursive_include);
  ut_run(test_symbol_import);
  return 0;
}

RUN(all)

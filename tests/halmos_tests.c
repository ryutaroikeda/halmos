#include "unittest.h"
#include "halmos.h"

static int
test_demo0_mm(void)
{
  return 0;
}

static int
all(void)
{
  ut_run(test_demo0_mm);
  return 0;
}

RUN(all)

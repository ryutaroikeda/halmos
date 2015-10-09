#include "unittest.h"
#include "reader.h"
#include "verifier.h"

#define check_err(got, expect) \
ut_assert(got == expect, #got " == %s, expected " #expect, \
  HalmosError_String(got))

static int Test_HalmosVerifier_ParseComment()
{
  const char f[] = " "
  "And here were forests ancient as the hills\n"
  "Enfolding sunny spots of greenery$)*";
  const char g[] = " "
  "But oh! that deep romantic chasm which slanted\n"
  "Down the green hill athward a cedarn cover!";
  int c;
  HalmosError err;
  HalmosReader r;
  HalmosReader_InitString(&r, f);
  HalmosConfiguration conf;
  conf.constantMax = 1;
  HalmosVerifier vrf;
  HalmosVerifier_Init(&vrf, &r, &conf);
  err = HalmosVerifier_ParseComment(&vrf);
  ut_assert(vrf.err == err, "err and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  c = HalmosReader_Get(&r);
  ut_assert(c == '*', "Get() == %c, expected *", c);
  HalmosReader_InitString(&r, g);
  err = HalmosVerifier_ParseComment(&vrf);
  ut_assert(vrf.err == err, "err and .err mismatch");
  ut_assert(vrf.err == HalmosError_UnterminatedComment,
    ".err == %s, expected %s", HalmosError_String(vrf.err),
    HalmosError_String(HalmosError_UnterminatedComment));
  HalmosVerifier_Clean(&vrf);
  return 0;
}

static int Test_HalmosVerifier_AddConstant()
{
  char sym[] = "Asavageplace!asholyandenchanted";
  HalmosError err;
  HalmosReader r;
  HalmosConfiguration conf;
  conf.constantMax = 1;
  HalmosVerifier vrf;
  err = HalmosVerifier_Init(&vrf, &r, &conf);
  check_err(err, HalmosError_None);
  err = HalmosVerifier_AddConstant(&vrf, sym);
  ut_assert(vrf.constantSize == 1, ".constantSize wrong");
  return 0;
}

static int all()
{
  ut_run(Test_HalmosVerifier_ParseComment);
  ut_run(Test_HalmosVerifier_AddConstant);
  return 0;
}

RUN(all)

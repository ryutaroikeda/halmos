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
  HalmosVerifier_Clear(&vrf);
  return 0;
}

static int Test_HalmosVerifier_AddConstant()
{
  char* sym = "Asavageplace!asholyandenchanted";
  HalmosError err;
  HalmosReader r;
  HalmosReader_InitString(&r, "");
  HalmosConfiguration conf;
  conf.constantMax = 1;
  HalmosVerifier vrf;
  err = HalmosVerifier_Init(&vrf, &r, &conf);
  check_err(err, HalmosError_None);
  err = HalmosVerifier_AddConstant(&vrf, sym);
  ut_assert(vrf.constantSize == 1, ".constantSize wrong");
  err = HalmosVerifier_AddConstant(&vrf, sym);
  check_err(err, HalmosError_DuplicateSymbol);
  ut_assert(vrf.constantSize == 1, ".constantSize wrong");
  char bigsym[HalmosVerifier_ConstantSymbolMax + 1];
  memset(bigsym, 'a', HalmosVerifier_ConstantSymbolMax);
  bigsym[HalmosVerifier_ConstantSymbolMax] = '\0';
  err = HalmosVerifier_AddConstant(&vrf, bigsym);
  check_err(err, HalmosError_SymbolTooBig);
  ut_assert(vrf.constantSize == 1, ".constantSize wrong");
  sym = "Ase'erbeneathawaningmoonwashaunted";
  err = HalmosVerifier_AddConstant(&vrf, sym);
  check_err(err, HalmosError_None);
  ut_assert(vrf.constantSize == 2, ".constantSize wrong");
  ut_assert(vrf.constantMax == 2, ".constantMax wrong");
  HalmosVerifier_Clear(&vrf);
  return 0;
}

static int Test_HalmosVerifier_ParseConstants()
{
  char* file;
  HalmosError err;
  HalmosReader r;
  HalmosConfiguration conf;
  conf.constantMax = 1;
  HalmosVerifier vrf;
#define testfile(f, e) \
  do { \
    file = f; \
    HalmosReader_InitString(&r, file); \
    HalmosVerifier_Init(&vrf, &r, &conf); \
    err = HalmosVerifier_ParseConstants(&vrf); \
    check_err(err, e); \
    HalmosVerifier_Clear(&vrf); \
  } while (0)

  testfile("By woman wailing for her demon-lover! $$",
   HalmosError_ExpectedEndStatement);
  testfile("And from this chasm with ceaseless turmoil seething, $..",
    HalmosError_ExpectedWhitespace);
  testfile("As if this earth in fast thick pants were breathing, $.",
    HalmosError_ExpectedNewLine);
  testfile("A mighty mountain was momently forced:",
    HalmosError_UnterminatedStatement);
  testfile("Amid those swift half-intermitted bursts$. ",
    HalmosError_InvalidSymbolName);
  char g[HalmosReader_TokenMax + 2];
  memset(g, 'a', HalmosReader_TokenMax + 2);
  g[HalmosReader_TokenMax + 1] = '\0';
  testfile(g, HalmosError_TokenTooBig);
#undef testfile
  return 0;
}

static int all()
{
  ut_run(Test_HalmosVerifier_ParseComment);
  ut_run(Test_HalmosVerifier_AddConstant);
  ut_run(Test_HalmosVerifier_ParseConstants);
  return 0;
}

RUN(all)

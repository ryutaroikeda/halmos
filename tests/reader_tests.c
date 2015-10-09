#include "unittest.h"
#include "error.h"
#include "reader.h"
#include <string.h>

static int Test_HalmosReader_Get()
{
  HalmosError err;
  int c;
  const char s[60] =
  "In Xanadu did Kubla Khan\n"
  "A stately pleasure-dome decree :";
  HalmosReader r;
  err = HalmosReader_InitString(&r, s);
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  c = HalmosReader_Get(&r);
  ut_assert(c == 'I', "get() == %c, expected 'I'", c);
  ut_assert(r.err == HalmosError_None, ".err == %s, expected None",
    HalmosError_String(r.err));
  c = HalmosReader_Get(&r);
  ut_assert(c == 'n', "get() == %c, expected 'n'", c);
  ut_assert(r.err == HalmosError_None, ".err == %s, expected None",
    HalmosError_String(r.err));
  int i;
  for (i = 0; i < 55; i++) {
    HalmosReader_Get(&r);
    ut_assert(r.err == HalmosError_None, ".err == %s, expected None",
      HalmosError_String(r.err));
  }
  c = HalmosReader_Get(&r);
  ut_assert(c == 0, "get() == %c, expected 0", c);
  ut_assert(r.err == HalmosError_EndOfString, ".err == %s, expected %s",
    HalmosError_String(r.err),
     HalmosError_String(HalmosError_EndOfString));
  return 0;
}

static int Test_HalmosReader_GetToken()
{
  HalmosError err;
  const char s[] = 
  "Where Alph, the sacred river, ran\n"
  "Through caverns measureless to man\n"
  "Down to a sunless sea.";
  HalmosReader r;
  HalmosReader_InitString(&r, s);
  err = HalmosReader_GetToken(&r, " ");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  ut_assert(strcmp(r.tok, "Where") == 0, "tok == %s, expected Where", r.tok);
  err = HalmosReader_GetToken(&r, " ,");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  ut_assert(strcmp(r.tok, "Alph") == 0, "tok == %s, expected Alph", r.tok);
  err = HalmosReader_GetToken(&r, "$");
  ut_assert(r.err == err, "returned error and .err mismatch");
  char t[HalmosReader_TokenMax + 2];
  memset(t, 'a', HalmosReader_TokenMax);
  t[HalmosReader_TokenMax] = '@';
  t[HalmosReader_TokenMax + 1] = '\0';
  HalmosReader_InitString(&r, t);
  err = HalmosReader_GetToken(&r, "@");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(r.err == HalmosError_TokenTooBig, ".err == %s, expected %s",
    HalmosError_String(r.err),
     HalmosError_String(HalmosError_TokenTooBig));
  return 0;
}

static int Test_HalmosReader_Skip()
{
  HalmosError err;
  int c;
  const char f[] =
  "So twice five miles of fertile ground\n"
  "With walls and towers were girdled round;";
  HalmosReader r;
  HalmosReader_InitString(&r, f);
  err = HalmosReader_Skip(&r, "Sotwicefivemiles ");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  ut_assert(r.didSkip != 0, ".didSkip == 0, expected 1");
  ut_assert(r.skipped == 'r', ".skipped == %c, expected r", r.skipped);
  c = HalmosReader_Get(&r);
  ut_assert(c == 'r', "get() == %c, expected r", c);
  err =
   HalmosReader_Skip(&r, "tileground\nWithwallsandtowersweregirdledround; ");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(r.err == HalmosError_EndOfString, ".err == %s, expected %s",
    HalmosError_String(r.err),
     HalmosError_String(HalmosError_EndOfString));
  return 0;
}

static int Test_HalmosReader_Find()
{
  HalmosError err;
  int c;
  const char f[] =
  "And there were gardens bright with sinuous rills,\n"
  "Where blossomed many many an incense-bearing tree";
  HalmosReader r;
  HalmosReader_InitString(&r, f);
  err = HalmosReader_Find(&r, "g");
  ut_assert(r.err == err, "returned error and .err mismatch");
  ut_assert(err == HalmosError_None, "err == %s, expected None",
    HalmosError_String(err));
  c = HalmosReader_Get(&r);
  ut_assert(c == 'g', "Get() == %c, expected g", c);
  return 0;
}

static int all()
{
  ut_run(Test_HalmosReader_Get);
  ut_run(Test_HalmosReader_GetToken);
  ut_run(Test_HalmosReader_Skip);
  ut_run(Test_HalmosReader_Find);
  return 0;
}

RUN(all)

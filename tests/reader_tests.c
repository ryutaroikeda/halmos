#include "unittest.h"
#include "reader.h"
#include <string.h>

static int Test_readerGet(void)
{
  int c;
  const char s[60] =
  "In Xanadu did Kubla Khan\n"
  "A stately pleasure-dome decree :";
  struct reader r;
  readerInitString(&r, s);
  ut_assert(r.err == error_none, "err == %s, expected None",
   errorString(r.err));
  c = readerGet(&r);
  ut_assert(c == 'I', "get() == %c, expected 'I'", c);
  ut_assert(r.err == error_none, ".err == %s, expected None",
   errorString(r.err));
  c = readerGet(&r);
  ut_assert(c == 'n', "get() == %c, expected 'n'", c);
  ut_assert(r.err == error_none, ".err == %s, expected None",
   errorString(r.err));
  int i;
  for (i = 0; i < 55; i++) {
    readerGet(&r);
    ut_assert(r.err == error_none, ".err == %s, expected None",
     errorString(r.err));
  }
  c = readerGet(&r);
  ut_assert(c == 0, "get() == %c, expected 0", c);
  ut_assert(r.err == error_endOfString, ".err == %s, expected %s",
    errorString(r.err), errorString(error_endOfString));
  readerClean(&r);
  return 0;
}

static int Test_readerGetToken(void)
{
  const char s[] = 
  "Where Alph, the sacred river, ran\n"
  "Through caverns measureless to man\n"
  "Down to a sunless sea.";
  struct reader r;
  readerInitString(&r, s);
  readerGetToken(&r, " ");
  ut_assert(r.err == error_none, "err == %s, expected None",
    errorString(r.err));
  ut_assert(strcmp(r.tok.vals, "Where") == 0, "tok == %s, expected Where", r.tok.vals);
  // ut_assert(r.last == 'e', "last == %c, expected e", r.last);
  readerGetToken(&r, " ,");
  ut_assert(r.err == error_none, "err == %s, expected None",
    errorString(r.err));
  ut_assert(strcmp(r.tok.vals, "Alph") == 0, "tok == %s, expected Alph", r.tok.vals);
  // ut_assert(r.last == 'h', "last == %c, expected h", r.last);
  readerGetToken(&r, "$");
  // ut_assert(r.last == '.', "last == %c, expected '.'", r.last);
  readerClean(&r);
  char t[1111111 + 1];
  memset(t, 'a', 1111111);
  t[1111111] = '\0';
  readerInitString(&r, t);
  readerGetToken(&r, "");
  ut_assert(r.err == error_endOfString, ".err == %s, expected %s",
    errorString(r.err), errorString(error_endOfString));
  t[1111110] = ' ';
  readerClean(&r);
  readerInitString(&r, t);
  readerGetToken(&r, " ");
  ut_assert(r.err == error_none, ".err == %s, expected %s",
    errorString(r.err), errorString(error_none));
  readerClean(&r);
  return 0;
}

static int Test_readerSkip(void)
{
  int c;
  const char f[] =
  "So twice five miles of fertile ground\n"
  "With walls and towers were girdled round;";
  struct reader r;
  readerInitString(&r, f);
  readerSkip(&r, "Sotwicefivemiles ");
  ut_assert(r.err == error_none, "err == %s, expected None",
    errorString(r.err));
  ut_assert(r.didSkip != 0, ".didSkip == 0, expected 1");
  ut_assert(r.skipped == 'r', ".skipped == %c, expected r", r.skipped);
  // ut_assert(r.last == 'e', "last == %c, expected e", r.last);
  c = readerGet(&r);
  ut_assert(c == 'r', "get() == %c, expected r", c);
  readerSkip(&r, "tileground\nWithwallsandtowersweregirdledround; ");
  ut_assert(r.err == error_endOfString, ".err == %s, expected %s",
    errorString(r.err), errorString(error_endOfString));
  ut_assert(r.last == ';', "last == %c, expected ;", r.last);
  readerClean(&r);
  return 0;
}

static int Test_readerFind(void)
{
  int c;
  const char f[] =
  "And there were gardens bright with sinuous rills,\n"
  "Where blossomed many many an incense-bearing tree";
  struct reader r;
  readerInitString(&r, f);
  readerFind(&r, "g");
  ut_assert(r.err == error_none, "err == %s, expected None",
    errorString(r.err));
  c = readerGet(&r);
  ut_assert(c == 'g', "Get() == %c, expected g", c);
  readerClean(&r);
  return 0;
}

static int all()
{
  ut_run(Test_readerGet);
  ut_run(Test_readerGetToken);
  ut_run(Test_readerSkip);
  ut_run(Test_readerFind);
  return 0;
}

RUN(all)

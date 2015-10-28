/* verifier_tests.c */
/* 15 Oct 2015 */
#include "unittest.h"
#include "reader.h"
#include "verifier.h"

static int
Test_frameInit(void)
{
  struct frame frm;
  frameInit(&frm);
  ut_assert(frm.stmts.size == 0, "size == %lu, expected 0", frm.stmts.size);
  frameClean(&frm);
  return 0;
}

static int
Test_verifierInit(void)
{
  char f[] = "";
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader file;
  readerInitString(&file, f);
  charArrayAppend(&file.filename, "test", 4);
  verifierAddFile(&vrf, &file);
  vrf.r = &vrf.files.vals[0];
  ut_assert(vrf.files.size == 1, "files.size == %lu, expected 1",
    vrf.files.size);
  // struct statement stmt;
  // statementInit(&stmt);
  // statementArrayAdd(&vrf.stmts, stmt);
  // ut_assert(vrf.stmts.size == 1, "stmts.size == %lu, expected 1",
   // vrf.stmts.size);
  // struct frame frm;
  // frameInit(&frm);
  // frameArrayAdd(&vrf.frames, frm);
  // ut_assert(vrf.frames.size == 1, "frames.size == %lu, expected 1", 
  //  vrf.frames.size);
  verifierClean(&vrf);
  return 0;
}

#define check_err(actual, expected) \
do { \
  ut_assert(actual == expected, "err = %s, expected %s", errorString(actual), \
   errorString(expected)); \
} while (0)

static int
Test_verifierParseSymbol(void)
{
  char* file;
  struct reader r;
  struct verifier vrf;
  int isEndOfStatement;
#define testfile(f, e) \
  do { \
    file = f; \
    readerInitString(&r, file); \
    verifierInit(&vrf); \
    verifierAddFile(&vrf, &r); \
    vrf.r = &vrf.files.vals[0]; \
    verifierParseSymbol(&vrf, &isEndOfStatement); \
    check_err(vrf.err, e); \
    verifierClean(&vrf); \
  } while (0)

  // testfile("$$", error_expectedEndStatement);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
  //  isEndOfStatement);
  // testfile("$..", error_expectedWhitespace);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   // isEndOfStatement);
  // testfile("$.", error_expectedNewLine);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
  //  isEndOfStatement);
  testfile("forced:", error_unterminatedStatement);
  ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   isEndOfStatement);
  testfile("bursts$. ", error_invalidSymbol);
  ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   isEndOfStatement);
  testfile("$. ", error_none);
  ut_assert(isEndOfStatement == 1, "isEndOfStatement is %d, expected 1",
   isEndOfStatement);
#undef testfile
  return 0;
}

static int
Test_verifierParseConstants(void)
{
  char* file;
  struct reader r;
  struct verifier vrf;
#define testparse(f, e, n) \
  do { \
    file = f; \
    readerInitString(&r, file); \
    verifierInit(&vrf); \
    verifierAddFile(&vrf, &r); \
    vrf.r = &vrf.files.vals[0]; \
    verifierParseConstants(&vrf); \
    check_err(vrf.err, e); \
    ut_assert(vrf.symbols.size == n, "size == %lu, expected %lu", \
     vrf.symbols.size, n); \
    verifierClean(&vrf); \
  } while (0)
  testparse("", error_unterminatedStatement, (size_t)0);
  testparse("A savage place! as holy and enchanted $.\n", error_none,
   (size_t)7);
  readerInitString(&r, "savage $.\n");
  charArrayAppend(&r.filename, "file1", 5 + 1);
  verifierInit(&vrf);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[0];
  verifierParseConstants(&vrf);
  readerInitString(&r, "savage $.\n");
  charArrayAppend(&r.filename, "file2", 5 + 1);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[1];
  verifierParseConstants(&vrf);
  check_err(vrf.err, error_duplicateSymbol);
  verifierClean(&vrf);
  return 0;
#undef testparse
}

static int
Test_verifierParseVariables()
{
  char* file = "And here were forests ancient as the hills $.\n";
  struct reader r;
  struct verifier vrf;
  readerInitString(&r, file);
  charArrayAppend(&r.filename, "file", 4 + 1);
  verifierInit(&vrf);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[0];
  verifierParseVariables(&vrf);
  check_err(vrf.err, error_none);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierIsValidDisjointPairSubstitution(void)
{
  struct verifier vrf;
  struct frame frm;
  struct substitution sub;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "v1", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v2", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v3", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  LOG_INFO("added symbol");
  size_t v1 = verifierGetSymId(&vrf, "v1");
  size_t v2 = verifierGetSymId(&vrf, "v2");
  size_t v3 = verifierGetSymId(&vrf, "v3");
  ut_assert(!vrf.err, "failed to find symbol");
  frameInit(&frm);
  frameAddDisjoint(&frm, v1, v2);
/* build the substitution */
  struct symstring str1, str2;
  symstringInit(&str1);
  symstringInit(&str2);
  symstringAdd(&str1, v3);
  symstringAdd(&str2, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
   "sub should be invalid");
/* str1 and str2 cleaned here */
  substitutionClean(&sub);
  frameClean(&frm);
  frameInit(&frm);
  frameAddDisjoint(&frm, v1, v2);
  symstringInit(&str1);
  symstringInit(&str2);
  symstringAdd(&str1, v1);
  symstringAdd(&str1, v2);
  symstringAdd(&str2, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be invalid");
  frameAddDisjoint(&frm, v1, v3);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be invalid");
  frameAddDisjoint(&frm, v2, v3);
  ut_assert(verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be valid");
  substitutionClean(&sub);
  frameClean(&frm);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierIsValidSubstitution(void)
{
  struct verifier vrf;
  struct substitution sub;
  struct frame frm;
  size_t v1, v2, v3;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "v1", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v2", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v3", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  v1 = verifierGetSymId(&vrf, "v1");
  v2 = verifierGetSymId(&vrf, "v2");
  v3 = verifierGetSymId(&vrf, "v3");
  frameInit(&frm);
/* build the substitution */
  struct symstring str1, str2, str3;
  symstringInit(&str1);
  symstringInit(&str2);
  symstringInit(&str3);
  symstringAdd(&str1, v1);
  symstringAdd(&str2, v1);
  symstringAdd(&str3, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  substitutionAdd(&sub, v3, &str3);
  ut_assert(verifierIsValidSubstitution(&vrf, &frm, &sub), "invalid sub");
  frameAddDisjoint(&frm, v2, v3);
  ut_assert(!verifierIsValidSubstitution(&vrf, &frm, &sub),
    "sub should be invalid");
  frameAddDisjoint(&frm, v1, v3);
  ut_assert(verifierIsValidSubstitution(&vrf, &frm, &sub), "invalid sub");
  substitutionClean(&sub);
  frameClean(&frm);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierUnify(void)
{
  const size_t type = 0;
  const size_t var = 1;
  const size_t type2 = 2;
  const size_t strVals[4] = {type, var, var, var};
  const size_t floatingVals[2] = {type, var};
  const size_t floatingVals2[2] = {type2, var};
  struct verifier vrf;
  struct substitution sub;
  struct symstring str;
  struct symstring floating;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "type", symType_constant, 0, 0, 0, 0, 0, 0,
    0, 0);
  verifierAddSymbolExplicit(&vrf, "v", symType_variable, 0, 0, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "type2", symType_constant, 0, 0, 0, 0, 0, 0,
    0, 0);
  symstringInit(&str);
  symstringInit(&floating);
  size_tArrayAppend(&str, strVals, 4);
  size_tArrayAppend(&floating, floatingVals, 2);
  substitutionInit(&sub);
  verifierUnify(&vrf, &sub, &str, &floating);
  ut_assert(!vrf.err, "unification failed");
  symstringClean(&floating);
  symstringInit(&floating);
  symstringClean(&str);
  symstringInit(&str);
  size_tArrayAppend(&str, strVals, 4);
  size_tArrayAppend(&floating, floatingVals2, 2);
  verifierUnify(&vrf, &sub, &str, &floating);
  ut_assert(vrf.err == error_mismatchedType, "unifciation should have failed");
  symstringClean(&str);
  symstringClean(&floating);
  substitutionClean(&sub);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierApplyAssertion(void)
{
  const size_t t = 0;
  const size_t x = 1;
  const size_t y = 2;
  const size_t a = 3;
  const size_t b = 4;
  const size_t tx = 5;
  const size_t ty = 6;
  const size_t txy = 7;
  const size_t tyx = 8;
  const size_t tx_f[2] = {t, x};
  const size_t ty_f[2] = {t, y};
  const size_t txy_e[3] = {t, x, y};
  const size_t tyx_a[3] = {t, y, x};
  const size_t frame_a[3] = {tx, ty, txy};
  const size_t stack1[2] = {t, a};
  const size_t stack2[2] = {t, b};
  const size_t stack3[3] = {t, a, b};
  const size_t res[3] = {t, b, a};
  struct verifier vrf;
  struct symstring stmt;
  struct frame frm;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "t", symType_constant, 1, 0, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "x", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "y", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "a", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "b", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, tx_f, 2);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "tx", symType_floating, 1, 0, 0, 0, 0, 0, 0,
    0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, ty_f, 2);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "ty", symType_floating, 1, 0, 0, 1, 0, 0,
    0, 0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, txy_e, 3);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "txy", symType_essential, 1, 0, 0, 2, 0, 0,
    0, 0);
/* the frame for tyx */
  frameInit(&frm);
  size_tArrayAppend(&frm.stmts, frame_a, 3);
  verifierAddFrame(&vrf, &frm);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, tyx_a, 3);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "tyx", symType_assertion, 1, 0, 0, 3, 0, 0,
    0, 0);
/* prepare the stack */
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack1, 2);
  symstringArrayAdd(&vrf.stack, stmt);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack2, 2);
  symstringArrayAdd(&vrf.stack, stmt);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack3, 3);
  symstringArrayAdd(&vrf.stack, stmt);
/* apply the assertion */
  verifierApplyAssertion(&vrf, tyx);
  ut_assert(!vrf.err, "assertion application failed");
  (void) res;
  (void) tyx;
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, res, 3);
  ut_assert(symstringIsEqual(&vrf.stack.vals[0], &stmt), "result of assertion "
    "is wrong");
  symstringClean(&stmt);
/* frm is cleaned here */
  verifierClean(&vrf);
  return 0;
}

static int
all(void)
{
  ut_run(Test_frameInit);
  ut_run(Test_verifierInit);
  ut_run(Test_verifierParseSymbol);
  ut_run(Test_verifierParseConstants);
  ut_run(Test_verifierParseVariables);
  ut_run(Test_verifierIsValidDisjointPairSubstitution);
  ut_run(Test_verifierIsValidSubstitution);
  ut_run(Test_verifierUnify);
  ut_run(Test_verifierApplyAssertion);
  return 0;
}

RUN(all)
